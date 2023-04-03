#include "breakpoint.h"
#include <stdio.h>

void show_breakpoint_info(BreakPoint *bpt, int id) {
    if (bpt->state == BREAKPOINT_UNUSED) {
        return;
    } else if (bpt->state == BREAKPOINT_ENABLED) {
        printf("%d: breakpoint enabled, address: %p, hit %d times\n", id,
               bpt->addr, bpt->hits);
    } else {
        printf("%d: breakpoint disabled, address %p, hit %d times\n", id,
               bpt->addr, bpt->hits);
    }
}