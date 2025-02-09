#
# @Date: 2024-12-10 12:05:12
# @LastEditors: jiangrd3 jiangrd3@mail2.sysu.edu.cn
# @LastEditTime: 2024-12-12 20:33:14
# @FilePath: /New_folder2/pp.py
#
PATH = "/mnt/c/jiangrd3/ad9361remaster/untitled1/build/Desktop_Qt_6_7_2_MSVC2019_64bit-Debug/csv"
import os
import csv
from matplotlib import pyplot as plt
import numpy as np

file = []
for i in os.listdir(PATH):
    if i.endswith('.csv'):
        file.append(i)
print(file)

ICAO = {}
for j in file:
    f = open(PATH + '/' + j, 'r')
    print(j)
    f_r = csv.reader(f)
    for i in f_r:
        # print(i)
        # print(len(i))
        if i[0] in ICAO.keys():
            if len(i) in ICAO[i[0]].keys():
                ICAO[i[0]][len(i)] = 1 + ICAO[i[0]][len(i)]
            else:
                ICAO[i[0]][len(i)] = 1
        else:
            
            ICAO[i[0]] = {len(i):1}
                
        pass

    pass
    f.close()
pass
for key in list(ICAO.keys()):
    if sum([ICAO[key][j] for j in ICAO[key].keys()])>60:
        pass
    else:
        del ICAO[key]
        # ICAO.pop(key)
    pass
del ICAO['FFFFFF']
f_w = open(PATH + '/' + 'combined.csv', 'w', newline='')
key_list = list(ICAO.keys())
for j in file:
    f = open(PATH + '/' + j, 'r')
    print(j)
    f_r = csv.reader(f)
    for i in f_r:
        # print(i)
        # print(len(i))
        if i[0] in key_list:
            f_w.write(','.join(i)+'\n')

    pass
    f.close()
pass
f_w.close()

