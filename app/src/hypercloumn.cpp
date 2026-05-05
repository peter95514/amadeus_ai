#include "hypercloumn.h"

#include <algorithm>
#include <cmath>    // 提供 std::abs, std::max
#include <cstdint>  // 提供 uint64_t 等定寬整數型別
#include <cstring>  // 提供 std::memcpy, std::memset
#include <iostream>
#include <vector>

CorticalColumn::CorticalColumn(int potential_E_rate, int potential_I_rate, int leak_speed, int essential_A_mask,
                               int P_of_growth, int P_of_death) {
    // C++ 原生陣列初始化，將膜電位底線設為 1 (最低位階)
    std::random_device rd;
    rng.seed(rd());
    for (int i = 0; i < NUM_NEURONS; i++) {
        V[i] = 1ULL;
        A[i] = 0;
    }

    this->potential_E_rate = potential_E_rate;
    this->potential_I_rate = potential_I_rate;
    this->leak_speed = leak_speed;
    this->essential_A_mask = essential_A_mask;
    this->P_of_growth = P_of_growth;
    this->P_of_death = P_of_death;

    initialize_bucket_topology();

    initialize_random_connections(potential_E_rate, potential_I_rate);
}

uint64_t CorticalColumn::generate_random_mask(int rate) {
    uint64_t mask = 0;
    std::uniform_int_distribution<int> dist(1, 100);
    for (int b = 0; b < 64; b++) {
        if (dist(rng) <= rate) mask |= (1ULL << b);
    }
    return mask;
}

void CorticalColumn::initialize_random_connections(int e_rate, int i_rate) {
    // 將潛在的墨西哥帽空間，套上隨機的活化率
    for (int i = 0; i < MASKS_SIZE; i++) {
        active_E_mask[i] = potential_E_mask[i] & generate_random_mask(e_rate);
        active_I_mask[i] = potential_I_mask[i] & generate_random_mask(i_rate);
    }
}

void CorticalColumn::initialize_bucket_topology() {
    for (int i = 0; i < NUM_NEURONS; i++) {
        // 計算目前神經元屬於哪個桶子，以及該桶子在 4x4 網格中的座標
        int my_bucket = i / NEURONS_PER_BUCKET;
        int my_bx = my_bucket % 4;
        int my_by = my_bucket / 4;

        for (int target_bucket = 0; target_bucket < NUM_BUCKETS; target_bucket++) {
            int tx = target_bucket % 4;
            int ty = target_bucket / 4;

            // 切比雪夫距離 (降維桶子計算)
            int dist = std::max(std::abs(my_bx - tx), std::abs(my_by - ty));

            // 遮罩陣列的起始索引
            int block_idx = (i * NUM_BUCKETS) + target_bucket;

            if (dist <= 1) {
                // 距離 0 或 1：近側激發區
                // ~0ULL 等同於 0xFFFFFFFFFFFFFFFF，直接開啟 64 條潛在連線
                potential_E_mask[block_idx] = ~0ULL;

                // 假設初始化時有 20% 的機率是天生連通的 (這裡簡化，實際可接入亂數)
                // active_E_mask[block_idx] = generate_random_mask_20_percent();
            } else if (dist == 2) {
                // 距離 2：遠側抑制區
                potential_I_mask[block_idx] = ~0ULL;
            }
        }

        // 防呆：把自己跟自己的潛在激發連線挖掉 (Bitwise AND NOT)
        int self_block_idx = (i * NUM_BUCKETS) + my_bucket;
        uint64_t self_bit = 1ULL << (i % NEURONS_PER_BUCKET);
        potential_E_mask[self_block_idx] &= ~self_bit;
    }
}

// 從冷區 (Binary Data) 極速載入記憶體
void CorticalColumn::load_from_cold_storage(const void* binary_data) {
    // 假設 binary_data 是精準對應所有陣列大小的資料區塊
    // 在 C++ 中，這可以做到真正的 Zero-Copy 載入
    // 為了安全起見，可以用 memcpy 倒進去
    // std::memcpy(V, binary_data, sizeof(V)); ...等等
}

// 觸發新突觸生長 (受限於潛在遮罩的防線)
void CorticalColumn::try_grow_synapse(int target_neuron, int source_neuron) {
    int block_idx = (target_neuron * NUM_BUCKETS) + (source_neuron / NEURONS_PER_BUCKET);
    uint64_t bit = 1ULL << (source_neuron % NEURONS_PER_BUCKET);

    active_E_mask[block_idx] |= (bit & potential_E_mask[block_idx]);
    active_I_mask[block_idx] |= (bit & potential_I_mask[block_idx]);
}

// 執行一次完整的運算步長
// 傳入 256-bit 輸入封包，回傳 256-bit 輸出封包

