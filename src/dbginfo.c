#include "dbginfo.h"

#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*Create debug info manager*/
DebugInfoManager *create_info_manager(const char *tracee_name) {
    DebugInfoManager *im = NULL;

    int fd = open(tracee_name, O_RDONLY);
    if (fd == -1)
        return NULL;

    Elf64_Ehdr elf_hdr;
    ssize_t n = read(fd, &elf_hdr, sizeof(elf_hdr));
    if (n != sizeof(elf_hdr))
        goto end;

    im = malloc(sizeof(*im));
    im->elf_type = elf_hdr.e_type;
    im->elf_fd = fd;

    Dwarf_Error err;
    if (dwarf_init(fd, DW_DLC_READ, NULL, NULL, &im->dbg, &err) != DW_DLV_OK) {
        free(im);
        im = NULL;
    }

end:
    return im;
}

/*Destroy debug info manager*/
void destroy_info_manager(DebugInfoManager *im) {
    Dwarf_Error err;
    dwarf_finish(im->dbg, &err);
    close(im->elf_fd);
    free(im);
}

void *addr_of_lineno(DebugInfoManager *im, int lineno) {
    void *result = NULL;
    Dwarf_Debug dbg = im->dbg;
    Dwarf_Unsigned cu_next = 0;
    Dwarf_Die cu_die = NULL;

    /*Reset iteration*/
    int ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, &cu_next, NULL);
    while (ret == DW_DLV_OK) {
        dwarf_siblingof(dbg, NULL, &cu_die, NULL);
        Dwarf_Line *lines = NULL;
        Dwarf_Signed nlines = 0;
        dwarf_srclines(cu_die, &lines, &nlines, NULL);

        for (int i = 0; i < nlines; i++) {
            Dwarf_Unsigned lineno_cur = 0;
            dwarf_lineno(lines[i], &lineno_cur, NULL);
            if (lineno_cur == lineno) {
                Dwarf_Addr addr = 0;
                dwarf_lineaddr(lines[i], &addr, NULL);
                result = (void *)addr;
                while (dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL,
                                            &cu_next, NULL) == DW_DLV_OK) {
                }
                goto found;
            }
        }

        ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, &cu_next, NULL);
    }

found:
    return result;
}

void *addr_of_function(DebugInfoManager *im, const char *fn) {
    void *result = NULL;
    Dwarf_Debug dbg = im->dbg;

    Dwarf_Unsigned cu_length;
    Dwarf_Half cu_version;
    Dwarf_Off cu_abbrev_offset;
    Dwarf_Half cu_pointer_size;
    Dwarf_Unsigned cu_next;
    Dwarf_Error err;

    Dwarf_Die cu_die = NULL;
    /*For each compile unit*/
    while (dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, &cu_next, &err) ==
           DW_DLV_OK) {
        if (dwarf_siblingof(dbg, cu_die, &cu_die, &err) != DW_DLV_OK) {
            break;
        }

        Dwarf_Die cu_child = NULL;
        if (dwarf_child(cu_die, &cu_child, &err) != DW_DLV_OK) {
            continue;
        }

        while (cu_child) {
            Dwarf_Half tag = 0;
            char *name = NULL;
            dwarf_tag(cu_child, &tag, &err);
            dwarf_diename(cu_child, &name, &err);
            if (tag == DW_TAG_subprogram && !strcmp(name, fn)) {
                Dwarf_Addr addr = 0;
                dwarf_lowpc(cu_child, &addr, &err);
                result = (void *)addr;
                while (dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL,
                                            &cu_next, &err) == DW_DLV_OK) {
                }
                goto found;
            }

            if (dwarf_siblingof(dbg, cu_child, &cu_child, &err) != DW_DLV_OK) {
                cu_child = NULL;
            }
        }
    }

found:
    return result;
}