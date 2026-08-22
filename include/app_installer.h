#ifndef REPAIRBOX_XMB_APP_INSTALLER_H
#define REPAIRBOX_XMB_APP_INSTALLER_H

#include <tamtypes.h>

#include "storage.h"

#define APP_COUNT 4
#define APP_FILE_COUNT 6

typedef struct app_package_file {
    const char *relative_path;
    u64 size;
    const char *sha256;
} app_package_file_t;

typedef struct app_package {
    const char *id;
    const char *title;
    const char *partition_name;
    const char *source_root;
    const app_package_file_t *files;
} app_package_t;

extern const app_package_t app_packages[APP_COUNT];

typedef struct app_file_diagnostic {
    char relative_path[64];
    u64 expected_size;
    char expected_sha256[65];
    int source_hash_result;
    u64 source_size;
    char source_sha256[65];
    int source_valid;
    int source_open_result;
    int destination_open_result;
    int source_read_result;
    int destination_write_result;
    int source_close_result;
    int destination_close_result;
    int target_hash_result;
    u64 target_size;
    char target_sha256[65];
    int target_valid;
} app_file_diagnostic_t;

typedef struct app_preflight {
    const app_package_t *package;
    int hdd_status;
    int format_version;
    int hdd_open_result;
    int hdd_close_result;
    int required_partitions_valid;
    int target_exists;
    u32 target_start;
    u32 target_length;
    int package_valid;
    unsigned int package_files_valid;
    u64 package_bytes;
    int failure_result;
    char failure_item[96];
    app_file_diagnostic_t files[APP_FILE_COUNT];
} app_preflight_t;

typedef struct app_install_result {
    const app_package_t *package;
    int success;
    int failed_step;
    int failure_result;
    char failure_operation[64];
    char failure_item[96];
    int target_was_created;
    int target_was_reformatted;
    u32 target_start;
    u32 target_length;
    unsigned int copied_files;
    unsigned int verified_files;
    u64 copied_bytes;
    app_file_diagnostic_t files[APP_FILE_COUNT];
} app_install_result_t;

typedef void (*app_progress_callback_t)(unsigned int step,
                                        unsigned int step_count,
                                        const char *operation,
                                        unsigned int percent,
                                        void *context);

void app_scan_preflight(const app_package_t *package,
                        app_preflight_t *result);
void app_install(const app_package_t *package,
                 psx_revision_t revision,
                 const app_preflight_t *preflight,
                 app_install_result_t *result,
                 app_progress_callback_t progress,
                 void *context);

#endif
