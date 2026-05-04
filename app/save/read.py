import pandas as pd
import matplotlib.pyplot as plt
import os

# 讀取交叉比對的 CSV 檔案
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(BASE_DIR, "average_distance_log.csv")
df = pd.read_csv(file_path)

plt.figure(figsize=(10, 6))

# 畫第一條線：A vs B (完全不同的特徵) -> 預期距離要拉開，用紅色表示
plt.plot(
    df["Tick"],
    df["Dist_DifferentFeature"],
    linestyle="-",
    color="red",
    linewidth=2,
    label="A vs B (Distinct Patterns)",
)

# 畫第二條線：A vs A' (帶雜訊的同一個特徵) -> 預期距離要收斂，用綠色表示
plt.plot(
    df["Tick"],
    df["Dist_SameFeature_WithNoise"],
    linestyle="-",
    color="green",
    linewidth=2,
    label="A vs A' (Same Pattern + Noise)",
)

# 圖表美化
plt.title("Amadeus Output Layer (256-bit): Noise Tolerance vs Feature Separation", fontsize=14)
plt.xlabel("Simulation Step (Tick)", fontsize=12)
plt.ylabel("Average Hamming Distance (bits)", fontsize=12)

# 加入圖例 (Legend) 讓我們知道哪條線是哪條
plt.legend(loc="center right", fontsize=11)

plt.grid(True, linestyle="--", alpha=0.6)
plt.ylim(bottom=0)

plt.tight_layout()


save_dir = os.path.join(BASE_DIR, "./saved_plots_test_for_leakspeed_from_1_to_15")
base_filename = "amadeus_distance_plot"
extension = ".png"

# 如果存檔的資料夾不存在，自動建立它
if not os.path.exists(save_dir):
    os.makedirs(save_dir)

# 掃描資料夾，找出目前最大的編號
max_num = 0
for filename in os.listdir(save_dir):
    # 檢查檔案是否符合我們的命名規則 (例如: amadeus_distance_plot_001.png)
    if filename.startswith(base_filename + "_") and filename.endswith(extension):
        # 擷取中間的數字部分
        try:
            # 去頭去尾，只留下數字字串
            num_str = filename.replace(base_filename + "_", "").replace(extension, "")
            num = int(num_str)
            if num > max_num:
                max_num = num
        except ValueError:
            # 如果轉換數字失敗(例如有人手動亂改檔名)，就忽略該檔案
            pass

# 新編號為最大值 + 1
new_num = max_num + 1

# 將新編號格式化為 3 位數，例如 001, 002... (可以依需求改成 02d 變兩位數)
new_filename = f"{base_filename}_{new_num:03d}{extension}"
save_path = os.path.join(save_dir, new_filename)

# 儲存圖片
plt.savefig(save_path, dpi=300, transparent=False)
print(f"✅ 圖表已成功儲存至: {save_path}")


# plt.show()
