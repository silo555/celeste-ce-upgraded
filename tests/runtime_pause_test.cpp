// Execute the real emu.cpp control/render loop against two fake LCD buffers.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define LCD_WIDTH 320
#define LCD_HEIGHT 240
#define MODE_3_CONTINUOUS 3
#define profiler_add(x)
#define profiler_end(x)
#define profiler_init()
#define profiler_tick()
#define profiler_print()
enum { kb_KeyDel, kb_KeyYequ, kb_KeyClear, kb_KeyTrace, kb_KeyEnter };
using fontlib_font_t = uint8_t;
using fontlib_load_options_t = unsigned;
static uint8_t buffers[2][240][320];
static unsigned draw_buffer = 1, display_buffer = 0;
static uint8_t (*gfx_vbuffer)[320] = buffers[1];
static bool keys[5], title;
static unsigned updates, draws, practice_ticks, huds, resets, swaps;
static unsigned timer_value, timer_increment = 1092;
static const uint8_t mypalette[32] = {};
static unsigned lcd_Control;
int freeze;
bool practice_mode;
bool kb_IsDown(unsigned k) { return keys[k]; }
void kb_SetMode(unsigned) {}
void gfx_ZeroScreen() { memset(gfx_vbuffer, 0, sizeof buffers[0]); }
void gfx_SetDrawBuffer() {}
void gfx_SetPalette(const void*, unsigned, unsigned) {}
void gfx_SetClipRegion(int,int,int,int) {}
void fontlib_SetFont(const fontlib_font_t*, fontlib_load_options_t) {}
void fontlib_SetTransparency(bool) {}
void gen_lookups() {}
void speedrun_init() {}
void practice_load_bests() {}
uint8_t ti_Open(const char*,const char*) { return 0; }
void ti_Close(uint8_t) {}
void _init(uint8_t) {}
void timer_Set(unsigned,unsigned v) { timer_value = v; }
unsigned timer_Get(unsigned) { return timer_value += timer_increment; }
bool is_title() { return title; }
void reset_run() { ++resets; freeze = 0; title = false; practice_mode = false; }
void _update() { ++updates; if(freeze > 0) --freeze; }
void _draw() { ++draws; memset(gfx_vbuffer, (draws % 100)+1, sizeof buffers[0]); }
void practice_update() { ++practice_ticks; }
void practice_draw_hud() { ++huds; }
void draw_time(int,int) {}
void speedrun_draw_hud() {}
void gfx_BlitScreen() { memcpy(gfx_vbuffer, buffers[display_buffer], sizeof buffers[0]); }
void gfx_SwapDraw() {
    ++swaps;
    unsigned previous_display = display_buffer;
    display_buffer = draw_buffer; draw_buffer = previous_display;
    gfx_vbuffer = buffers[draw_buffer];
}
void camera();
void rectfill(int,int,int,int,uint8_t) { gfx_vbuffer[0][110] = 0; }
void print(const char*,int,int,uint8_t) { gfx_vbuffer[2][110] = 0x77; }
#include "runtime.inc"
void camera() { offset.x = BASE_X; offset.y = BASE_Y; }

int main() {
    (void)pal_map; (void)default_pal; // Used by drawing helpers outside the tested prefix.
    update();
    assert(updates == 1 && draws == 1 && practice_ticks == 1);
    static uint8_t original[240][320];
    memcpy(original, buffers[display_buffer], sizeof original);
    freeze = 6; offset.x = 19; offset.y = 1;
    keys[kb_KeyYequ] = true; update();
    assert(run_controls.paused && updates == 1 && draws == 1 && freeze == 6);
    assert(offset.x == 19 && offset.y == 1);
    assert(memcmp(original, buffers[display_buffer], sizeof original) != 0);
    unsigned pause_swaps = swaps;
    timer_increment = 32768; // Long pauses cannot accumulate catch-up debt.
    for(int i=0; i<1000; ++i) update();
    assert(updates == 1 && draws == 1 && practice_ticks == 1 && huds == 1);
    assert(freeze == 6 && swaps == pause_swaps);
    keys[kb_KeyYequ] = false; update();
    keys[kb_KeyYequ] = true; timer_increment = 1092; update();
    assert(!run_controls.paused && updates == 1 && draws == 1 && freeze == 6);
    assert(memcmp(original, buffers[display_buffer], sizeof original) == 0);
    update(); assert(updates == 2 && freeze == 5 && draws == 1);
    puts("PASS actual runtime freezes updates, draw side effects, HUD, practice clock and hitstop; restores exact frame");

    keys[kb_KeyYequ] = false; update();
    keys[kb_KeyYequ] = true; update(); assert(run_controls.paused);
    keys[kb_KeyDel] = true; update();
    assert(!run_controls.paused && resets == 1 && freeze == 0 && draws == 2);
    update(); assert(resets == 1);
    keys[kb_KeyDel] = false; keys[kb_KeyYequ] = false; update();
    keys[kb_KeyEnter] = true; update(); assert(resets == 1);
    keys[kb_KeyClear] = true; update();
    puts("PASS actual runtime Del resets while paused, held Del does not repeat, Enter does not reset, Clear returns");
}
