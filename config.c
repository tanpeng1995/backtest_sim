#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ERROR_CODE 1

// First read X, then filter the trailing chars of this line
#define safe_read(file, X, placeholder) \
    do {   \
        if (fscanf(file, placeholder, X) != 1) goto exit;    \
        while(!feof(file)) if (fgetc(file) == '\n') break; \
    } while (0)

#define safe_read_int(file, X) \
    safe_read(file, &(X), "%d")

#define safe_read_double(file, X) \
    safe_read(file, &(X), "%f")

#define safe_read_string(file, X) \
    safe_read(file, X, "%s")

#define safe_read_uint64(file, X) \
    safe_read(file, &(X), "%ld")

int read_case_config(const char* config_filename, CaseConfig* cfg) {
    FILE* file;
    file = fopen(config_filename, "r");
    if (!file) {
        printf("Open case config failed\n");
        return ERROR_CODE;
    }
    safe_read_string(file, cfg->filename);
    safe_read_string(file, cfg->dataname);
    safe_read_int(file, cfg->num_alphas);
    safe_read_int(file, cfg->num_ticks);
    safe_read_int(file, cfg->num_instr);
    safe_read_uint64(file, cfg->calc_time_per_tick);
    fclose(file);
    return 0;
exit:
    fclose(file);
    return ERROR_CODE;
}


int read_tuning_config(const char* config_filename, TuningConfig *cfg) {
    FILE* file;
    file = fopen(config_filename, "r");
    if (!file) {
        printf("Open tuning config failed\n");
        return ERROR_CODE;
    }
    // Read meta cache size
    safe_read_int(file, cfg->cache_min_size);
    safe_read_int(file, cfg->cache_max_size);
    safe_read_int(file, cfg->cache_initial_size);

    // Read compression
    safe_read_int(file, cfg->use_compression);

    // Dump mode
    safe_read_int(file, cfg->mode);
    safe_read_int(file, cfg->dump_per_tick);
    fclose(file);
    return 0;
exit:
    fclose(file);
    return ERROR_CODE;
}