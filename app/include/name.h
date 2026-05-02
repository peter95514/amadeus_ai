#pragma once

#ifndef NAME_H
#define NAME_H

#include <sys/types.h>

#include <array>
#include <atomic>
#include <cstdint>
//全域變數
constexpr int NUM_BUCKETS = 16;
constexpr int NEURONS_PER_BUCKET = 64;
constexpr int NUM_NEURONS = NUM_BUCKETS * NEURONS_PER_BUCKET;
constexpr int MASKS_SIZE = NUM_NEURONS * NUM_BUCKETS;  // 1024 * 16 = 16384
extern std::atomic<unsigned long long> global_tick_counter;
struct Packet256 {
    uint64_t blocks[4];
};
struct Pixel {
    uint8_t r, g, b;
};

#endif
