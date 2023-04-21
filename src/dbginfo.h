#ifndef DBGINFO_H
#define DBGINFO_H

#include <stdint.h>
#include <dwarf.h>
#include <libdwarf.h>

typedef struct DebugInfoManager {
    Dwarf_Debug dbg;
    uint16_t elf_type;
    int elf_fd;
} DebugInfoManager;

DebugInfoManager *create_info_manager(const char *tracee_name);
void destroy_info_manager(DebugInfoManager *im);

void *addr_of_lineno(DebugInfoManager *im, int lineno);
void *addr_of_function(DebugInfoManager *im, const char *func);

#endif