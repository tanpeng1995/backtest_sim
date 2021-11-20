# Build
`HDF5_HOME=/path/to/your/hdf5 make`

# Run
- `./sim case.cfg tuning.cfg`
- `case.cfg` describes the (simulated) test case. It will be given on site
- `tuning.cfg` is a tuning file that you should provide.

# Validation (Not required)
- `python validate.py case.cfg corr_mat.npy`. It will read the case config file, and output the correlation matrix to the corr_map file. (In fact it is not in the workflow of the contest, but you can use it to compare the result with the baseline tuning config)