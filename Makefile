CXXFLAGS += -std=c++14 -Wall -Wextra -Werror -MMD -I$(abspath src)
CXXFLAGS += -Wno-non-template-friend
LDFLAGS  += -g -lsqlite3 -lgit2 -lsource-highlight -lz
LDFLAGS  += -lboost_filesystem -lboost_iostreams -lboost_system

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
            ifneq ($(call pos,show-coverage,$(MAKECMDGOALS)),-1)
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

bin := $(out_dir)/uncover$(bin_suffix)

bin_sources := $(call rwildcard, src/, *.cpp)
bin_objects := $(bin_sources:%.cpp=$(out_dir)/%.o)
bin_depends := $(bin_objects:.o=.d)

tests_sources := $(call rwildcard, tests/, *.cpp)
tests_sources := $(filter-out tests/test-repo/%, $(tests_sources))

tests_objects := $(tests_sources:%.cpp=$(out_dir)/%.o)
tests_objects += $(filter-out %/main.o,$(bin_objects))
tests_depends := $(tests_sources:%.cpp=$(out_dir)/%.d)

out_dirs := $(sort $(dir $(bin_objects) $(tests_objects)))

.PHONY: check clean coverage man show-coverage reset-coverage debug release all
.PHONY: sanitize-basic

all: $(bin)

debug release: $(bin)

sanitize-basic: $(bin)

coverage: check $(bin)
	find $(out_dir)/ -name '*.o' -exec gcov -p {} +
	uncover-gcov --root . --build-root . --no-gcov --exclude tests | \
	    uncover . new
	find . -name '*.gcov' -delete

show-coverage: coverage
	$$BROWSER coverage/data/index.html

reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif

$(bin): | $(out_dirs)

$(bin): $(bin_objects)
	$(CXX) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

check: $(target) $(out_dir)/tests/tests reset-coverage
	@$(out_dir)/tests/tests

# work around parenthesis warning in tests somehow caused by ccache
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses -Itests/
$(out_dir)/tests/tests: $(tests_objects) tests/. | $(out_dirs)
	$(CXX) -o $@ $(tests_objects) $(LDFLAGS) $(EXTRA_LDFLAGS)

$(out_dir)/%.o: %.cpp | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dirs):
	mkdir -p $@

clean:
	-$(RM) -r coverage/ debug/ release/
	-$(RM) $(bin_objects) $(bin_depends) $(tests_objects) $(tests_depends) \
           $(bin) $(out_dir)/tests/tests

include $(wildcard $(bin_depends) $(tests_depends))
