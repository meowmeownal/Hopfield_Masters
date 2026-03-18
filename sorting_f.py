import numpy as np 
import matplotlib.pyplot as plt 
import sys
import math
import re

def extract_alfa(path):

    match = re.search(r'_(m|p)(\d+\.\d+)\.out$', path)
    if not match:
        raise ValueError(f"cant read alpha from: {path}")

    sign, value = match.groups()
    alfa = float(value)
    return -alfa if sign == 'm' else alfa


data_paths_double = []
data_paths_float = []
with open('paths_to_data_double_og.txt') as f_double,\
    open('paths_to_data_float_og.txt') as f_float:

    for data_path, data_path2 in zip(f_double, f_float):
        data_path_stripped = data_path.strip()
        data_path2_stripped = data_path2.strip()

        data_paths_double.append(data_path_stripped)
        data_paths_float.append(data_path2_stripped)

data_paths_double_m = []
data_paths_double_p = []
data_paths_float_m = []
data_paths_float_p = []

for data_path_double, data_path_float in zip(data_paths_double, data_paths_float):
    if data_path_double[-12] == 'm':
        data_paths_double_m.append(data_path_double)
    elif data_path_double[-12] == 'p':
        data_paths_double_p.append(data_path_double)

    if data_path_float[-12] == 'm':
        data_paths_float_m.append(data_path_float)
    elif data_path_float[-12] == 'p':
        data_paths_float_p.append(data_path_float)

data_paths_double_m.sort(key=extract_alfa)
data_paths_double_p.sort(key=extract_alfa)
data_paths_float_m.sort(key=extract_alfa)
data_paths_float_p.sort(key=extract_alfa)

data_paths_double_sorted = []
data_paths_double_sorted.extend(data_paths_double_m)
data_paths_double_sorted.extend(data_paths_double_p)

data_paths_float_sorted = []
data_paths_float_sorted.extend(data_paths_float_m)
data_paths_float_sorted.extend(data_paths_float_p)

data_paths_float_sorted_cut = []
data_paths_double_sorted_cut = []
for data_path_d, data_path_f in zip(data_paths_double_sorted, data_paths_float_sorted):
    with open(data_path_d) as file_d,\
        open(data_path_f) as file_f:


        if data_path_d[-12] == 'm':
            alpha_d = -1.*float(data_path_d[-11:-4])
        else:
            alpha_d = float(data_path_d[-11:-4])

        if data_path_f[-12] == 'm':
            alpha_f = -1.*float(data_path_f[-11:-4])
        else:
            alpha_f = float(data_path_f[-11:-4])

        if alpha_f < 8.21 and alpha_f > 2.99:
            data_paths_float_sorted_cut.append(data_path_f)

        if alpha_d < 8.21 and alpha_d > 2.99:
            data_paths_double_sorted_cut.append(data_path_d)


for data_path_d, data_path_f in zip(data_paths_double_sorted_cut, data_paths_float_sorted_cut):
    with open('out_f.txt', 'a') as outputf,\
        open('out_d.txt', 'a') as outputd:

        outputf.write(data_path_f"\n")
        outputd.write(data_path_d"\n")

