import matplotlib.pyplot as plt
import numpy as np
from matplotlib.pyplot import MultipleLocator

fig = plt.figure(num=1,figsize=(35,30))

fig, ax1 = plt.subplots()


font = {'family' : 'Times New Roman',

'size'   : 26,
}

ax1.set_xlabel('days', font)
ax1.set_ylabel('percentage of growth', font)
plt.ylim(ymax = 10)
plt.xlim(xmax=90)

x = [3*i for i in range(1,29)]
perstore_a = []
perstore_b = []
perstore_c = []


with open("build/client_storage.dat", "r") as f:
    lines = f.readlines()
    lines = [line.rstrip() for line in lines]

lines[0] = lines[0].split(',')
lines[1] = lines[1].split(',')
lines[2] = lines[2].split(',')

perstore_a = [float(ele) for ele in lines[0]]
perstore_b = [float(ele) for ele in lines[1]]
perstore_c = [float(ele) for ele in lines[2]]

ax1.plot(x, perstore_a, color='blue',
          linestyle='solid', linewidth=3,
          marker='s', markerfacecolor='blue', markersize=5,
          label='100')
ax1.plot(x, perstore_b, color='slateblue',
          linestyle='solid', linewidth=3,
          marker='s', markerfacecolor='slateblue', markersize=5,
          label='200')
ax1.plot(x, perstore_c, color='thistle',
          linestyle='solid', linewidth=3,
          marker='s', markerfacecolor='thistle', markersize=5,
          label='500')

ax1.legend(loc='upper center', bbox_to_anchor=(0.2,0.9), framealpha=0, prop={'family':'Times New Roman', 'size': 24})

plt.text(6, 9,'#Queries between updates', fontfamily='Times New Roman', fontsize=26)

plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)

x_major_locator=MultipleLocator(25)
ax1.xaxis.set_major_locator(x_major_locator)

plt.tick_params(labelsize=20)
labels = ax1.get_xticklabels() + ax1.get_yticklabels()
# print labels
[label.set_fontname('Times New Roman') for label in labels]

ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)

ax1.spines['bottom'].set_linewidth(2)
ax1.spines['left'].set_linewidth(2)

fig.tight_layout()
fig.savefig("Figure-12b.pdf")