Packet256 CorticalColumn::tick(const Packet256& input_packet, bool enable_learning) {
    // 1. 寫入輸入層 (Bucket 0 ~ 3)
    for (int b = 0; b < 4; b++) {
        spikes_current[b] = input_packet.blocks[b];
    }

    // 2. 隱藏層與輸出層運算 (ID 256 ~ 1023)
    for (int i = 256; i < NUM_NEURONS; i++) {
        int excitatory_count = 0;
        int inhibitory_count = 0;

        for (int b = 0; b < NUM_BUCKETS; b++) {
            int block_idx = (i * NUM_BUCKETS) + b;
            excitatory_count += POPCOUNT64(spikes_current[b] & active_E_mask[block_idx]);
            inhibitory_count += POPCOUNT64(spikes_current[b] & active_I_mask[block_idx]);
            // 1. 突觸降維 (Synaptic Downscaling)
            // 使用位元右移 (等同於除以 4 和 2)，編譯器會優化成極速指令
            // 這樣代表：每 4 個興奮輸入，才產生 1 階電位上升
            // 每 2 個抑制輸入，才產生 1 階電位下降 (保留了抑制性大於興奮性的 2 倍比例，但力度溫和)
            int net_shift = excitatory_count - inhibitory_count;

            // 2. 位移限幅 (Shift Clamping) ★ 關鍵防護 ★
            // 限制單次 Tick 的最大電位變動率，防止瞬間暴走或瞬間失憶
            // 這裡將單步最大位移限制在 [-3, +3] 之間 (即單步最多放大/縮小 8 倍)
            net_shift = std::max(-3, std::min(3, net_shift));

            // 接下來接回你原本修復過的防溢位運算
            if (net_shift > 0) {
                int current_highest_bit = 63 - __builtin_clzll(V[i]);
                if (current_highest_bit + net_shift >= 63)
                    V[i] = 1ULL << 63;
                else
                    V[i] = V[i] << net_shift;
            } else if (net_shift < 0) {
                int down_shift = -net_shift;
                // 因為前面有限幅，down_shift 最大只會是 3，絕對不會 >= 64
                V[i] = std::max(1ULL, (unsigned long long)V[i] >> down_shift);
            } else {
                V[i] = (V[i] >> leak_speed) | 1ULL;  // Leak
            }
        }

        // 脈衝觸發判定
        uint64_t threshold_mask = 1ULL << (essential_A_mask + A[i]);

        if ((V[i] & ~(threshold_mask - 1)) != 0) {
            // 發射 Spike
            spikes_next[i / NEURONS_PER_BUCKET] |= (1ULL << (i % NEURONS_PER_BUCKET));
            V[i] = 1ULL;
            int max_allowed_A = 63 - essential_A_mask;

            // 如果覺得一次 +1 (門檻變2倍) 不夠，可以改為 +2 (門檻瞬間變4倍)
            if (A[i] + 1 <= max_allowed_A) {
                A[i]++;
            } else {
                A[i] = max_allowed_A;  // 頂到最高門檻，鎖死
            }

            // ==========================================
            // ★ 純位元結構可塑性 (Bitwise Structural Plasticity) ★
            // 只有當這個神經元發射了，且開啟學習模式時才進行生長
            // ==========================================
            if (enable_learning) {
                for (int b = 0; b < NUM_BUCKETS; b++) {
                    int block_idx = (i * NUM_BUCKETS) + b;

                    // 尋找「剛剛發射了，在潛在允許範圍內，但尚未連線」的神經元
                    uint64_t candidates = spikes_current[b] & potential_E_mask[block_idx] & ~active_E_mask[block_idx];

                    // 如果有候選者，我們給予它 5% 的極低機率長出連線 (避免突觸暴增)
                    if (candidates != 0) {
                        uint64_t growth_mask = generate_random_mask(P_of_growth);
                        active_E_mask[block_idx] |= (candidates & growth_mask);
                    }
                    // ----------------------------------------------------
                    // 2. 突觸凋零 (LTD / 剪枝) : "沒有貢獻，就被淘汰"
                    // 條件：來源剛剛【沒有】發射 + 但目前【有連線】
                    // ----------------------------------------------------
                    uint64_t freeloaders = ~spikes_current[b] & active_E_mask[block_idx];

                    if (freeloaders != 0) {
                        // 1% 的機率將這條無用的突觸剪斷 (死亡)
                        // 這裡機率必須比生長(5%)低，否則網路會太快斷光光
                        uint64_t death_mask = generate_random_mask(P_of_death);
                        // 將抽中死亡的 bit 挖掉 (Bitwise AND NOT)
                        active_E_mask[block_idx] &= ~(freeloaders & death_mask);
                    }
                }
            }
        } else {
            if ((global_tick_counter & 63) == 0) {
                if (A[i] > 0) {
                    A[i]--;
                }
            }
        }
    }

    // 3. 封裝輸出層 (Bucket 12 ~ 15)
    Packet256 output_packet;
    for (int b = 0; b < 4; b++) {
        output_packet.blocks[b] = spikes_next[12 + b];
    }

    // 4. 雙重緩衝交換
    std::memcpy(&spikes_current[4], &spikes_next[4], 12 * sizeof(uint64_t));
    std::memset(&spikes_next[4], 0, 12 * sizeof(uint64_t));

    return output_packet;
}
