#include <math.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include "hypercloumn.h"
#include "name.h"
#include "retina.h"

// 1. 宣告系統元件
V1_RetinaEncoder retina_encoder;
CorticalColumn v1_column;  // 使用我們之前設計好的純位元神經柱
unsigned long long n = pow(2, 12);

void amadeus_vision_tick(const Pixel* raw_screen_buffer, int screen_width) {
    // 假設我們現在只看畫面上左上角 (0,0) 的 8x8 區塊
    Pixel patch[64];

    // 從大畫面中裁切出 8x8 的像素 (這裡僅為示意邏輯)
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            patch[y * 8 + x] = raw_screen_buffer[y * screen_width + x];
        }
    }

    // 2. 編碼：將 8x8 像素轉化為 256-bit 的脈衝訊號
    Packet256 sensory_spikes = retina_encoder.encode_patch(patch);

    // 3. 核心運算：將脈衝打入 V1 神經柱！
    // 這些 Spike 會從 ID 0~255 的輸入區，透過墨西哥帽拓撲擴散到水庫隱藏層
    Packet256 v1_features = v1_column.tick(sensory_spikes);

    // 4. v1_features 現在包含了這 8x8 區塊的高階特徵 (例如邊緣、移動方向)
    // 可以將它傳遞給 V2，或是與其他神經柱的輸出進行 Router 組合
}

// 隨機翻轉 Packet256 中的幾個 bit 來模擬雜訊
Packet256 generate_noisy_packet(const Packet256& original, int num_noise_bits, std::mt19937& rng) {
    Packet256 noisy = original;
    std::uniform_int_distribution<int> block_dist(0, 3);
    std::uniform_int_distribution<int> bit_dist(0, 63);

    for (int i = 0; i < num_noise_bits; i++) {
        int block_idx = block_dist(rng);
        int bit_idx = bit_dist(rng);
        // 使用 XOR (^) 來翻轉特定的 bit
        noisy.blocks[block_idx] ^= (1ULL << bit_idx);
    }
    return noisy;
}

