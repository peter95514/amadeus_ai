#include <algorithm>
#include <cstdint>
#include <random>

#include "name.h"

#ifdef _MSC_VER
#include <intrin.h>  // MSVC
#define POPCOUNT64(x) __popcnt64(x)
#else
// GCC / Clang
#define POPCOUNT64(x) __builtin_popcountll(x)
#endif

struct TopologyTable {
    // 0: 無連線, 1: 激發區 (dist <= 1), 2: 抑制區 (dist == 2)
    int conn_type[16][16];

    constexpr TopologyTable() : conn_type{} {
        for (int b1 = 0; b1 < 16; b1++) {
            for (int b2 = 0; b2 < 16; b2++) {
                int x1 = b1 % 4, y1 = b1 / 4;
                int x2 = b2 % 4, y2 = b2 / 4;

                // 絕對值相減
                int dx = (x1 > x2) ? x1 - x2 : x2 - x1;
                int dy = (y1 > y2) ? y1 - y2 : y2 - y1;

                // 切比雪夫距離
                int dist = (dx > dy) ? dx : dy;

                if (dist <= 1) {
                    conn_type[b1][b2] = 1;
                } else if (dist == 2) {
                    conn_type[b1][b2] = 2;
                } else {
                    conn_type[b1][b2] = 0;
                }
            }
        }
    }
};

// 實例化這個常數表
constexpr TopologyTable TOPO_LUT;

class CorticalColumn {
private:
    // === 動態狀態緩衝區 (16 * 64 = 1024 bits) ===
    // 初始化為 0
    alignas(64) uint64_t spikes_current[NUM_BUCKETS] = {0};
    alignas(64) uint64_t spikes_next[NUM_BUCKETS] = {0};

    // === 膜電位與動態閾值 (SoA 佈局，極致記憶體連續性) ===
    alignas(64) uint8_t V[NUM_NEURONS];
    int A[NUM_NEURONS];

    // === 活化遮罩 (Runtime 真正連通的突觸，會隨學習改變) ===
    alignas(64) uint64_t active_E_mask[MASKS_SIZE] = {0};
    alignas(64) uint64_t active_I_mask[MASKS_SIZE] = {0};

    // === 潛在遮罩 (唯讀，定義了墨西哥帽與桶子的物理極限) ===
    alignas(64) uint64_t potential_E_mask[MASKS_SIZE] = {0};
    alignas(64) uint64_t potential_I_mask[MASKS_SIZE] = {0};
    std::mt19937 rng;

    int potential_E_rate;
    int potential_I_rate;

    int leak_speed;
    int essential_A_mask;
    int P_of_growth;
    int P_of_death;
    int T_of_leak;
    int one_time_of_V;

    int max_level_of_V = 64;

    const int max_allowed_A = 63 - essential_A_mask;

public:
    CorticalColumn(int potential_E_rate = 10, int potential_I_rate = 2, int leak_speed = 9, int essential_A_mask = 11,
                   int P_of_growth = 5, int P_of_death = 2, int T_of_leak = 72, int one_time_of_V = 5);

private:
    // 將 16 個桶子排成 4x4 的 2D 拓撲
    void initialize_bucket_topology();
    uint64_t generate_random_mask(int rate);
    void initialize_random_connections(int e_rate, int i_rate);

public:
    // 從冷區 (Binary Data) 極速載入記憶體
    void load_from_cold_storage(const void* binary_data);

    // 觸發新突觸生長 (受限於潛在遮罩的防線)
    void try_grow_synapse(int target_neuron, int source_neuron);

    // 執行一次完整的運算步長
    // 傳入 256-bit 輸入封包，回傳 256-bit 輸出封包
    Packet256 tick(const Packet256& input_packet, bool enable_learning = true);
};
