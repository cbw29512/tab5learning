#include "../include/display_startup_refresh.h"
#include <cassert>
int main() {
    DisplayStartupRefresh r;
    assert(!r.due(0));
    r.begin(100);
    assert(!r.due(2099));
    assert(r.due(2100));
    assert(!r.due(2100));
    assert(r.due(6100));
    assert(r.due(12100));
    assert(r.due(24100));
    assert(!r.due(100000));
    assert(!r.active);
    r.begin(100);
    r.cancel();
    assert(!r.due(25000));
    r.begin(UINT32_MAX-1000);
    assert(!r.due(998));
    assert(r.due(999));
}
