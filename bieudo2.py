import matplotlib.pyplot as plt
import numpy as np

# Cập nhật số liệu thực tế (Đơn vị: KB)
labels = ['PQ-PKI\n(ML-KEM+DSA)', 'TLS 1.3 PSK\n(HMAC-SHA2)', 'Muckle\n(HMAC-SHA2)', 'TB-PQ-AKE\n(Ours)']

# Flash (Code Size)
# PQ-PKI = ML-KEM + ML-DSA (~165 KB)
# TLS/Muckle = ML-KEM + Thư viện SHA2 (~90 KB)
# TB-PQ-AKE = 83.7 KB (Số đo thực tế)
flash_usage = [165.0, 92.0, 90.0, 83.7]

# RAM (Static + Stack estimation)
# PQ-PKI (Rất nặng do ML-DSA)
ram_usage = [25.0, 4.5, 4.2, 2.17]


x = np.arange(len(labels))
width = 0.35

fig, ax = plt.subplots(figsize=(10, 6))

# Vẽ biểu đồ cột đôi
rects1 = ax.bar(x - width/2, flash_usage, width, label='Flash (Code Size)', color='#2c3e50', edgecolor='black')
rects2 = ax.bar(x + width/2, ram_usage, width, label='RAM (Stack Memory)', color='#e74c3c', edgecolor='black')

ax.set_ylabel('Memory Consumption (KB)', fontsize=12)
ax.set_title('Figure 1: Hardware Resource Requirements (Flash vs RAM)', fontsize=14, fontweight='bold')
ax.set_xticks(x)
ax.set_xticklabels(labels, fontsize=11)
ax.legend()
ax.grid(axis='y', linestyle='--', alpha=0.5)

# Gắn nhãn số liệu lên đầu từng cột
def autolabel(rects):
    for rect in rects:
        height = rect.get_height()
        ax.annotate(f'{height}',
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=10)

autolabel(rects1)
autolabel(rects2)

plt.tight_layout()
plt.show()