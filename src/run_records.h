#pragma once

// Platform-independent record tracking and save encoding, shared by the game
// and the executable regression tests. Times are always simulation ticks.
#include <stdint.h>
#include <string.h>

namespace speedrun {
constexpr unsigned rooms = 30;
constexpr uint32_t max_ticks = 30UL * 60 * 60 * 24;
constexpr unsigned record_bytes = 4 + 1 + 4 + (1 + rooms * 3) * 4 + 4;
constexpr unsigned resume_bytes = 4 + 1 + 1 + 1 + rooms * 4 + 4;

struct Records {
    uint32_t total;
    uint32_t splits[rooms];
    uint32_t room_best[rooms];
    uint32_t practice_best[rooms];
};

struct Run {
    bool active;
    uint8_t next_room;
    uint32_t splits[rooms];
};

enum Popup { none, delta, room_pb, run_pb };

struct Tracker {
    Records records{};
    Run run{};
    Popup popup = none;
    int32_t difference = 0;
    uint8_t popup_ticks = 0;
    bool dirty = false;

    void begin() {
        memset(&run, 0, sizeof run);
        run.active = true;
        popup = none;
        popup_ticks = 0;
    }

    void tick() {
        if(popup_ticks) --popup_ticks;
    }

    bool complete(unsigned room, uint32_t elapsed) {
        if(!run.active || room >= rooms || room != run.next_room) return false;
        uint32_t previous = room ? run.splits[room - 1] : 0;
        if(elapsed <= previous || elapsed > max_ticks) {
            run.active = false;
            return false;
        }

        uint32_t duration = elapsed - previous;
        bool improved = records.room_best[room] && duration < records.room_best[room];
        if(!records.room_best[room] || improved) {
            records.room_best[room] = duration;
            dirty = true;
        }
        popup = none;
        popup_ticks = 0;
        if(records.total) {
            // Compare with one real completed PB, never a sum of best segments.
            difference = (int32_t)elapsed - (int32_t)records.splits[room];
            popup = delta;
            popup_ticks = 30;
        } else if(improved) {
            popup = room_pb;
            popup_ticks = 30;
        }

        run.splits[room] = elapsed;
        ++run.next_room;
        if(run.next_room != rooms) return false;
        run.active = false;
        if(!records.total || elapsed < records.total) {
            records.total = elapsed;
            memcpy(records.splits, run.splits, sizeof records.splits);
            dirty = true;
            popup = run_pb;
            popup_ticks = 60;
        }
        return true;
    }

    void practice(unsigned room, uint32_t ticks) {
        if(room >= rooms || !ticks || ticks > max_ticks) return;
        uint32_t &best = records.practice_best[room];
        if(!best || ticks < best) {
            best = ticks;
            dirty = true;
        }
    }
};

inline void put32(uint8_t *p, uint32_t value) {
    for(unsigned i = 0; i < 4; ++i) { p[i] = (uint8_t)value; value >>= 8; }
}

inline uint32_t get32(const uint8_t *p) {
    uint32_t value = 0;
    for(unsigned i = 0; i < 4; ++i) value |= (uint32_t)p[i] << (i * 8);
    return value;
}

inline uint32_t checksum(const uint8_t *p, unsigned size) {
    uint32_t hash = 2166136261UL;
    for(unsigned i = 0; i < size; ++i) hash = (hash ^ p[i]) * 16777619UL;
    return hash;
}

inline bool valid_records(const Records &r) {
    if(r.total > max_ticks) return false;
    uint32_t previous = 0;
    for(unsigned i = 0; i < rooms; ++i) {
        if(r.total && (r.splits[i] <= previous || r.splits[i] > r.total)) return false;
        if(!r.total && r.splits[i]) return false;
        if(r.room_best[i] > max_ticks || r.practice_best[i] > max_ticks) return false;
        previous = r.splits[i];
    }
    return !r.total || r.splits[rooms - 1] == r.total;
}

inline void encode_records(const Records &r, uint32_t generation, uint8_t *out) {
    memcpy(out, "CLPB", 4);
    out[4] = 1;
    put32(out + 5, generation);
    put32(out + 9, r.total);
    unsigned pos = 13;
    for(unsigned i = 0; i < rooms; ++i, pos += 4) put32(out + pos, r.splits[i]);
    for(unsigned i = 0; i < rooms; ++i, pos += 4) put32(out + pos, r.room_best[i]);
    for(unsigned i = 0; i < rooms; ++i, pos += 4) put32(out + pos, r.practice_best[i]);
    put32(out + pos, checksum(out, pos));
}

inline bool decode_records(const uint8_t *data, unsigned size, Records &out, uint32_t &generation) {
    if(size != record_bytes || memcmp(data, "CLPB", 4) || data[4] != 1) return false;
    if(get32(data + size - 4) != checksum(data, size - 4)) return false;
    Records candidate{};
    candidate.total = get32(data + 9);
    unsigned pos = 13;
    for(unsigned i = 0; i < rooms; ++i, pos += 4) candidate.splits[i] = get32(data + pos);
    for(unsigned i = 0; i < rooms; ++i, pos += 4) candidate.room_best[i] = get32(data + pos);
    for(unsigned i = 0; i < rooms; ++i, pos += 4) candidate.practice_best[i] = get32(data + pos);
    if(!valid_records(candidate)) return false;
    out = candidate;
    generation = get32(data + 5);
    return true;
}

inline bool newer(uint32_t a, uint32_t b) { return (int32_t)(a - b) > 0; }

inline void encode_run(const Run &run, uint8_t *out) {
    memcpy(out, "CLRN", 4);
    out[4] = 1;
    out[5] = run.active ? 1 : 0;
    out[6] = run.next_room;
    for(unsigned i = 0; i < rooms; ++i) put32(out + 7 + i * 4, run.splits[i]);
    put32(out + resume_bytes - 4, checksum(out, resume_bytes - 4));
}

inline bool decode_run(const uint8_t *data, unsigned size, unsigned room,
                       uint32_t elapsed, Run &out) {
    if(size != resume_bytes || memcmp(data, "CLRN", 4) || data[4] != 1) return false;
    if(get32(data + size - 4) != checksum(data, size - 4)) return false;
    if(data[5] != 1 || room >= rooms || data[6] != room || elapsed > max_ticks) return false;
    Run candidate{};
    candidate.active = true;
    candidate.next_room = data[6];
    uint32_t previous = 0;
    for(unsigned i = 0; i < rooms; ++i) {
        uint32_t split = get32(data + 7 + i * 4);
        if(i < room) {
            if(split <= previous || split > elapsed) return false;
            previous = split;
        } else if(split) return false;
        candidate.splits[i] = split;
    }
    out = candidate;
    return true;
}
} // namespace speedrun
