import numpy as np 
import pandas as pd
import matplotlib.pyplot as plt 
import sys
import math
import re
import os
from matplotlib.gridspec import GridSpec
from pathlib import Path


def get_alpha(path: Path):
    alpha_str = path.stem.split("_p")[-1]
    return float(alpha_str)


start = int(sys.argv[1]) 
stop = int(sys.argv[2])

fig = plt.figure(figsize=(12,15), constrained_layout=True)

gs = GridSpec(
    5, 3,
    width_ratios=[1.5,1,1.5], 
    hspace=0.4,
    wspace=0.3
)

axes_time = []
axes_phase = []
axes_fft = []

for i in range(5):

    if i == 0:
        ax_time = fig.add_subplot(gs[i,0])
    else:
        ax_time = fig.add_subplot(gs[i,0], sharex=axes_time[0])

    ax_phase = fig.add_subplot(gs[i,1])
    ax_fft = fig.add_subplot(gs[i,2])

    axes_time.append(ax_time)
    axes_phase.append(ax_phase)
    axes_fft.append(ax_fft)


alphas = np.round(np.arange(3.0, 9.2, 0.01), 2)
#alphas =  [3.00, 3.66, 3.90, 4.76, 5.57, 6.25, 7.50, 8.07]
alphasMCU = [3.00, 3.66, 3.90, 4.36, 4.76, 5.57, 5.99, 6.00, 6.02, 6.08, 6.25, 6.54, 7.50, 8.07]
time = np.arange(start,stop,1,dtype=np.int_)

folder_names = ["double_vs_float", "fcache_vs_fnocache", "fgsl_vs_fmath", "f_bg_precomp_vs_f_all_precomp", "f_math_precomp_vs_f_math_noprecomp"]
labels_f = [r"float data with gsl $\Gamma$", r"f_gsl_data neurons not cached", r"f_data with gsl $\Gamma$", f"f_math_data with \n cached neurons + bg", f"f_math_data with \n cached neurons"]
labels_d = [r"double data with gsl $\Gamma$", r"f_gsl_data neurons cached", r"f_data with mathematical $\Gamma$", f"f_math_data \n with cached neurons \n + bg + sinh + tanh", f"f_math_data_ \n with cached neurons \n + bg + sinh + tanh"]

fft_labels_a = ["double_og", "float_cache" , "float_nogamma" , "float_nogamma_precomp", "float_nogamma_precomp"]
fft_labels_b = ["float_og", "float_og",      "float_og",       "float_nogamma2",        "float_nogamma2" ]

with open("sorted_paths/paths_to_data_float_nogamma_precomp.txt") as f_d,\
    open("sorted_paths/paths_to_data_float_nogamma.txt") as f_f:
    data_paths_double_sorted = [line.strip() for line in f_d]
    data_paths_float_sorted  = [line.strip() for line in f_f]

    
number = 4
folder1 = Path(f'fft/2k_to_2.5k_data/{fft_labels_a[number]}')
folder2 = Path(f'fft/2k_to_2.5k_data/{fft_labels_b[number]}')

files1 = sorted(folder1.iterdir(), key=get_alpha)
files2 = sorted(folder2.iterdir(), key=get_alpha)


# for data_path_d, data_path_f in zip(data_paths_double_sorted, data_paths_float_sorted):
#     with open(data_path_d) as file_d,\
#         open(data_path_f) as file_f:
#         data_d = np.loadtxt(file_d)[start:stop]
#         data_f = np.loadtxt(file_f)[start:stop]


#         if data_path_d[-12] == 'm':
#             alpha_d = -1.*float(data_path_d[-11:-4])
#         else:
#             alpha_d = float(data_path_d[-11:-4])

#         if data_path_f[-12] == 'm':
#             alpha_f = -1.*float(data_path_f[-11:-4])
#         else:
#             alpha_f = float(data_path_f[-11:-4])

#         if abs(alpha_d - alpha_f) > 1e-6:
#             print("NOT EQUAL ALPHAS!")


