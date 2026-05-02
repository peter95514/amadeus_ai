#include <math.h>

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

int main() {
    std::ofstream log_file("amadeus_output.bin", std::ios::binary);
    if (!log_file) {
        std::cerr << "無法建立存檔檔案！\n";
        return 0;
    }

    Packet256 red_top_left_input = {0, 0, 0, 0};
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

    /*for (; global_tick_counter < n; global_tick_counter++) {
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

    log_file.close();
    std::cout << "資料紀錄完畢，已存為 amadeus_output.bin\n";
};
