#include "name.h"

class V1_RetinaEncoder {
private:
    // 記錄上一幀的亮度，用來計算動態視覺 (DVS)
    uint8_t previous_luminance[64] = {0};

    // 觸發 Spike 的靜態閾值
    uint8_t color_threshold = 128;
    // 觸發 DVS Spike 的動態變化閾值
    uint8_t delta_threshold = 15;

public:
    // 傳入一個長度為 64 的像素陣列 (代表畫面上的 8x8 區塊)
    Packet256 encode_patch(const Pixel* patch_8x8);
};
