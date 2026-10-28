#include "run_controls.h"
#include "run_records.h"
#include <assert.h>
#include <stdio.h>

int main() {
    RunControls c;
    assert(c.sample(true,true,true) == RunControls::none && !c.paused);
    c.sample(false,false,true);
    assert(c.sample(false,true,true) == RunControls::pause_changed && c.paused);
    for(int i=0;i<100;++i) assert(c.sample(false,true,true) == RunControls::none && c.paused);
    c.sample(false,false,true);
    assert(c.sample(false,true,true) == RunControls::pause_changed && !c.paused);
    puts("PASS launch release latch, Y= pause/resume and one action per press");
    c.sample(false,false,true);
    assert(c.sample(true,true,true) == RunControls::reset && !c.paused);
    assert(c.sample(true,true,true) == RunControls::none);
    c.sample(false,false,true);
    c.sample(false,true,true); assert(c.paused);
    assert(c.sample(true,true,true) == RunControls::reset && !c.paused);
    puts("PASS Del reset priority, no hold-repeat and reset while paused");
    c.sample(false,false,false);
    assert(c.sample(false,true,false) == RunControls::none && !c.paused);
    assert(c.sample(false,true,true) == RunControls::none && !c.paused);
    c.sample(false,false,true);
    speedrun::Tracker tracker; tracker.begin();
    tracker.complete(0,100);
    tracker.records.total=3000;
    tracker.records.splits[1]=200;
    tracker.complete(1,190);
    unsigned original_popup=tracker.popup_ticks;
    c.sample(false,true,true);
    for(int i=0;i<1000;++i) if(!c.paused) tracker.tick();
    assert(tracker.popup_ticks==original_popup);
    c.sample(false,false,true); c.sample(false,true,true);
    if(!c.paused)tracker.tick();
    assert(tracker.popup_ticks+1==original_popup);
    puts("PASS title ignores pause and paused PB popup remains frozen");
}
