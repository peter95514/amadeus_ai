import pandas as pd
import matplotlib.pyplot as plt

# 讀取交叉比對的 CSV 檔案
df = pd.read_csv("average_distance_log.csv")

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
plt.show()
