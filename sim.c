#include "hdf5.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#include  "config.h"

double *alpha;

// H5 param

double calculate_alpha(int t, int n, int i) {
    return (t * n * i + t + n + i) % 10007;
}

volatile int last_prepared_tick = 0;
void *worker(void* _cfg) {
    CaseConfig* cfg = (CaseConfig*) _cfg;
    struct timeval tv_start, tv_end;
    struct timespec tick_start, tick_end;
    struct timespec total_start, total_end;
    size_t index = 0;
    gettimeofday(&tv_start, NULL);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &total_start);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tick_start);
    for (int t = 0; t < cfg->num_ticks; ++t) {
        // Simulation Time. Use a empty while-loop to get a high-resolution timer.
        // printf("index = %ld\n", index);
        while (1) {
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tick_end);
            if ( (tick_end.tv_nsec - tick_start.tv_nsec) + (tick_end.tv_sec - tick_start.tv_sec) 
                    * 1000000000l > cfg->calc_time_per_tick)
                break;
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tick_end);
        printf("%lu\n", (tick_end.tv_nsec - tick_start.tv_nsec) + (tick_end.tv_sec - tick_start.tv_sec) * 1000000000l);
        for (int n = 0; n < cfg->num_alphas; ++n) {
            for (int i = 0; i < cfg->num_instr; ++i) {
                alpha[index++] = calculate_alpha(t, n, i);
            }
        }
        last_prepared_tick = t;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tick_start);
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &total_end);
    printf("%lu\n", (total_end.tv_nsec - total_start.tv_nsec) 
            + (total_end.tv_sec - total_start.tv_sec) * 1000000000l);

    gettimeofday(&tv_end, NULL);
    printf("Calculate Time: %ld us\n",
          ((tv_end.tv_sec * 1000000 + tv_end.tv_usec) -
          (tv_start.tv_sec * 1000000 + tv_start.tv_usec)));
}

