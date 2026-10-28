#include "speedrun.h"
#include "run_records.h"
#include "classic.h"
#include "emu.h"
#include <fileioc.h>

static speedrun::Tracker tracker;
static uint32_t generation;
static uint8_t active_slot;
static bool finish_save_pending;
static bool save_failed;
static const char *const record_names[] = {"CelPbA", "CelPbB"};

void speedrun_init() {
    // Read both journal slots; an interrupted write never replaces the only PB.
    for(uint8_t slot = 0; slot < 2; ++slot) {
        uint8_t file = ti_Open(record_names[slot], "r");
        if(!file) continue;
        uint8_t bytes[speedrun::record_bytes];
        speedrun::Records records;
        uint32_t serial;
        bool valid = ti_GetSize(file) == sizeof bytes &&
                     ti_Read(bytes, sizeof bytes, 1, file) == 1 &&
                     speedrun::decode_records(bytes, sizeof bytes, records, serial);
        ti_Close(file);
        if(valid && (!generation || speedrun::newer(serial, generation))) {
            tracker.records = records;
            generation = serial;
            active_slot = slot;
        }
    }
}

void speedrun_begin() {
    tracker.begin();
    save_failed = false;
}

void speedrun_tick() { tracker.tick(); }

void speedrun_complete(unsigned room, uint32_t elapsed) {
    if(tracker.complete(room, elapsed)) finish_save_pending = true;
}

void speedrun_practice_complete(unsigned room, uint32_t ticks) {
    tracker.practice(room, ticks);
}

uint32_t speedrun_practice_best(unsigned room) {
    return room < speedrun::rooms ? tracker.records.practice_best[room] : 0;
}

static void print_ticks(uint32_t ticks, int x, int y, uint8_t col, bool compact) {
    uint32_t whole = ticks / 30;
    uint32_t mins = whole / 60;
    if(!compact || mins) {
        print_int((int)mins, x, y, col, compact ? 1 : 2);
        print(":");
        print_int((int)(whole % 60), 2);
    } else {
        print_int((int)whole, x, y, col);
    }
    print(".");
    print_int((int)(ticks % 30) * 1000 / 30, 3);
}

void speedrun_draw_hud() {
    // Timer is row one; this small second row appears only for a split or finish.
    if(!is_title() && tracker.popup_ticks) {
        if(tracker.popup == speedrun::delta) {
            int32_t diff = tracker.difference;
            uint32_t ticks = (uint32_t)(diff < 0 ? -diff : diff);
            uint32_t seconds = ticks / 30;
            unsigned chars = seconds < 10 ? 6 : seconds < 60 ? 7 : 9;
            for(uint32_t mins = seconds / 60; mins >= 10; mins /= 10) ++chars;
            rectfill(4, 12, 5 + (int)chars * 4, 18, 0);
            uint8_t col = diff < 0 ? 11 : diff > 0 ? 8 : 7;
            print(diff < 0 ? "-" : "+", 5, 13, col);
            print_ticks(ticks, 9, 13, col, true);
        } else {
            rectfill(4, 12, tracker.popup == speedrun::run_pb ? 33 : 37, 18, 0);
            print(tracker.popup == speedrun::run_pb ? "new pb!" : "room pb!", 5, 13, 11);
        }
    } else if(level_index() == 30 && tracker.records.total) {
        rectfill(4, 12, 55, 18, 0);
        print("pb", 5, 13, 6);
        print_ticks(tracker.records.total, 17, 13, 6, false);
    }
    if(is_title() && tracker.records.total) {
        rectfill(4, 115, 59, 121, 0);
        print("pb", 5, 116, 11);
        print_ticks(tracker.records.total, 17, 116, 7, false);
    }
    if(save_failed && level_index() == 30) {
        rectfill(4, 20, 61, 26, 0);
        print("pb save failed", 5, 21, 8);
    }
}

void speedrun_save_records() {
    if(!tracker.dirty) return;
    uint8_t next_slot = 1 - active_slot;
    uint8_t bytes[speedrun::record_bytes];
    // Serial zero is reserved for no previously loaded slot.
    uint32_t serial = generation + 1;
    if(!serial) serial = 1;
    speedrun::encode_records(tracker.records, serial, bytes);
    uint8_t file = ti_Open(record_names[next_slot], "w");
    if(!file) { save_failed = true; return; }
    bool saved = ti_Write(bytes, sizeof bytes, 1, file) == 1;
    if(saved) saved = ti_SetArchiveStatus(true, file) != 0;
    ti_Close(file);
    save_failed = !saved;
    if(saved) {
        generation = serial;
        active_slot = next_slot;
        tracker.dirty = false;
    }
}

void speedrun_flush_pending() {
    if(!finish_save_pending) return;
    finish_save_pending = false;
    speedrun_save_records();
}

void speedrun_load_run(uint8_t handle, unsigned room, uint32_t elapsed, bool practice) {
    tracker.run = {};
    if(practice) return;
    uint8_t bytes[speedrun::resume_bytes];
    if(ti_Read(bytes, sizeof bytes, 1, handle) == 1) {
        speedrun::decode_run(bytes, sizeof bytes, room, elapsed, tracker.run);
    }
    // Legacy saves remain playable, but lack the split history for a valid PB.
}

void speedrun_store_run(uint8_t handle) {
    uint8_t bytes[speedrun::resume_bytes];
    speedrun::encode_run(tracker.run, bytes);
    ti_Write(bytes, sizeof bytes, 1, handle);
}