for a in alphas:
    data_path_d = next(dp for dp in data_paths_double_sorted if abs(float(dp[-11:-4]) - a) < 1e-5)
    data_path_f = next(fp for fp in data_paths_float_sorted if abs(float(fp[-11:-4]) - a) < 1e-5)
    
    data_d = np.loadtxt(data_path_d)[start:stop]
    data_f = np.loadtxt(data_path_f)[start:stop]

    y0d, y1d, y2d, y3d, y4d = data_d[:,0], data_d[:,1], data_d[:,2], data_d[:,3], data_d[:,4]
    y0f, y1f, y2f, y3f, y4f = data_f[:,0], data_f[:,1], data_f[:,2], data_f[:,3], data_f[:,4]

        # for a in alphas:
        #     if abs(alpha_f - a) < 1e-5 and abs(alpha_d - a) < 1e-5: 

    fig.suptitle(f"Neurons dynamic for alpha={a:.2f}",fontsize = "x-large")

    if a in alphasMCU and stop <4001:
        print("Weszlismy do pętli! hura!", flush = True)
        df1 = pd.read_csv(f"MCU/MCU_time_evol_p{a:.2f}.csv", delimiter=",", header=None, names=["y1mcu", "y2mcu", "y3mcu", "y4mcu", "y5mcu", "dt1"])
        y1mcu = list(df1["y1mcu"])[start:stop]
        y2mcu = list(df1["y2mcu"])[start:stop]
        y3mcu = list(df1["y3mcu"])[start:stop]
        y4mcu = list(df1["y4mcu"])[start:stop]
        y5mcu = list(df1["y5mcu"])[start:stop]

        axes_time[0].scatter(time, y1mcu, s = 2.1, c = "green", marker = 'o', label = 'MCU')
        axes_time[1].scatter(time, y2mcu, s = 2.1, c = "green", marker = 'o')
        axes_time[2].scatter(time, y3mcu, s = 2.1, c = "green", marker = 'o')
        axes_time[3].scatter(time, y4mcu, s = 2.1, c = "green", marker = 'o')
        axes_time[4].scatter(time, y5mcu, s = 2.1, c = "green", marker = 'o')

        axes_phase[0].scatter(y1mcu[0:-1], y1mcu[1:], s = 10, c = "green", marker = 'o', label = 'MCU')
        axes_phase[1].scatter(y2mcu[0:-1], y2mcu[1:], s = 10, c = "green", marker = 'o')
        axes_phase[2].scatter(y3mcu[0:-1], y3mcu[1:], s = 10, c = "green", marker = 'o')
        axes_phase[3].scatter(y4mcu[0:-1], y4mcu[1:], s = 10, c = "green", marker = 'o')
        axes_phase[4].scatter(y5mcu[0:-1], y5mcu[1:], s = 10, c = "green", marker = 'o')


    axes_time[0].scatter(time, y0d, s = 0.6, alpha = 0.8, c = "red", marker = 'o', label = f'{labels_d[number]}') #r'mathematical $\Ga$
    axes_time[0].scatter(time, y0f, s = 0.1, c = "black", marker = 'o', label = f'{labels_f[number]}')

    axes_time[0].yaxis.get_major_formatter().set_scientific(True)
    axes_time[0].xaxis.get_major_formatter().set_scientific(True)
    axes_time[0].ticklabel_format(style='plain', useOffset=False)

    axes_time[1].scatter(time, y1d, s = 0.6, alpha = 0.8, marker = "o", facecolors='none', edgecolors='r')
    axes_time[1].scatter(time, y1f, s = 0.1, c = "black", marker = 'o')

    axes_time[1].yaxis.get_major_formatter().set_scientific(True)
    axes_time[1].xaxis.get_major_formatter().set_scientific(True)
    axes_time[1].ticklabel_format(style='plain', useOffset=False)

    axes_time[2].scatter(time, y2d, s = 0.6, alpha = 0.8, marker = 'o', facecolors='none', edgecolors='r')
    axes_time[2].scatter(time, y2f, s = 0.1, c = "black", marker = 'o')

    axes_time[2].yaxis.get_major_formatter().set_scientific(True)
    axes_time[2].xaxis.get_major_formatter().set_scientific(True)
    axes_time[2].ticklabel_format(style='plain', useOffset=False)

    axes_time[3].scatter(time, y3d, s = 0.6, alpha = 0.8, marker = 'o', facecolors='none', edgecolors='r')
    axes_time[3].scatter(time, y3f, s = 0.1, c = "black", marker = 'o')

    axes_time[3].yaxis.get_major_formatter().set_scientific(True)
    axes_time[3].xaxis.get_major_formatter().set_scientific(True)
    axes_time[3].ticklabel_format(style='plain', useOffset=False)

    axes_time[4].scatter(time, y4d, s = 0.6, alpha = 0.8, marker = 'o', facecolors='none', edgecolors='r')
    axes_time[4].scatter(time, y4f, s = 0.1, c = "black", marker = 'o')

    axes_time[4].yaxis.get_major_formatter().set_scientific(True)
    axes_time[4].xaxis.get_major_formatter().set_scientific(True)
    axes_time[4].ticklabel_format(style='plain', useOffset=False)

    axes_time[0].set_ylabel("y0")
    axes_time[1].set_ylabel("y1")
    axes_time[2].set_ylabel("y2")
    axes_time[3].set_ylabel("y3")
    axes_time[4].set_ylabel("y4")

    axes_time[4].set_xlabel("Time [steps]")


    axes_time[0].legend(loc="best", fontsize="6")

    axes_phase[0].scatter(y0d[0:-1], y0d[1:], s = 4, alpha = 0.5, c = "red", marker = 'o', label = f'{labels_d[number]}') #x od 0 do 10, y od 1 do 11 
    axes_phase[0].scatter(y0f[0:-1], y0f[1:], s = 1, alpha = 0.5, c = "black", label = f'{labels_f[number]}')

    axes_phase[0].yaxis.get_major_formatter().set_scientific(True)
    axes_phase[0].xaxis.get_major_formatter().set_scientific(True)
    axes_phase[0].ticklabel_format(style='plain', useOffset=False)

    axes_phase[1].scatter(y1d[0:-1], y1d[1:], s = 4, alpha = 0.5,  c = "red", marker = 'o')
    axes_phase[1].scatter(y1f[0:-1], y1f[1:], s = 1, alpha = 0.5,  c = "black")

    axes_phase[1].yaxis.get_major_formatter().set_scientific(True)
    axes_phase[1].xaxis.get_major_formatter().set_scientific(True)
    axes_phase[1].ticklabel_format(style='plain', useOffset=False)

    axes_phase[2].scatter(y2d[0:-1], y2d[1:], s = 4, alpha = 0.5, c = "red", marker = 'o')
    axes_phase[2].scatter(y2f[0:-1], y2f[1:], s = 1, alpha = 0.5, c = "black")

    axes_phase[2].yaxis.get_major_formatter().set_scientific(True)
    axes_phase[2].xaxis.get_major_formatter().set_scientific(True)
    axes_phase[2].ticklabel_format(style='plain', useOffset=False)

    axes_phase[3].scatter(y3d[0:-1], y3d[1:], s = 4, alpha = 0.5, c = "red", marker = 'o')
    axes_phase[3].scatter(y3f[0:-1], y3f[1:], s = 1, alpha = 0.5, c = "black")

    axes_phase[3].yaxis.get_major_formatter().set_scientific(True)
    axes_phase[3].xaxis.get_major_formatter().set_scientific(True)
    axes_phase[3].ticklabel_format(style='plain', useOffset=False)

    axes_phase[4].scatter(y4d[0:-1], y4d[1:], s = 4, alpha = 0.5, c = "red", marker = 'o')
    axes_phase[4].scatter(y4f[0:-1], y4f[1:], s = 1, alpha = 0.5, c = "black")

    axes_phase[4].yaxis.get_major_formatter().set_scientific(True)
    axes_phase[4].xaxis.get_major_formatter().set_scientific(True)
    axes_phase[4].ticklabel_format(style='plain', useOffset=False)

    axes_phase[0].set_xlabel("y0(t)")
    axes_phase[1].set_xlabel("y1(t)")
    axes_phase[2].set_xlabel("y2(t)")
    axes_phase[3].set_xlabel("y3(t)")
    axes_phase[4].set_xlabel("y4(t)")

    axes_phase[0].set_ylabel("y0(t+1)")
    axes_phase[1].set_ylabel("y1(t+1)")
    axes_phase[2].set_ylabel("y2(t+1)")
    axes_phase[3].set_ylabel("y3(t+1)")
    axes_phase[4].set_ylabel("y4(t+1)")


    axes_phase[0].legend(loc='best', prop={'size': 5})

