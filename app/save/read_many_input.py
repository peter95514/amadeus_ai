import os
import sys
from numpy import save
import pandas as pd

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

# 檢查參數總數（sys.argv[0] 是腳本自己，後面接 8 個參數，所以總長度必須 >= 9）
if len(sys.argv) >= 9:
    p1, p2, p3, p4, p5, p6, p7, p8 = sys.argv[1:9]
else:
    print("警告：傳入參數不足 8 個，將使用 N/A 代替")
    p1 = p2 = p3 = p4 = p5 = p6 = p7 = p8 = "N/A"

# 讀取 CSV
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(BASE_DIR, "average_distance_log.csv")
df = pd.read_csv(file_path)

plt.figure(figsize=(10, 6))
plt.plot(
    df["Tick"], df["Dist_DifferentFeature"], linestyle="-", color="red", linewidth=2, label="A vs B"
)
plt.plot(
    df["Tick"],
    df["Dist_SameFeature_WithNoise"],
    linestyle="-",
    color="green",
    linewidth=2,
    label="A vs A'",
)

# 標題與存檔
title_text = f"Amadeus Output\n[{p1}, {p2}, {p3}, {p4}] [{p5}, {p6}, {p7}, {p8}]"
plt.title(title_text, fontsize=11)
plt.xlabel("Simulation Step (Tick)")
plt.ylabel("Average Hamming Distance (bits)")
plt.legend(loc="center right")
plt.grid(True, linestyle="--", alpha=0.6)
plt.ylim(bottom=0)
plt.tight_layout()

filename = f"saved_plot_{p1}_{p2}_{p3}_{p4}_{p5}_{p6}_{p7}_{p8}.png"
save_dir = os.path.join(BASE_DIR, "./saved_plots_test")
if not os.path.exists(save_dir):
    os.makedirs(save_dir)
plt.savefig(os.path.join(save_dir, filename))
plt.close()
