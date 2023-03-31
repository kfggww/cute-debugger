PROJECT := qdb
VERSION := 0.0.1

Q :=
SHELL := /bin/bash
CC := gcc

CFLAGS := -g -Ideps/linenoise
LDFLAGS := -ldwarf

QDB_SRCS := main.c debugger.c
QDB_SRCS := $(addprefix src/, $(QDB_SRCS))
QDB_OBJS := $(QDB_SRCS:%.c=%.o)

DEP_PROJECTS := deps/linenoise
LINENOISE_OBJS := deps/linenoise/linenoise.o
QDB_OBJS += $(LINENOISE_OBJS)

define build_deps
	for dep in $(1); \
	do \
		$(MAKE) --no-print-directory -C $${dep}; \
	done;
endef

define clean_deps
	for dep in $(1); \
	do \
		$(MAKE) --no-print-directory -C $${dep} clean; \
	done;
endef

all: deps_build qdb test

deps_build:
	$(Q)$(call build_deps, $(DEP_PROJECTS))

deps_clean:
	$(Q)$(call clean_deps, $(DEP_PROJECTS))

qdb: $(QDB_OBJS)
	$(Q)$(CC) $^ $(LDFLAGS) -o $@

test: src/test.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c -MM $(CFLAGS) $< -MT $@ >> .dep
	$(CC) -c $(CFLAGS) $< -o $@

clean: deps_clean
	$(Q)rm -rf $(QDB_OBJS) qdb .dep test

.PHONY: all deps_build clean
-include *.dep