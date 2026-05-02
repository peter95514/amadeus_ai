#include "name.h"

#include <atomic>
// 真正的定義與初始化
std::atomic<unsigned long long> global_tick_counter(0);
