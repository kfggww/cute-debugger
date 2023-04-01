#include "dbginfo.h"

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

    Dwarf_Error err;
    if (dwarf_init(fd, DW_DLC_READ, NULL, NULL, &im->dbg, &err) != DW_DLV_OK) {
        free(im);
        im = NULL;
    }

end:
    close(fd);
    return im;
}

/*Destroy debug info manager*/
void destroy_info_manager(DebugInfoManager *im) {
    Dwarf_Error err;
    dwarf_finish(im->dbg, &err);
    free(im);
}

void *addr_of_lineno(DebugInfoManager *im, int lineno) { return NULL; }
void *addr_of_function(DebugInfoManager *im, const char *fn) { return NULL; }