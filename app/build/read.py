import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider

# ==========================================
# 1. 讀取與解碼二進位資料 (Zero-overhead Decoding)
# ==========================================
# 讀取 C++ 寫出的二進位檔
raw_data = np.fromfile("amadeus_output.bin", dtype=np.uint64)

# 算出總幀數 (每個 Tick 有 4 個 uint64 桶子)
num_ticks = len(raw_data) // 4
if num_ticks == 0:
    print("錯誤：檔案為空或讀取失敗！")
    exit()

# 將一維陣列重塑為 (Ticks, 4個桶子)
packets = raw_data.reshape((num_ticks, 4))

# 瞬間位元解碼魔法
shifts = np.arange(64, dtype=np.uint64)
# 這會把每個 uint64 炸開成 64 個 0 或 1，然後直接塑形成 16x16 網格
spikes_2d = ((packets[:, :, None] >> shifts) & 1).reshape((num_ticks, 16, 16))

print(f"成功載入資料！總共 {num_ticks} 個 Ticks。")

# ==========================================
# 2. 建立互動式 UI 畫布
# ==========================================
fig, ax = plt.subplots(figsize=(7, 7))

# 底部留白，用來放置時間軸拉桿
plt.subplots_adjust(bottom=0.25) 

# 設定顏色映射：0 顯示為深藍色(或黑色)，1 顯示為亮黃色
cmap = plt.cm.magma 
img = ax.imshow(spikes_2d[0], cmap=cmap, vmin=0, vmax=1)
ax.set_title(f"Amadeus Output - Tick: 0")

# 畫上 16x16 網格線輔助視覺 (讓單個細胞更清楚)
ax.set_xticks(np.arange(-.5, 16, 1), minor=True)
ax.set_yticks(np.arange(-.5, 16, 1), minor=True)
ax.grid(which='minor', color='gray', linestyle='-', linewidth=0.5)
ax.tick_params(which='minor', size=0)

# ==========================================
# 3. 建立時間軸拉桿 (Slider)
# ==========================================
# 定義拉桿的位置 [左, 下, 寬, 高]
ax_slider = plt.axes([0.15, 0.1, 0.7, 0.03]) 
slider = Slider(
    ax=ax_slider,
    label='Tick',
    valmin=0,
    valmax=num_ticks - 1,
    valinit=0,
    valstep=1 # 強制只能選取整數 Tick
)

# ==========================================
# 4. 定義拉桿拖曳時的更新事件
# ==========================================
def update(val):
    tick = int(slider.val)
    # 更新畫面上的 16x16 矩陣資料
    img.set_data(spikes_2d[tick])
    # 更新標題顯示當前的 Tick
    ax.set_title(f"Amadeus Output - Tick: {tick}")
    # 通知畫布重新繪製
    fig.canvas.draw_idle()

# 將更新事件綁定到拉桿上
slider.on_changed(update)

# 啟動互動視窗
plt.show()
