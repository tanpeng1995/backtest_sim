#include <unistd.h>
#include <stdint.h>

/*
#define N 2
#define T 105
#define ii 100
*/


#define STORE_DEBUG     0  // Baseline, do not use
#define STORE_CONCURRENT  1

// Tuning parameters

// #define CACHE_MIN_SIZE 1024
// #define CACHE_MAX_SIZE 16777216
// #define CACHE_INITIAL_SIZE 1048576

// #define USE_COMPRESSION 1

// #define MODE STORE_CONCURRENT

// // For (MODE == STORE_CONCURRENT)
// #define DUMP_PER_TICK 200

typedef struct CaseConfig{
    char filename[20];
    char dataname[20];
    int num_alphas;   // Number of alphas
    int num_ticks;   // number of Ticks of one round
    int num_instr;   // Number of instruments
    uint64_t calc_time_per_tick;    // for each tick, the (approximate) calculation time, count in ns.  
} CaseConfig;

typedef struct TuningConfig{
    CaseConfig* case_cfg;
    int cache_min_size;
    int cache_max_size;
    int cache_initial_size;
    int use_compression;
    int mode;
    int dump_per_tick;
} TuningConfig;

int read_case_config(const char* config_filename, CaseConfig* cfg);

int read_tuning_config(const char* config_filename, TuningConfig *cfg);
