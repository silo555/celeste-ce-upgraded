#include "run_records.h"
#include <assert.h>
#include <stdio.h>

using namespace speedrun;

static void finish(Tracker &t, unsigned per_room) {
    t.begin();
    for(unsigned i = 0; i < rooms; ++i) assert(t.complete(i, (i + 1) * per_room) == (i == rooms - 1));
}

int main() {
    Tracker t;
    finish(t, 100);
    assert(t.records.total == 3000 && t.popup == run_pb && !t.run.active);
    for(unsigned i = 0; i < rooms; ++i) assert(t.records.splits[i] == (i + 1) * 100);
    puts("PASS first completed run establishes PB and coherent split reference");

    t.begin();
    t.complete(0, 110);
    assert(t.popup == delta && t.difference == 10 && t.popup_ticks == 30);
    for(unsigned i = 0; i < 29; ++i) t.tick();
    assert(t.popup_ticks == 1);
    t.tick(); t.tick();
    assert(t.popup_ticks == 0);
    t.complete(1, 190);
    assert(t.difference == -10 && t.records.room_best[1] == 80);
    t.complete(2, 300);
    assert(t.difference == 0);
    for(unsigned i = 3; i < rooms; ++i) t.complete(i, (i + 1) * 110);
    assert(t.records.total == 3000 && t.records.splits[1] == 200);
    assert(t.records.room_best[1] == 80);
    puts("PASS ahead/behind/tied deltas, one-second popup, and slower run preserves PB splits");

    finish(t, 90);
    assert(t.records.total == 2700 && t.records.splits[1] == 180);
    assert(t.records.room_best[1] == 80);
    finish(t, 90);
    assert(t.popup == delta && t.difference == 0);
    puts("PASS faster PB replaces whole run; ties do not create false new PBs");

    t.begin(); t.complete(0, 80); t.begin();
    assert(t.records.total == 2700 && t.run.next_room == 0 && !t.run.splits[0] && !t.popup_ticks);
    assert(!t.complete(29, 50) && t.records.total == 2700);
    t.practice(4, 50); t.practice(4, 60); t.practice(4, 0); t.practice(30, 1);
    assert(t.records.practice_best[4] == 50 && t.records.total == 2700);
    puts("PASS reset preserves records; skipped rooms/practice cannot award a full-run PB");

    uint8_t encoded[record_bytes];
    encode_records(t.records, 42, encoded);
    Records restored{};
    uint32_t serial = 0;
    assert(decode_records(encoded, sizeof encoded, restored, serial) && serial == 42);
    assert(restored.total == 2700 && restored.room_best[1] == 80 && restored.practice_best[4] == 50);
    for(unsigned byte = 0; byte < record_bytes; ++byte) {
        for(unsigned bit = 0; bit < 8; ++bit) {
            encoded[byte] ^= (1u << bit);
            assert(!decode_records(encoded, sizeof encoded, restored, serial));
            assert(restored.total == 2700 && serial == 42);
            encoded[byte] ^= (1u << bit);
        }
        assert(!decode_records(encoded, byte, restored, serial));
    }
    put32(encoded + 13 + 4, 1);
    put32(encoded + record_bytes - 4, checksum(encoded, record_bytes - 4));
    assert(!decode_records(encoded, sizeof encoded, restored, serial));
    assert(newer(1, 0xffffffffUL) && !newer(5, 6));
    puts("PASS durable record roundtrip, all single-bit corruption/truncation, invalid splits and journal serial wrap");

    t.begin();
    for(unsigned i = 0; i < 4; ++i) t.complete(i, (i + 1) * 70);
    uint8_t resume[resume_bytes];
    encode_run(t.run, resume);
    Run loaded{};
    assert(decode_run(resume, sizeof resume, 4, 300, loaded));
    assert(loaded.active && loaded.next_room == 4 && loaded.splits[3] == 280);
    Tracker resumed;
    resumed.records = t.records;
    resumed.run = loaded;
    for(unsigned i = 4; i < rooms; ++i) resumed.complete(i, (i + 1) * 70);
    assert(resumed.records.total == 2100);
    assert(!decode_run(resume, 0, 4, 300, loaded));
    assert(!decode_run(resume, sizeof resume, 5, 300, loaded));
    assert(!decode_run(resume, sizeof resume, 4, 270, loaded));
    for(unsigned i = 0; i < resume_bytes; ++i) {
        resume[i] ^= 1;
        assert(!decode_run(resume, sizeof resume, 4, 300, loaded));
        resume[i] ^= 1;
    }
    puts("PASS mid-run save/resume, full finish after resume, legacy/corrupt/mismatched run rejection");
    puts("ALL RECORD TESTS PASSED");
}
