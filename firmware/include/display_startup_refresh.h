#pragma once
#include <stdint.h>

// A bounded, nonblocking retry window. Never reset hardware or learner state.
struct DisplayStartupRefresh {
    uint32_t started = 0;
    uint8_t next = 0;
    bool active = false;

    void begin(uint32_t now) {
        started = now;
        next = 0;
        active = true;
    }

    void cancel() { active = false; }

    bool due(uint32_t now) {
        constexpr uint32_t offsets[] = {2000, 6000, 12000, 24000};
        if (!active || uint32_t(now - started) < offsets[next]) return false;
        if (++next == sizeof(offsets) / sizeof(offsets[0])) active = false;
        return true;
    }
};
