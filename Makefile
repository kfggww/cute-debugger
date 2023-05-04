PROJECT := qdb
VERSION := 0.0.2

BUILD_DIR ?= build
BUILD_TYPE ?= Debug

CFLAGS :=
LDFLAGS :=

ifeq ($(BUILD_TYPE), Debug)
	CFLAGS += -g
endif

qdb_srcs := breakpoint.c command.c dbginfodb.c debugger.c main.c tracee.c
qdb_srcs := $(qdb_srcs:%=src/%)
qdb_objs := $(qdb_srcs:%.c=$(BUILD_DIR)/%.o)
qdb_deps := $(qdb_objs:%.o=%.d)

all: qdb

qdb: $(qdb_objs)
	$(CC) $(LDFLAGS) $^ -o $(BUILD_DIR)/$@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -MM -MT $@ $< > $(@:%.o=%.d)
	$(CC) -c $(CFLAGS) -o $@ $<

debug: qdb
	gdb --args $(BUILD_DIR)/$<

run: qdb
	$(BUILD_DIR)/$<

clean:
	rm -rf $(qdb_objs) $(qdb_deps) $(BUILD_DIR)/qdb

.PHONY: all qdb debug run clean

-include $(qdb_deps)