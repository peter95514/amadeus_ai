import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# ==========================================
# 1. 瞬間讀取與解碼二進位檔
# ==========================================
# 直接把檔案讀成 uint64 陣列
raw_data = np.fromfile("build/amadeus_output.bin", dtype=np.uint64)

# 算出總共有多少個 Tick (每個 Tick 有 4 個 uint64)
num_ticks = len(raw_data) // 4

# 重塑陣列形狀：(時間幀數, 4 個桶子)
packets = raw_data.reshape((num_ticks, 4))

print(f"成功載入 {num_ticks} 幀神經脈衝資料！")

# ==========================================
# 2. NumPy 魔法：把 uint64 炸開成 0 與 1 的 256 bits
# ==========================================
# 準備 0~63 的位移量陣列
shifts = np.arange(64, dtype=np.uint64)

# 利用廣播機制，一口氣把所有時間點、所有桶子的 bit 全部取出來
# packets[:, :, None] 會變成 (N, 4, 1) 維度，對上 (64,) 維度的 shifts
spikes = (packets[:, :, None] >> shifts) & 1

# 重塑為 (時間幀數, X座標, Y座標) 的 16x16 網格
# 這就是完美的時空脈衝矩陣 (Spatio-temporal Spike Matrix)
spikes_2d = spikes.reshape((num_ticks, 16, 16))

# ==========================================
# 3. 視覺化：畫出神經科學最愛的「脈衝光柵圖 (Raster Plot)」
# ==========================================
# Raster plot 是一次看清所有神經元在時間軸上表現的最佳方式
spikes_flat = spikes.reshape((num_ticks, 256))
# 找出所有發射了脈衝的 (時間點, 神經元ID)
time_indices, neuron_indices = np.where(spikes_flat == 1)

plt.figure(figsize=(12, 6))
plt.scatter(time_indices, neuron_indices, s=1, c="black", marker="|")
plt.title("Amadeus Output Layer - Spike Raster Plot")
plt.xlabel("Time (Ticks)")
plt.ylabel("Neuron ID (0-255)")
plt.ylim(0, 255)
plt.tight_layout()
plt.show()

# ==========================================
# 4. 視覺化：產生帶有殘影的動態熱圖 (Heatmap Animation)
# ==========================================
decay_rate = 0.85
heatmap = np.zeros((num_ticks, 16, 16))

# 計算時間軸上的熱量衰減
current_heat = np.zeros((16, 16))
for t in range(num_ticks):
    current_heat = current_heat * decay_rate + spikes_2d[t]
    # 限制最高溫度為 1.0
    current_heat = np.clip(current_heat, 0.0, 1.0)
    heatmap[t] = current_heat

# 顯示特定一幀的熱圖 (例如第 50 幀)
plt.figure(figsize=(5, 5))
plt.imshow(heatmap[50], cmap="inferno", vmin=0, vmax=1.0)
plt.title("Heatmap at Tick 50")
plt.colorbar()
plt.show()