void* dump(void* _cfg) {
    TuningConfig *cfg = (TuningConfig *) _cfg;
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    // Init hdf5 file
    hid_t status;
    hid_t file_id;
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5AC_cache_config_t cache_config;
    cache_config.version = H5AC__CURR_CACHE_CONFIG_VERSION;
    H5Pget_mdc_config(fapl, &cache_config);
    cache_config.min_size = cfg->cache_min_size;
    cache_config.max_size = cfg->cache_max_size;
    cache_config.initial_size = cfg->cache_initial_size;
    cache_config.incr_mode = H5C_incr__off;
    cache_config.flash_incr_mode = H5C_flash_incr__off;
    cache_config.decr_mode = H5C_decr__off;
    H5Pset_mdc_config(fapl, &cache_config);


    int T = cfg->case_cfg->num_ticks;
    int ii = cfg->case_cfg->num_instr;
    int N = cfg->case_cfg->num_alphas;
    size_t alpha_size = (size_t) ii * T;

    file_id = H5Fcreate(cfg->case_cfg->filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

    if (cfg->mode == STORE_DEBUG) {  // Baseline, do not use.
        while (last_prepared_tick < cfg->case_cfg->num_ticks - 1) {
            usleep(1);
        }

        // Dump result to hdf5
        hsize_t dimsf[2] = {T, ii};
        hid_t dataspace_id = H5Screate_simple(2, dimsf, NULL);
        char name[32];
        for (int alpha_idx = 0; alpha_idx < N; ++alpha_idx) {
            sprintf(name, "%s_%d", cfg->case_cfg->dataname, alpha_idx);
            hid_t dataset_id = H5Dcreate2(file_id, name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            status = H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, alpha + alpha_size * alpha_idx);
        }
    } else if (cfg->mode == STORE_CONCURRENT) {
        int dump_per_tick = cfg->dump_per_tick;
        // init property
        hid_t cparams;
        cparams = H5Pcreate(H5P_DATASET_CREATE);
        hsize_t chunk_dims [2];
        chunk_dims[0] = dump_per_tick;
        chunk_dims[1] = ii;
        H5Pset_chunk(cparams, 2,  chunk_dims);

        if (cfg->use_compression) {
            H5Pset_deflate(cparams, 9);
        }
        // Create dataset
        hsize_t dims[2], maxdims[2] = {H5S_UNLIMITED, ii};
        dims[0] = T;//((T - 1)/dump_per_tick + 1) * dump_per_tick;
        dims[1] = ii;
        hid_t dataspace_id = H5Screate_simple(2, dims, dims);
        hid_t dataset_id[N], filespace_id[N];
        char name[32];
        hsize_t offset[2] = {0, 0};
        for (int idx = 0; idx < N; ++idx) {
            sprintf(name, "%s_%d", cfg->case_cfg->dataname, idx);
            dataset_id[idx] =  H5Dcreate2(file_id, name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, cparams, H5P_DEFAULT);
            H5Dset_extent(dataset_id[idx], dims);
            filespace_id[idx] = H5Dget_space(dataset_id[idx]);
        }

        int dumped_tick = 0;
        int next_dump_tick = dump_per_tick;
        while (dumped_tick < T - 1) {
            while (last_prepared_tick < next_dump_tick - 1) {
                usleep(1);
            }
            // dump [dumped_tick, next_dump_tick)

            chunk_dims[0] = next_dump_tick - dumped_tick;
            offset[0] = dumped_tick;
            hid_t filespace;
            for (int idx = 0; idx < N; ++idx) {
                hid_t filespace = filespace_id[idx];
                hid_t dataspace = H5Screate_simple(2, chunk_dims, NULL);
                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, chunk_dims, NULL);
                H5Dwrite(dataset_id[idx], H5T_NATIVE_DOUBLE, dataspace, filespace, H5P_DEFAULT, alpha + idx * alpha_size + offset[0] * ii);
            }

            dumped_tick = next_dump_tick;
            next_dump_tick += dump_per_tick;
            if (next_dump_tick >= T) next_dump_tick = T;
        }

        H5Sclose(dataspace_id);
        for (int idx = 0; idx < N; ++idx) {
            H5Dclose(dataset_id[idx]);
            H5Sclose(filespace_id[idx]);
        }    
    }
    double hit_rate;
    H5Fget_mdc_hit_rate(file_id, &hit_rate);
    printf("Hit rate = %lf\n", hit_rate);
    H5Pclose(fapl);
    H5Fflush(file_id, H5F_SCOPE_LOCAL);
    int ret = system("sync");
    H5Fclose(file_id);
    gettimeofday(&tv_end, NULL);
    printf("Dump Time: %ld us\n",
          ((tv_end.tv_sec * 1000000 + tv_end.tv_usec) -
          (tv_start.tv_sec * 1000000 + tv_start.tv_usec)));
}


int main(int argc, char* argv[]) {
    CaseConfig case_config;
    TuningConfig tuning_config;
    clockid_t clock = CLOCK_MONOTONIC;
    struct timespec tv_start, tv_end;
    clock_gettime(clock, &tv_start);

    if (argc < 3) {
        printf("Usage: %s [case.config] [tuning.config]\n", argv[0]);
        exit(1);
    }

    // Read configs
    int err = read_case_config(argv[1], &case_config);
    if (err) {
        printf("Read case config error! Check your case config file\n");
        exit(1);
    }
    err = read_tuning_config(argv[2], &tuning_config);
    if (err) {
        printf("Read tuning config error! Check your tuning config file\n");
        exit(1);
    }
    tuning_config.case_cfg = &case_config;

    // Allocate memory
    size_t size = (size_t)case_config.num_alphas * case_config.num_instr * case_config.num_ticks;
    alpha = (double*) malloc (size * sizeof(double));


    pthread_t worker_thread, dump_thread;
    int ret;
    ret = pthread_create( &worker_thread, NULL, worker, &case_config);
    ret = pthread_create( &dump_thread, NULL, dump, &tuning_config);

    pthread_join(worker_thread, NULL);
    pthread_join(dump_thread, NULL);

    clock_gettime(clock, &tv_end);
    double elapsed;
    elapsed = tv_end.tv_sec - tv_start.tv_sec;
    elapsed += (tv_end.tv_nsec - tv_start.tv_nsec) / 1000000000.0;
    printf("Wall time : %f s\n", elapsed);
    free(alpha);
    return 0;
}
