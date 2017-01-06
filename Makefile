CXXFLAGS += -std=c++14 -Wall -Wextra -Werror -MMD -I$(abspath src)
CXXFLAGS += -Wno-non-template-friend
LDFLAGS  += -g -lsqlite3 -lgit2 -lsource-highlight -lz
LDFLAGS  += -lboost_filesystem -lboost_iostreams -lboost_system

INSTALL := install -D

ifneq ($(OS),Windows_NT)
    bin_suffix :=
else
    bin_suffix := .exe
endif

# this function of two arguments (array and element) returns index of the
# element in the array; return -1 if item not found in the list
pos = $(strip $(eval T := ) \
              $(eval i := -1) \
              $(foreach elem, $1, \
                        $(if $(filter $2,$(elem)), \
                                      $(eval i := $(words $T)), \
                                      $(eval T := $T $(elem)))) \
              $i)

# determine output directory and build target; "." is the directory by default
# or "release"/"debug" for corresponding targets
is_release := 0
ifneq ($(call pos,release,$(MAKECMDGOALS)),-1)
    is_release := 1
endif
ifneq ($(call pos,install,$(MAKECMDGOALS)),-1)
    is_release := 1
endif
ifneq ($(is_release),0)
    EXTRA_CXXFLAGS := -O3
    # EXTRA_LDFLAGS  := -Wl,--strip-all

    out_dir := release
    target  := release
else
    EXTRA_CXXFLAGS := -O0 -g
    EXTRA_LDFLAGS  := -g

    ifneq ($(call pos,debug,$(MAKECMDGOALS)),-1)
        out_dir := debug
    else
        ifneq ($(call pos,sanitize-basic,$(MAKECMDGOALS)),-1)
            out_dir := sanitize-basic
            EXTRA_CXXFLAGS += -fsanitize=address -fsanitize=undefined
            EXTRA_LDFLAGS  += -fsanitize=address -fsanitize=undefined -pthread
        else
            with_cov := 0
            ifneq ($(call pos,coverage,$(MAKECMDGOALS)),-1)
                with_cov := 1
            endif
            ifneq ($(call pos,self-coverage,$(MAKECMDGOALS)),-1)
                with_cov := 1
            endif

            ifneq ($(with_cov),0)
                out_dir := coverage
                EXTRA_CXXFLAGS += --coverage
                EXTRA_LDFLAGS  += --coverage
            else
                out_dir := .
            endif
        endif
    endif
    target := debug
endif

# traverse directories ($1) recursively looking for a pattern ($2) to make list
# of matching files
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) \
                                        $(filter $(subst *,%,$2),$d))

bin_name := uncov$(bin_suffix)
bin := $(out_dir)/$(bin_name)

bin_sources := $(call rwildcard, src/, *.cpp)
bin_objects := $(bin_sources:%.cpp=$(out_dir)/%.o)
bin_depends := $(bin_objects:.o=.d)

tests_sources := $(call rwildcard, tests/, *.cpp)
tests_sources := $(filter-out tests/test-repo/%, $(tests_sources))

tests_objects := $(tests_sources:%.cpp=$(out_dir)/%.o)
tests_objects += $(filter-out %/main.o,$(bin_objects))
tests_depends := $(tests_sources:%.cpp=$(out_dir)/%.d)

out_dirs := $(sort $(dir $(bin_objects) $(tests_objects)))

.PHONY: all check clean debug release sanitize-basic man install uninstall
.PHONY: coverage self-coverage reset-coverage

all: $(bin)

debug release: $(bin)

sanitize-basic: $(bin)

coverage: check $(bin)
	find $(out_dir)/ -name '*.o' -exec gcov -p {} +
	$(GCOV_PREFIX)uncov-gcov --root . --build-root . --no-gcov \
	                         --capture-worktree --exclude tests \
	| $(UNCOV_PREFIX)uncov new
	find . -name '*.gcov' -delete

self-coverage: UNCOV_PREFIX := $(out_dir)/
self-coverage: GCOV_PREFIX := ./
self-coverage: coverage

man: $(out_dir)/docs/uncov.1
# the next target doesn't depend on $(wildcard docs/*.md) to make pandoc
# optional
$(out_dir)/docs/uncov.1: force | $(out_dir)/docs
	pandoc -V title=uncov \
	       -V section=1 \
	       -V app=uncov \
	       -V date="$$(date +'%B %d, %Y')" \
	       -V author='xaizek <xaizek@openmailbox.org>' \
	       -s -o $@ $(sort $(wildcard docs/*.md))

# target that doesn't exist and used to force rebuild
force:

reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif

$(bin): | $(out_dirs)

$(bin): $(bin_objects)
	$(CXX) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

check: $(target) $(out_dir)/tests/tests reset-coverage
	@$(out_dir)/tests/tests

install: release
	$(INSTALL) -t $(DESTDIR)/usr/bin/ $(bin) uncov-gcov
	$(INSTALL) -m 644 $(out_dir)/docs/uncov.1 \
	                  $(DESTDIR)/usr/share/man/man1/uncov.1

uninstall:
	$(RM) $(DESTDIR)/usr/bin/$(bin_name) $(DESTDIR)/usr/share/man/man1/uncov.1

# work around parenthesis warning in tests somehow caused by ccache
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses -Itests/
$(out_dir)/tests/tests: $(tests_objects) tests/. | $(out_dirs)
	$(CXX) -o $@ $(tests_objects) $(LDFLAGS) $(EXTRA_LDFLAGS)

$(out_dir)/%.o: %.cpp | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dirs) $(out_dir)/docs:
	mkdir -p $@

clean:
	-$(RM) -r coverage/ debug/ release/
	-$(RM) $(bin_objects) $(bin_depends) $(tests_objects) $(tests_depends) \
	       $(bin) $(out_dir)/tests/tests

include $(wildcard $(bin_depends) $(tests_depends))
