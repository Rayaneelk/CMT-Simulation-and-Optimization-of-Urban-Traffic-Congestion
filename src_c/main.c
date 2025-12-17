#include "config_kv.h"
#include "sim.h"
#include <stdio.h>
#include <string.h>

static const char* get_arg(int argc, char** argv, const char* key, const char* def) {
    for (int i=1;i<argc-1;i++) {
        if (strcmp(argv[i], key)==0) return argv[i+1];
    }
    return def;
}

int main(int argc, char** argv) {
    const char* cfg_path = get_arg(argc, argv, "--config", NULL);
    const char* out_dir  = get_arg(argc, argv, "--out", ".");

    if (!cfg_path) {
        fprintf(stderr, "Usage: %s --config path/to/config.kv --out out_dir\n", argv[0]);
        return 1;
    }

    Config cfg;
    if (config_load_kv(&cfg, cfg_path) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", cfg_path);
        return 1;
    }

    int rc = sim_run(&cfg, out_dir);
    if (rc != 0) {
        fprintf(stderr, "Simulation failed.\n");
        return 1;
    }
    return 0;
}
