#include <random>

#include "name.h"

class CorticalColumn {
private:
    // === 動態狀態緩衝區 (16 * 64 = 1024 bits) ===
    // 初始化為 0
    uint64_t spikes_current[NUM_BUCKETS] = {0};
    uint64_t spikes_next[NUM_BUCKETS] = {0};

    // === 膜電位與動態閾值 (SoA 佈局，極致記憶體連續性) ===
    uint64_t V[NUM_NEURONS];
    uint64_t A[NUM_NEURONS];

    // === 活化遮罩 (Runtime 真正連通的突觸，會隨學習改變) ===
    uint64_t active_E_mask[MASKS_SIZE] = {0};
    uint64_t active_I_mask[MASKS_SIZE] = {0};

    // === 潛在遮罩 (唯讀，定義了墨西哥帽與桶子的物理極限) ===
    uint64_t potential_E_mask[MASKS_SIZE] = {0};
    uint64_t potential_I_mask[MASKS_SIZE] = {0};
    std::mt19937 rng;

    int potential_E_rate = 30;
    int potential_I_rate = 25;

public:
    CorticalColumn();

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