void run_for_no_learning() {
    const int NUM_TRIALS = 1000;  // 跑 1000 次
    const int NUM_TICKS = 1024;   // 每次觀察 100 個時間步長
    const int NOISE_BITS = 2;     // 設定 A' 只有 2 個 bit 的雜訊微小差異

    // 用一個 vector 來儲存每個 tick 的「距離總和」
    // 大小為 100，初始值全部填 0
    std::vector<long long> dist_A_Aprime_sum(NUM_TICKS, 0);
    std::vector<long long> dist_A_B_sum(NUM_TICKS, 0);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::cout << "開始執行 " << NUM_TRIALS << " 次實驗取平均...\n";

    for (int trial = 0; trial < NUM_TRIALS; trial++) {
        // 1. 產生一個全新的皮層柱 (包含隨機初始化的連線與遮罩)
        CorticalColumn col_A;

        // 2. 複製出一個一模一樣的雙胞胎 (權重、初始狀態完全相同)
        CorticalColumn col_B = col_A;
        CorticalColumn col_A_prime = col_A;

        // 3. 準備你的正交輸入 (這裡以假代碼表示，請換成你的 Packet256)
        Packet256 pattern_A; /* 設定為左刺激 */
        Packet256 pattern_B; /* 設定為右刺激 */

        pattern_A.blocks[0] = ~0ULL;
        pattern_A.blocks[1] = ~0ULL;
        pattern_B.blocks[2] = ~0ULL;
        pattern_B.blocks[3] = ~0ULL;
        Packet256 pattern_A_prime = pattern_A;

        // 4. 讓這對雙胞胎在時間軸上平行推進
        for (int tick = 0; tick < NUM_TICKS; tick++) {
            // 雙胞胎分別接收不同的刺激 (記得關閉學習模式)
            Packet256 out_A = col_A.tick(pattern_A, false);
            Packet256 out_A_prime = col_A_prime.tick(pattern_A_prime, false);
            Packet256 out_B = col_B.tick(pattern_B, false);

            int dist_AAp = 0;
            int dist_AB = 0;
            for (int b = 0; b < 4; b++) {
                dist_AAp += POPCOUNT64(out_A.blocks[b] ^ out_A_prime.blocks[b]);
                dist_AB += POPCOUNT64(out_A.blocks[b] ^ out_B.blocks[b]);
            }

            // 累加距離
            dist_A_Aprime_sum[tick] += dist_AAp;
            dist_A_B_sum[tick] += dist_AB;
        }

        // 印出進度條，以免畫面卡住以為當機
        std::cout << "已完成 " << (trial + 1) << " 次實驗...\n";
    }

    // ---------------------------------------------------------
    // 實驗跑完，計算平均並寫入 CSV
    // ---------------------------------------------------------
    std::string target_folder = "../save";
    std::string file_path = target_folder + "/average_distance_log.csv";

    if (!std::filesystem::exists(target_folder)) {
        std::filesystem::create_directory(target_folder);
    }

    std::ofstream outfile(file_path);
    if (!outfile.is_open()) {
        std::cerr << "無法開啟檔案寫入！路徑：" << file_path << "\n";
        return;
    }

    outfile << "Tick,Dist_SameFeature_WithNoise,Dist_DifferentFeature\n";
    for (int tick = 0; tick < NUM_TICKS; tick++) {
        double avg_AAp = static_cast<double>(dist_A_Aprime_sum[tick]) / NUM_TRIALS;
        double avg_AB = static_cast<double>(dist_A_B_sum[tick]) / NUM_TRIALS;
        outfile << tick << "," << avg_AAp << "," << avg_AB << "\n";
    }
    outfile.close();
    std::cout << "1000次平均數據已成功儲存至 " << file_path << "\n";

    /*Packet256 red_top_left_input = {0, 0, 0, 0};
    Packet256 green_top_left_input = {0, 0, 0, 0};
    Packet256 null_top_left_input = {0, 0, 0, 0};
    Packet256 blue = {0, 0, 0, 0};
    Packet256 right_shift = {0, 0, 0, 0};

    // 1ULL = 二進位的 0001 (1個細胞)
    // 15ULL = 二進位的 1111 (4個細胞同時激發)
    // // 修改前：
    // red_top_left_input.blocks[0] = 15ULL;

    // 修改後：~0ULL 代表 64 個 bit 全為 1，火力全開！
    red_top_left_input.blocks[0] = ~0ULL;
    green_top_left_input.blocks[1] = ~0ULL;
    blue.blocks[2] = ~0ULL;
    right_shift.blocks[0] = 1ULL;

    for (; global_tick_counter < n; global_tick_counter++) {
        std::cout << "time " << global_tick_counter << std::endl;
        Packet256 output_packet = v1_column.tick(right_shift);
        log_file.write(reinterpret_cast<const char*>(&output_packet), sizeof(Packet256));
        right_shift.blocks[0] = right_shift.blocks[0] << 1;
        if ((global_tick_counter & 256) == 0) {
            right_shift.blocks[0] = 1ULL;
        }
    }

    for (; global_tick_counter < n; global_tick_counter++) {
        std::cout << "time " << global_tick_counter << std::endl;
        Packet256 output_packet = v1_column.tick(red_top_left_input);
        log_file.write(reinterpret_cast<const char*>(&output_packet), sizeof(Packet256));
    }
    for (; global_tick_counter < n + 1024; global_tick_counter++) {
        std::cout << "time " << global_tick_counter << std::endl;
        Packet256 output_packet = v1_column.tick(null_top_left_input);
        log_file.write(reinterpret_cast<const char*>(&output_packet), sizeof(Packet256));
    }
    for (; global_tick_counter < 2 * n + 1024; global_tick_counter++) {
        std::cout << "time " << global_tick_counter << std::endl;
        Packet256 output_packet = v1_column.tick(green_top_left_input);
        log_file.write(reinterpret_cast<const char*>(&output_packet), sizeof(Packet256));
    }
    for (; global_tick_counter < 3 * n + 1024; global_tick_counter++) {
        std::cout << "time " << global_tick_counter << std::endl;
        Packet256 output_packet = v1_column.tick(blue);
        log_file.write(reinterpret_cast<const char*>(&output_packet), sizeof(Packet256));
    }*/
}

int main() {
    run_for_no_learning();
};
