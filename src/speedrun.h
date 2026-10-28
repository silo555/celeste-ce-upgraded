#pragma once
#include <stdint.h>

void speedrun_init();
void speedrun_begin();
void speedrun_tick();
void speedrun_complete(unsigned room, uint32_t elapsed);
void speedrun_practice_complete(unsigned room, uint32_t ticks);
uint32_t speedrun_practice_best(unsigned room);
void speedrun_draw_hud();
void speedrun_flush_pending();
void speedrun_save_records();
void speedrun_load_run(uint8_t handle, unsigned room, uint32_t elapsed, bool practice);
void speedrun_store_run(uint8_t handle);
