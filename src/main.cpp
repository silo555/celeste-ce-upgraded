#include <keypadc.h>
#include <graphx.h>
#include <tice.h>
#include "emu.h"
#include "speedrun.h"

int main(void)
{
    gfx_Begin();
    init();
    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    do {
        update();
        // Archive records only after finishing, while the run timer is stopped.
        speedrun_flush_pending();
    } while(!kb_IsDown(kb_KeyClear) && !kb_IsDown(kb_KeyDel));
    if(!kb_IsDown(kb_KeyDel)) {
        save_game();
    }
    // Del skips the current-run save, but never discards earned records.
    speedrun_save_records();
    gfx_End();
    return 0;
}
