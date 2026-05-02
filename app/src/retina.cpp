#include "retina.h"

#include <cmath>
#include <cstdint>

// V1 專用的視網膜編碼器 (負責處理 8x8 = 64 個像素的感受野)
// 傳入一個長度為 64 的像素陣列 (代表畫面上的 8x8 區塊)
Packet256 V1_RetinaEncoder::encode_patch(const Pixel* patch_8x8) {
    Packet256 packet = {0, 0, 0, 0};

    for (int i = 0; i < 64; i++) {
        const Pixel& p = patch_8x8[i];
        uint64_t bit = 1ULL << i;

        // 1. 紅色通道 (Block 0)
        if (p.r > color_threshold && p.r > p.g && p.r > p.b) {
            packet.blocks[0] |= bit;
        }

        // 2. 綠色通道 (Block 1)
        if (p.g > color_threshold && p.g > p.r && p.g > p.b) {
            packet.blocks[1] |= bit;
        }

        // 3. 藍色通道 (Block 2)
        if (p.b > color_threshold && p.b > p.r && p.b > p.g) {
            packet.blocks[2] |= bit;
        }

        // 4. 動態視覺 DVS (Block 3) - 偵測運動與閃爍
        uint8_t current_luma = (p.r + p.g + p.b) / 3;
        int delta = current_luma - previous_luminance[i];

        if (std::abs(delta) > delta_threshold) {
            packet.blocks[3] |= bit;
            // 更新基準點
            previous_luminance[i] = current_luma;
        }
    }

    return packet;
}
