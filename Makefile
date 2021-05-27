CXXFLAGS += -std=gnu++11 -Wall -Wextra -Werror -MMD -MP -I$(abspath src)
CXXFLAGS += -Wno-non-template-friend -include config.h
LDFLAGS  += -g -lsqlite3 -lgit2 -lsource-highlight -lz
LDFLAGS  += -lboost_filesystem -lboost_iostreams -lboost_program_options
LDFLAGS  += -lboost_system

INSTALL := install -D

PREFIX := /usr

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

bin := $(out_dir)/uncov$(bin_suffix)

bin_sources := $(call rwildcard, src/, *.cpp)
bin_objects := $(bin_sources:%.cpp=$(out_dir)/%.o)
bin_depends := $(bin_objects:.o=.d)

tests_sources := $(call rwildcard, tests/, *.cpp)
tests_sources := $(filter-out tests/test-repo%, $(tests_sources))

tests_objects := $(tests_sources:%.cpp=$(out_dir)/%.o)
tests_objects += $(filter-out %/main.o,$(bin_objects))
tests_depends := $(tests_objects:%.o=%.d)

webbin := $(out_dir)/uncov-web$(bin_suffix)
web_sources := $(call rwildcard, web/, *.cpp)
web_temps := $(patsubst %.ecpp,$(out_dir)/%.cpp,$(call rwildcard, web/, *.ecpp))
web_temps += $(patsubst %.css,$(out_dir)/%.cpp,$(call rwildcard, web/, *.css))
web_temps += $(patsubst %.ico,$(out_dir)/%.cpp,$(call rwildcard, web/, *.ico))
web_temps += $(patsubst %.txt,$(out_dir)/%.cpp,$(call rwildcard, web/, *.txt))
web_objects := $(web_sources:%.cpp=$(out_dir)/%.o)
web_objects += $(web_temps:%.cpp=%.o)
web_objects += $(filter-out %/main.o,$(bin_objects))
web_depends := $(web_objects:%.o=%.d)
web_depends += $(patsubst %.ecpp,$(out_dir)/%.ecpp.d, \
                          $(call rwildcard, web/, *.ecpp))

out_dirs := $(sort $(dir $(bin_objects) $(web_objects) $(tests_objects)))

.PHONY: all check clean debug release sanitize-basic install uninstall
.PHONY: man doxygen
.PHONY: coverage self-coverage self-coverage-release reset-coverage

all: $(bin) $(webbin)

debug release sanitize-basic: all

coverage: check $(bin)
	uncov new-gcovi --exclude tests/ --exclude web/ \
	                --capture-worktree $(out_dir)

self-coverage: check self-coverage-release
	release/uncov new-gcovi --exclude tests/ --exclude web/ \
	                        --capture-worktree $(out_dir)

self-coverage-release:
	+$(MAKE) release

