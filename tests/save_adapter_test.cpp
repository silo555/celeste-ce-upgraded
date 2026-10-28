// Host integration test: actual speedrun.cpp with a fake TI AppVar filesystem.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

struct Var { std::vector<uint8_t> bytes; bool archived = false; };
struct Handle { std::string name; size_t position; };
static std::map<std::string, Var> vars;
static std::map<uint8_t, Handle> handles;
static bool fail_open, fail_write, fail_archive;
static unsigned write_calls, archive_calls;
static uint8_t next_handle = 1;
static int test_level = 31;
static std::string text;
static uint8_t text_color;

uint8_t ti_Open(const char *name, const char *mode) {
    if(mode[0] == 'w') {
        if(fail_open) return 0;
        vars[name] = Var{};
    } else if(!vars.count(name)) return 0;
    uint8_t id = next_handle++;
    if(!next_handle) next_handle = 1;
    assert(!handles.count(id));
    handles[id] = {name, 0};
    return id;
}
int ti_Close(uint8_t id) { return (int)handles.erase(id); }
uint16_t ti_GetSize(uint8_t id) { return (uint16_t)vars[handles.at(id).name].bytes.size(); }
size_t ti_Read(void *data, size_t size, size_t count, uint8_t id) {
    Handle &h = handles.at(id);
    const auto &bytes = vars.at(h.name).bytes;
    size_t available = h.position <= bytes.size() ? bytes.size() - h.position : 0;
    size_t items = std::min(count, available / size);
    memcpy(data, bytes.data() + h.position, items * size);
    h.position += items * size;
    return items;
}
size_t ti_Write(const void *data, size_t size, size_t count, uint8_t id) {
    ++write_calls;
    Handle &h = handles.at(id);
    auto &bytes = vars[h.name].bytes;
    size_t length = fail_write ? size * count / 2 : size * count;
    bytes.resize(h.position + length);
    memcpy(bytes.data() + h.position, data, length);
    h.position += length;
    return fail_write ? 0 : count;
}
int ti_SetArchiveStatus(bool archive, uint8_t id) {
    ++archive_calls;
    if(fail_archive) return 0;
    vars[handles.at(id).name].archived = archive;
    return 1;
}
bool is_title() { return test_level == 31; }
int level_index() { return test_level; }
void rectfill(int x0, int y0, int x1, int y1, uint8_t) {
    assert(x0 >= 0 && x1 <= 128 && y0 >= 0 && y1 <= 128);
}
void print(const char *s) { text += s; }
void print(const char *s, int, int, uint8_t col) { text_color = col; text += s; }
void print_int(int n, int length) {
    char buf[32]; snprintf(buf, sizeof buf, "%0*d", length, n); text += buf;
}
void print_int(int n, int, int, uint8_t col, int length) { text_color = col; print_int(n, length); }
void print_int(int n, int x, int y, uint8_t col) { print_int(n, x, y, col, 1); }

#include "speedrun.cpp"

static void relaunch(bool ram_reset = false) {
    assert(handles.empty());
    if(ram_reset) {
        for(auto i = vars.begin(); i != vars.end();) {
            if(!i->second.archived) i = vars.erase(i); else ++i;
        }
    }
    tracker = speedrun::Tracker{};
    generation = 0;
    active_slot = 0;
    finish_save_pending = false;
    save_failed = false;
    speedrun_init();
    assert(handles.empty());
}
static void finish(unsigned duration) {
    speedrun_begin();
    for(unsigned i = 0; i < 30; ++i) speedrun_complete(i, (i + 1) * duration);
}

int main() {
    speedrun_init();
    speedrun_begin();
    speedrun_complete(0, 100);
    speedrun_flush_pending();
    assert(write_calls == 0 && archive_calls == 0);
    for(unsigned i = 1; i < 30; ++i) speedrun_complete(i, (i + 1) * 100);
    assert(write_calls == 0);
    speedrun_flush_pending();
    assert(write_calls == 1 && archive_calls == 1 && vars.at("CelPbB").archived);
    relaunch(true);
    assert(tracker.records.total == 3000 && generation == 1);
    puts("PASS actual adapter defers flash writes until finish and reloads archived PB after RAM reset");

    speedrun_begin(); speedrun_complete(0, 85);
    test_level = 1; text.clear(); speedrun_draw_hud();
    assert(text == "-0.500" && text_color == 11);
    speedrun_begin(); speedrun_complete(0, 137);
    text.clear(); speedrun_draw_hud();
    assert(text == "+1.233" && text_color == 8);
    for(unsigned i = 0; i < 30; ++i) speedrun_tick();
    text.clear(); speedrun_draw_hud(); assert(text.empty());
    test_level = 31; text.clear(); speedrun_draw_hud();
    assert(text == "pb01:40.000");
    puts("PASS actual HUD formats green/red deltas, expires after 30 ticks, and shows title PB");

    finish(90); speedrun_flush_pending();
    assert(vars.at("CelPbA").archived && generation == 2);
    auto saved = vars.at("CelPbA");
    vars.at("CelPbA").bytes[20] ^= 1;
    relaunch(true); assert(tracker.records.total == 3000);
    vars["CelPbA"] = saved;
    relaunch(true); assert(tracker.records.total == 2700);
    puts("PASS alternating archives and recovery from a corrupted newest slot");

    finish(80); fail_write = true; speedrun_flush_pending(); fail_write = false;
    assert(save_failed && tracker.dirty);
    relaunch(true); assert(tracker.records.total == 2700);
    finish(80); fail_archive = true; speedrun_flush_pending(); fail_archive = false;
    assert(save_failed && tracker.dirty);
    relaunch(true); assert(tracker.records.total == 2700);
    finish(80); fail_open = true; speedrun_flush_pending(); fail_open = false;
    assert(save_failed && tracker.dirty);
    speedrun_save_records();
    relaunch(true); assert(tracker.records.total == 2400);
    puts("PASS partial-write, archive and open failures preserve previous PB; retry persists new PB");

    speedrun_practice_complete(3, 42);
    assert(speedrun_practice_best(3) == 42);
    speedrun_save_records(); relaunch(true);
    assert(speedrun_practice_best(3) == 42 && tracker.records.total == 2400);
    puts("PASS practice PB persists independently through quit/relaunch");

    speedrun_begin();
    for(unsigned i = 0; i < 5; ++i) speedrun_complete(i, (i + 1) * 60);
    uint8_t file = ti_Open("CelesteS", "w");
    uint8_t prefix[13] = {};
    ti_Write(prefix, sizeof prefix, 1, file);
    speedrun_store_run(file); ti_Close(file);
    file = ti_Open("CelesteS", "r"); handles.at(file).position = 13;
    speedrun_load_run(file, 5, 320, false); ti_Close(file);
    assert(tracker.run.active && tracker.run.next_room == 5 && tracker.run.splits[4] == 300);
    file = ti_Open("CelesteS", "r"); handles.at(file).position = 13;
    speedrun_load_run(file, 5, 320, true); ti_Close(file);
    assert(!tracker.run.active);
    vars["Legacy"].bytes.resize(13);
    file = ti_Open("Legacy", "r"); handles.at(file).position = 13;
    speedrun_load_run(file, 5, 320, false); ti_Close(file);
    assert(!tracker.run.active);
    puts("PASS actual AppVar resume extension, practice exclusion and legacy-save handling");
    assert(handles.empty());
    puts("ALL SAVE/HUD ADAPTER TESTS PASSED");
}