#---------------------------------------------------------------------------------------------

    f1 = next(f for f in files1 if abs(get_alpha(f) - a) < 1e-5)
    f2 = next(f for f in files2 if abs(get_alpha(f) - a) < 1e-5)

    data1 = np.loadtxt(f1)
    data2 = np.loadtxt(f2)

    periods1, magn_y0a, magn_y1a, magn_y2a, magn_y3a, magn_y4a = data1[:,0], data1[:,1], data1[:,2], data1[:,3], data1[:,4], data1[:,5]
    periods2, magn_y0b, magn_y1b, magn_y2b, magn_y3b, magn_y4b = data2[:,0], data2[:,1], data2[:,2], data2[:,3], data2[:,4], data2[:,5]

    axes_fft[0].plot(periods1, magn_y0a, '-r', label = rf"{labels_d[number]}")
    axes_fft[0].set_title('y0')
    axes_fft[0].plot(periods2, magn_y0b, '-b', alpha = 0.5, label = rf"{labels_f[number]}")
    axes_fft[0].set_yscale('log')
    axes_fft[0].set_xlabel(r'$freq$')
    axes_fft[0].set_ylabel(r'$Mag$')
    axes_fft[0].grid(True)
    axes_fft[0].legend()
    axes_fft[0].set_xlim([0.01,0.49])
    axes_fft[0].legend(loc='best', prop={'size': 6})
    


    axes_fft[1].plot(periods1, magn_y1a, '-r')
    axes_fft[1].plot(periods2, magn_y1b, '-b', alpha = 0.5)
    axes_fft[1].set_title('y1')
    axes_fft[1].set_yscale('log')
    axes_fft[1].set_xlabel(r'$freq$')
    axes_fft[1].set_ylabel(r'$Mag$')
    axes_fft[1].grid(True)
    axes_fft[1].set_xlim([0.01,0.49])
    
    axes_fft[2].plot(periods1, magn_y2a, '-r')
    axes_fft[2].plot(periods2, magn_y2b, '-b', alpha = 0.5)
    axes_fft[2].set_title('y2')
    axes_fft[2].set_yscale('log')
    axes_fft[2].set_xlabel(r'$freq$')
    axes_fft[2].set_ylabel(r'$Mag$')
    axes_fft[2].grid(True)
    axes_fft[2].set_xlim([0.01,0.49])

    
    axes_fft[3].plot(periods1, magn_y3a, '-r')
    axes_fft[3].plot(periods2, magn_y3b, '-b', alpha = 0.5)
    axes_fft[3].set_title('y3')
    axes_fft[3].set_yscale('log')
    axes_fft[3].set_xlabel(r'$freq$')
    axes_fft[3].set_ylabel(r'$Mag$')
    axes_fft[3].grid(True)
    axes_fft[3].set_xlim([0.01,0.49])

    
    axes_fft[4].plot(periods1, magn_y4a, '-r')
    axes_fft[4].plot(periods2, magn_y4b, '-b', alpha = 0.5)
    axes_fft[4].set_title('y4')
    axes_fft[4].set_yscale('log')
    axes_fft[4].set_xlabel(r'$freq$')
    axes_fft[4].set_ylabel(r'$Mag$')
    axes_fft[4].grid(True)
    axes_fft[4].set_xlim([0.01,0.49])

            # fig.tight_layout()
            # fig.savefig(f"plots/{folder1}_vs_{folder2}/fft_{folder1}_vs_{folder2}_alpha_{alpha_1:.2f}.png", dpi=300)
            # plt.close(fig)


    #fig.tight_layout()
    fig.savefig(f"plots/neuron_dynamic/{folder_names[number]}/neuron_dynamic_{folder_names[number]}_for_precise_alpha={a:.2f}.png", dpi = 300) 
    axes_phase[0].cla()
    axes_phase[1].cla()
    axes_phase[2].cla()
    axes_phase[3].cla()
    axes_phase[4].cla()

    axes_time[0].cla()
    axes_time[1].cla()
    axes_time[2].cla()
    axes_time[3].cla()
    axes_time[4].cla()

    axes_fft[0].cla()
    axes_fft[1].cla()   
    axes_fft[2].cla()
    axes_fft[3].cla()
    axes_fft[4].cla()