man: docs/uncov.1 docs/uncov-gcov.1 docs/uncov-web.1
# the following targets don't depend on $(wildcard docs/*/*.md) to make pandoc
# optional
docs/uncov.1: force
	pandoc -V title=uncov \
	       -V section=1 \
	       -V app=uncov \
	       -V footer="uncov v0.3" \
	       -V date="$$(date +'%B %d, %Y')" \
	       -V author='xaizek <xaizek@posteo.net>' \
	       -s -o $@ $(sort $(wildcard docs/uncov/*.md))
docs/uncov-gcov.1: force
	pandoc -V title=uncov-gcov \
	       -V section=1 \
	       -V app=uncov-gcov \
	       -V footer="uncov v0.3" \
	       -V date="$$(date +'%B %d, %Y')" \
	       -V author='xaizek <xaizek@posteo.net>' \
	       -s -o $@ $(sort $(wildcard docs/uncov-gcov/*.md))
docs/uncov-web.1: force
	pandoc -V title=uncov-web \
	       -V section=1 \
	       -V app=uncov-web \
	       -V footer="uncov v0.3" \
	       -V date="$$(date +'%B %d, %Y')" \
	       -V author='xaizek <xaizek@posteo.net>' \
	       -s -o $@ $(sort $(wildcard docs/uncov-web/*.md))

doxygen:
	doxygen doxygen/config
	ln -sr data doxygen/html/

# target that doesn't exist and used to force rebuild
force:

reset-coverage: | $(out_dirs)
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif

$(bin) $(webbin): | $(out_dirs)

$(bin): $(bin_objects)
	$(CXX) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

ifndef NO-WEB
$(webbin): $(web_objects) | $(web_temps)
	$(CXX) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS) -ltntnet -lcxxtools
else
$(webbin):
	@echo '#!/bin/bash' >> $@
	@echo 'echo "Uncov-web was disabled during build"' >> $@
	@chmod +x $@
endif

check: $(target) $(out_dir)/tests/tests tests/test-repo-gcno/test-repo-gcno \
       reset-coverage
	@$(out_dir)/tests/tests

tests/test-repo-gcno/test-repo-gcno: tests/test-repo-gcno/main.cpp
	cd tests/test-repo-gcno/ && $(CXX) --coverage -o test-repo-gcno main.cpp

install: release
	$(INSTALL) -t $(DESTDIR)$(PREFIX)/bin/ $(bin) $(webbin) uncov-gcov
	$(INSTALL) -t $(DESTDIR)$(PREFIX)/share/uncov/srchilight/ data/srchilight/*
	$(INSTALL) -m 644 docs/uncov.1 $(DESTDIR)$(PREFIX)/share/man/man1/uncov.1
	$(INSTALL) -m 644 docs/uncov-gcov.1 \
	              $(DESTDIR)$(PREFIX)/share/man/man1/uncov-gcov.1
	$(INSTALL) -m 644 docs/uncov-web.1 $(DESTDIR)$(PREFIX)/share/man/man1/uncov-web.1

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(basename $(bin)) \
	      $(DESTDIR)$(PREFIX)/bin/$(basename $(webbin)) \
	      $(DESTDIR)$(PREFIX)/bin/uncov-gcov $(DESTDIR)$(PREFIX)/share/man/man1/uncov.1 \
	      $(DESTDIR)$(PREFIX)/share/man/man1/uncov-gcov.1 \
	      $(DESTDIR)$(PREFIX)/share/man/man1/uncov-web.1
	$(RM) -r $(DESTDIR)$(PREFIX)/share/uncov/

# work around parenthesis warning in tests somehow caused by ccache
$(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses -Itests/
$(out_dir)/tests/tests: $(tests_objects) tests/. | $(out_dirs)
	$(CXX) -o $@ $(tests_objects) $(LDFLAGS) $(EXTRA_LDFLAGS)

config.h: | config.h.in
	cp config.h.in $@

$(out_dir)/web/%.o: CXXFLAGS += -I$(abspath web)
$(out_dir)/web/%.o: $(out_dir)/web/%.cpp config.h | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dir)/%.o: %.cpp config.h | $(out_dirs)
	$(CXX) -o $@ -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $<

$(out_dir)/%.cpp: %.ecpp | $(out_dirs)
	ecppc -o $@ $<
	ecppc -M -o $(out_dir)/$*.ecpp.d -n $* $<

$(out_dir)/%.cpp: %.css | $(out_dirs)
	ecppc -b -m text/css -o $@ $<

$(out_dir)/%.cpp: %.ico | $(out_dirs)
	ecppc -b -m image/x-icon -o $@ $<

$(out_dir)/%.cpp: %.txt | $(out_dirs)
	ecppc -b -m text/plain -o $@ $<

$(out_dirs) $(out_dir)/docs:
	mkdir -p $@

clean:
	-$(RM) -r coverage/ debug/ release/
	-$(RM) $(bin_objects) $(tests_objects) $(web_objects) \
	       $(bin_depends) $(tests_depends) $(web_depends) \
	       $(web_temps) \
	       $(bin) $(webbin) $(out_dir)/tests/tests \
	       tests/test-repo-gcno/test-repo-gcno \
	       tests/test-repo-gcno/main.gcda tests/test-repo-gcno/main.gcno

include $(wildcard $(bin_depends) $(tests_depends) $(web_depends))
