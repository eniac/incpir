import sys
import ast
import matplotlib.pyplot as plt
import numpy as np
fig = plt.figure(num=1,figsize=(35,30))

fig, ax1 = plt.subplots()

font = {'family' : 'Times New Roman',
'weight' : 'normal',
'size'   : 20,
}

ax1.set_xlabel('days', font)
ax1.set_ylabel('relay numbers (k)', font)

# read y_relay_number
y_relay_number = []
trace_file = open("trace.dat", "r")
trace = trace_file.read().split(',')
y_relay_number = [int(ele) for ele in trace]
y_relay_number = [ele/1000 for ele in y_relay_number]  # y-scale is K
trace_file.close()

# set x-axis
x_relay_number = [i for i in range(0,len(y_relay_number))]

ax1.plot(x_relay_number, y_relay_number, color='sandybrown',
          linestyle='solid', linewidth=3,
          label='added relay numbers')
ax1.legend(ncol=1, loc='upper center', bbox_to_anchor=(0.5, 1), framealpha=0, prop={'family':'Times New Roman', 'weight': 'normal', 'size': 18})
plt.tick_params(labelsize=24)

labels = ax1.get_xticklabels() + ax1.get_yticklabels() 
# print labels
[label.set_fontname('Times New Roman') for label in labels]

y_server_comp = []
serial = 4
comp_file = open("build/server_comp.dat", "r")
comp = comp_file.read().split(',')
y_server_comp = [int(ele)*serial for ele in comp]
y_server_comp = [ele/1000000 for ele in y_server_comp] # scale to sec
x_server_comp = [3*i for i in range(0,len(y_server_comp))]   #  3 days

ax2 = ax1.twinx()
ax2.set_ylabel('time (sec)', font)
ax2.plot(x_server_comp, y_server_comp, color='blue',
          linestyle='',
          marker='s', markerfacecolor='blue', markersize=4, alpha=1,
          label='server cost (per change)')
ax2.legend(ncol=1, loc='upper center', bbox_to_anchor=(0.55, 0.90), framealpha=0, prop={'family':'Times New Roman', 'weight': 'normal', 'size': 18})

y_amortized = []
for i in range(1,len(y_server_comp)):
  y_amortized.append(sum(y_server_comp[0:i])/i)

ax2.plot(x_server_comp[0:len(y_amortized)], y_amortized, color='blue',
        linestyle='dashed', linewidth=3,
        label='server cost (amortized)')
ax2.legend(ncol=1, loc='upper center', bbox_to_anchor=(0.55, 0.90), framealpha=0, prop={'family':'Times New Roman', 'weight': 'normal', 'size': 18})

ax1.spines['bottom'].set_linewidth(2)
ax1.spines['left'].set_linewidth(2)
ax2.spines['right'].set_linewidth(2)


plt.rcParams['font.family'] = 'serif'
plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)

plt.tick_params(labelsize=24)
labels = ax1.get_xticklabels() + ax1.get_yticklabels() + ax2.get_yticklabels()
# print labels
[label.set_fontname('Times New Roman') for label in labels]


ax1.spines['top'].set_visible(False)
ax2.spines['top'].set_visible(False)
plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)

fig.tight_layout()
fig.savefig("Figure-12a.pdf")

