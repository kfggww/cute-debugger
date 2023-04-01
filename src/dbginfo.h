#ifndef DBGINFO_H
#define DBGINFO_H

#include <stdint.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

typedef struct DebugInfoManager {
    Dwarf_Debug dbg;
    uint16_t elf_type;
} DebugInfoManager;

DebugInfoManager *create_info_manager(const char *tracee_name);
void destroy_info_manager(DebugInfoManager *im);

void *addr_of_lineno(DebugInfoManager *im, int lineno);
void *addr_of_function(DebugInfoManager *im, const char *func);

#endif