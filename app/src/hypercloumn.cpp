#include "hypercloumn.h"

#include <algorithm>
#include <cmath>    // 提供 std::abs, std::max
#include <cstdint>  // 提供 uint64_t 等定寬整數型別
#include <cstring>  // 提供 std::memcpy, std::memset
#include <iostream>

#include "name.h"

CorticalColumn::CorticalColumn(int potential_E_rate, int potential_I_rate, int leak_speed, int essential_A_mask,
                               int P_of_growth, int P_of_death, int T_of_leak, int one_time_of_V) {
    this->potential_E_rate = potential_E_rate;
    this->potential_I_rate = potential_I_rate;
    this->leak_speed = leak_speed;
    this->essential_A_mask = essential_A_mask;
    this->P_of_growth = P_of_growth;
    this->P_of_death = P_of_death;
    this->T_of_leak = T_of_leak;
    this->one_time_of_V = one_time_of_V;

    // C++ 原生陣列初始化，將膜電位底線設為 1 (最低位階)
    std::random_device rd;
    rng.seed(rd());
    for (int i = 0; i < NUM_NEURONS; i++) {
        V[i] = 0;
        A[i] = essential_A_mask;
    }

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
        int my_bucket = i / NEURONS_PER_BUCKET;

        for (int target_bucket = 0; target_bucket < NUM_BUCKETS; target_bucket++) {
            // 使用轉置後的索引
            int block_idx = (target_bucket * NUM_NEURONS) + i;

            // ★ 直接查表，拿掉所有的座標除法、餘數與距離計算 ★
            int type = TOPO_LUT.conn_type[my_bucket][target_bucket];

            if (type == 1) {
                potential_E_mask[block_idx] = ~0ULL;
            } else if (type == 2) {
                potential_I_mask[block_idx] = ~0ULL;
            }
        }

        // 防呆：把自己跟自己的潛在激發連線挖掉 (Bitwise AND NOT)
        int self_block_idx = (my_bucket * NUM_NEURONS) + i;
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
    // 1.輸入外部信號
    for (int i = 0; i < 4; i++) {
        spikes_current[i] = input_packet.blocks[i];
    }

    // 2.神經狀態更新

    alignas(64) int E_spike[NUM_NEURONS] = {0};
    alignas(64) int I_spike[NUM_NEURONS] = {0};

    for (int b = 0; b < NUM_BUCKETS; b++) {
        uint64_t spikes = spikes_current[b];

        // 稀疏性優化：如果這個 Bucket 剛剛沒有任何人發射，直接跳過，省下 1024 次迴圈！
        if (spikes == 0) continue;
        int base_idx = b * NEURONS_PER_BUCKET;  // 鎖定這個 Bucket 的記憶體起點

        // 告訴編譯器這裡沒有迴圈相依性，大膽使用 SIMD 向量化指令
        for (int i = 0; i < NUM_NEURONS; i++) {
            // 注意這裡的索引：base_idx + i 是一段連續的記憶體！
            E_spike[i] += POPCOUNT64(spikes & active_E_mask[base_idx + i]);
            I_spike[i] += POPCOUNT64(spikes & active_I_mask[base_idx + i]);
            // std::cout << active_E_mask[base_idx + i] << std::endl;
        }
    }

    for (int i = 0; i < NUM_NEURONS; i++) {
        // 1. 結算淨位移 (限制在 -3 到 3 階之間)
        int net_shift = std::clamp(E_spike[i] - I_spike[i], -max_level_of_V, max_level_of_V);

        // 2. 更新膜電位 (純整數加減與位移)
        if (net_shift >= 0) {
            V[i] = std::min((int)V[i] + net_shift, max_level_of_V);  // 頂到 63 階為止
        } else {
            V[i] = std::max(0, (int)V[i] >> (-net_shift));  // 除法衰減，底線為 0
        }

        // 3. 脈衝觸發判定 (極簡的整數比較)
        int current_threshold = essential_A_mask + A[i];

        if (V[i] >= current_threshold) {
            // 發射 Spike
            spikes_next[i / NEURONS_PER_BUCKET] |= (1ULL << (i % NEURONS_PER_BUCKET));

            // 觸發後重置電位 (整數 0 即為底線)
            V[i] = 0;

            // 增加疲勞值 (無分支寫法)
            A[i] += (A[i] < max_allowed_A);

            // ==========================================
            // ★ 純位元結構可塑性 (配合轉置索引修正)
            // ==========================================
            if (enable_learning) {
                for (int b = 0; b < NUM_BUCKETS; b++) {
                    // 🚨 這裡的 block_idx 必須同步改成轉置寫法！
                    int block_idx = (b * NUM_NEURONS) + i;

                    // 尋找「剛剛發射了，在潛在允許範圍內，但尚未連線」的神經元
                    uint64_t candidates = spikes_current[b] & potential_E_mask[block_idx] & ~active_E_mask[block_idx];

                    if (candidates != 0) {
                        uint64_t growth_mask = generate_random_mask(P_of_growth);
                        active_E_mask[block_idx] |= (candidates & growth_mask);
                    }

                    // 尋找「沒有貢獻，卻佔用連線」的神經元
                    uint64_t freeloaders = ~spikes_current[b] & active_E_mask[block_idx];

                    if (freeloaders != 0) {
                        uint64_t death_mask = generate_random_mask(P_of_death);
                        active_E_mask[block_idx] &= ~(freeloaders & death_mask);
                    }
                }
            }
        } else {
            // 未觸發時的自然漏電與疲勞恢復
            if (((global_tick_counter + i) & T_of_leak) == 0) {
                A[i] -= (A[i] > 0);
                V[i] = std::max(0, (int)V[i] >> leak_speed);  // 自然漏電
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
