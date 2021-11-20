import h5py
import numpy as np
import pandas as pd
import scipy.stats
import sys
if (len(sys.argv) < 3):
    print(f"Usage: {sys.argv[0]} [case.cfg] [output numpy file]")
    exit(1)
with open(sys.argv[1], "r") as f:
    lines = f.readlines()
    h5filename = lines[0].split('#')[0].strip()
    h5alpha_prefix = lines[1].split('#')[0].strip()
    i = int(lines[2].split('#')[0].strip())
    j = int(lines[3].split('#')[0].strip())
    k = int(lines[4].split('#')[0].strip())
    shape = (i, j, k)
    print(h5filename, h5alpha_prefix, shape)
f = h5py.File(h5filename, 'r')
now_alpha = np.empty(shape)

num_instr = shape[0]
for i in range(num_instr):
    alpha_str = h5alpha_prefix + '_' + str(i)
    now_alpha[i, :] = f[alpha_str]
corr_mat = np.corrcoef(now_alpha.reshape(num_instr, -1), now_alpha.reshape(num_instr, -1))
np.save(argv[2], corr_mat)
