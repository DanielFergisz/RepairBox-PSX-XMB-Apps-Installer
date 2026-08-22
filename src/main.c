#include <debug.h>
#include <delaythread.h>
#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <stdio.h>
#include <string.h>
#include <timer.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

#include "app_installer.h"
#include "storage.h"
#include "system_version.h"
#include "ui.h"

#define PROGRAM_TITLE "RepairBox.pl PSX XMB Apps Installer v1.0"
#define USB_PACKAGE_ROOT "mass:/PSX_XMB_Apps"
#define USB_WAIT_TIMEOUT_MS 20000u
#define USB_POLL_INTERVAL_MS 250u
#define APP_STEP_COUNT 6u
#define PROGRESS_REFRESH_MS 500u
#define PROGRESS_LINE_WIDTH 54

typedef struct pad_diagnostics {
    int init_result;
    int open_result;
} pad_diagnostics_t;

typedef struct progress_state {
    psx_revision_t revision;
    unsigned int app_index;
    unsigned int last_app_index;
    unsigned int last_step;
    unsigned int last_percent;
    u64 start_time;
    u32 last_draw_ms;
    int screen_ready;
} progress_state_t;

static char pad_buffer[256] __attribute__((aligned(64)));

static const char *revision_title(psx_revision_t revision)
{
    return revision == PSX_REVISION_1
        ? "PSX1 - First Revision"
        : "PSX2 - Second Revision";
}

static void initialize_pad(pad_diagnostics_t *pad)
{
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    pad->init_result = padInit(0);
    pad->open_result = padPortOpen(0, 0, pad_buffer);
}

static void draw_storage_loading(void)
{
    ui_begin();
    ui_printf(PROGRAM_TITLE "\n\n");
    ui_inverse_status("LOADING STORAGE MODULES");
    ui_printf("Preparing read-only system detection.\n");
    ui_printf("No disk write has started.\n");
    ui_sync();
}

static void draw_revision_detection(void)
{
    ui_begin();
    ui_printf(PROGRAM_TITLE "\n\n");
    ui_inverse_status("DETECTING PSX SYSTEM REVISION");
    ui_printf("Reading __system/version.txt\n");
    ui_printf("No disk write has started.\n");
    ui_sync();
}

static void draw_detection_failed(const system_version_result_t *version,
                                  int version_result,
                                  int storage_result)
{
    ui_begin();
    ui_printf(PROGRAM_TITLE "\n\n");
    if (storage_result < 0) {
        ui_inverse_status("STOP - STORAGE MODULE STARTUP FAILED");
        ui_printf("Storage modules could not start.\n");
        ui_printf("Result: %d\n", storage_result);
    } else {
        ui_inverse_status("STOP - REVISION DETECTION FAILED");
        ui_printf("Cannot read a supported __system/version.txt.\n");
        ui_printf("Result: %d  mount=%d open=%d read=%d\n",
                  version_result, version->mount_result,
                  version->open_result, version->read_result);
        ui_printf("Detected text: %s\n", version->text);
    }
    ui_printf("No disk write was attempted.\n");
    ui_set_position(UI_SAFE_LEFT, 196);
    ui_printf("X/O Exit");
    ui_sync();
}

static void wait_for_exit(void)
{
    struct padButtonStatus buttons;
    unsigned int previous = 0;

    for (;;) {
        int state = padGetState(0, 0);

        if ((state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) &&
            padRead(0, 0, &buttons) != 0) {
            unsigned int current = 0xffffu ^ buttons.btns;
            unsigned int pressed = current & ~previous;

            previous = current;
            if ((pressed & (PAD_CROSS | PAD_CIRCLE)) != 0)
                return;
        }
        DelayThread(16000);
    }
}

static int package_usb_available(void)
{
    int fd = fileXioDopen(USB_PACKAGE_ROOT);

    if (fd < 0)
        return 0;
    return fileXioDclose(fd) >= 0;
}

static int wait_for_usb(psx_revision_t revision)
{
    unsigned int elapsed = 0;

    for (;;) {
        if (package_usb_available())
            return 1;
        ui_begin();
        ui_printf(PROGRAM_TITLE "\n%s\n\n", revision_title(revision));
        ui_inverse_status("WAITING FOR USB PACKAGE");
        ui_printf("Required directory:\n%s\n\n", USB_PACKAGE_ROOT);
        ui_printf("Elapsed: %u / %u ms\n", elapsed,
                  USB_WAIT_TIMEOUT_MS);
        ui_sync();
        if (elapsed >= USB_WAIT_TIMEOUT_MS)
            return 0;
        DelayThread(USB_POLL_INTERVAL_MS * 1000u);
        elapsed += USB_POLL_INTERVAL_MS;
    }
}

static void draw_loading(psx_revision_t revision, const char *status)
{
    ui_begin();
    ui_printf(PROGRAM_TITLE "\n%s\n\n", revision_title(revision));
    ui_inverse_status(status);
    ui_printf("No disk write has started.\n");
    ui_sync();
}

static int all_packages_ready(const app_preflight_t *preflight)
{
    unsigned int index;

    for (index = 0; index < APP_COUNT; ++index) {
        if (!preflight[index].required_partitions_valid ||
            !preflight[index].package_valid)
            return 0;
    }
    return 1;
}

static void scan_packages(app_preflight_t *preflight)
{
    unsigned int index;

    for (index = 0; index < APP_COUNT; ++index)
        app_scan_preflight(&app_packages[index], &preflight[index]);
}

static void draw_preflight(psx_revision_t revision,
                           const system_version_result_t *version,
                           int storage_init_result,
                           const app_preflight_t *preflight)
{
    unsigned int index;
    int ready = storage_init_result >= 0 &&
                all_packages_ready(preflight);

    ui_begin();
    ui_printf(PROGRAM_TITLE "\n%s\n", revision_title(revision));
    ui_printf("System version: %s (AUTO)\n\n", version->text);
    for (index = 0; index < APP_COUNT; ++index) {
        const app_preflight_t *item = &preflight[index];

        ui_printf("%-18s %s  %s\n", app_packages[index].title,
                  item->package_valid ? "PASS" : "FAIL",
                  item->target_exists ? "UPDATE / REPAIR" : "NEW");
    }
    ui_printf("\n");
    if (ready) {
        ui_inverse_status("READY - FOUR XMB APPLICATIONS");
        ui_printf("Four fixed 128 MiB application slots.\n");
        ui_printf("Hold L1 + R1 and press X to install.\n");
    } else {
        int error = storage_init_result;
        const char *item = "storage modules";

        for (index = 0; index < APP_COUNT && storage_init_result >= 0;
             ++index) {
            if (!preflight[index].required_partitions_valid ||
                !preflight[index].package_valid) {
                error = preflight[index].failure_result;
                item = preflight[index].failure_item;
                break;
            }
        }
        ui_inverse_status("STOP - CHECKING FAILED");
        ui_printf("Error: %d  Item: %s\n", error, item);
    }
    ui_set_position(UI_SAFE_LEFT, 196);
    ui_printf("TRIANGLE Rescan USB     O Exit");
    ui_sync();
}

static u32 progress_elapsed_ms(const progress_state_t *state)
{
    u32 seconds;
    u32 microseconds;

    TimerBusClock2USec(GetTimerSystemTime() - state->start_time,
                      &seconds, &microseconds);
    return seconds * 1000u + microseconds / 1000u;
}

static void progress_line(int y, const char *text)
{
    ui_set_position(UI_SAFE_LEFT, y);
    ui_printf("%-*.*s", PROGRESS_LINE_WIDTH, PROGRESS_LINE_WIDTH, text);
}

static void draw_progress(unsigned int step, unsigned int step_count,
                          const char *operation, unsigned int percent,
                          void *context)
{
    progress_state_t *state = (progress_state_t *)context;
    unsigned int work;
    unsigned int overall;
    u32 elapsed;
    int app_changed;
    char line[128];

    if (step == state->last_step && percent == state->last_percent)
        return;
    work = state->app_index * APP_STEP_COUNT * 100u +
           (step - 1u) * 100u + percent;
    overall = work / (APP_COUNT * APP_STEP_COUNT);
    elapsed = progress_elapsed_ms(state);
    app_changed = state->app_index != state->last_app_index;
    if (state->screen_ready && !app_changed && overall < 100u &&
        elapsed - state->last_draw_ms < PROGRESS_REFRESH_MS)
        return;

    if (!state->screen_ready) {
        ui_begin();
        ui_printf(PROGRAM_TITLE "\n%s\n\n",
                  revision_title(state->revision));
        ui_inverse_status("INSTALLING - DO NOT POWER OFF");
        state->screen_ready = 1;
    }
    snprintf(line, sizeof(line), "Application %u / %u: %s",
             state->app_index + 1u, APP_COUNT,
             app_packages[state->app_index].title);
    progress_line(88, line);
    snprintf(line, sizeof(line), "Step %u / %u: %s",
             step, step_count, operation);
    progress_line(112, line);
    snprintf(line, sizeof(line), "Application progress: %u%%", percent);
    progress_line(144, line);
    snprintf(line, sizeof(line), "Overall: %u%%   Elapsed: %02u:%02u",
             overall, elapsed / 60000u, (elapsed / 1000u) % 60u);
    progress_line(168, line);
    ui_sync();
    state->last_app_index = state->app_index;
    state->last_step = step;
    state->last_percent = percent;
    state->last_draw_ms = elapsed;
}

static void draw_result(psx_revision_t revision,
                        const app_install_result_t *install,
                        unsigned int completed,
                        unsigned int failed_index)
{
    unsigned int index;
    unsigned int copied = 0;
    unsigned int verified = 0;

    ui_begin();
    ui_printf(PROGRAM_TITLE "\n%s\n\n", revision_title(revision));
    if (completed == APP_COUNT) {
        ui_inverse_status("XMB APPLICATION INSTALLATION COMPLETE");
        for (index = 0; index < APP_COUNT; ++index) {
            copied += install[index].copied_files;
            verified += install[index].verified_files;
            ui_printf("%-18s PASS\n", app_packages[index].title);
        }
        ui_printf("Files: %u copied, %u verified\n", copied, verified);
        ui_inverse_status("FULL POWER OFF REQUIRED");
        ui_printf("Power off and disconnect AC power.\n");
        ui_printf("Reconnect, start PSX, then open Games.\n");
    } else {
        const app_install_result_t *failure = &install[failed_index];

        ui_inverse_status("INSTALLATION FAILED - STOPPED");
        ui_printf("Application %u / %u: %s\n", failed_index + 1u,
                  APP_COUNT, app_packages[failed_index].title);
        ui_printf("Step: %d / %u   Error: %d\n",
                  failure->failed_step, APP_STEP_COUNT,
                  failure->failure_result);
        ui_printf("Operation: %s\n", failure->failure_operation);
        ui_printf("Item: %s\n", failure->failure_item);
        ui_printf("Run v1.0 again to repair all slots.\n");
    }
    ui_draw_repairbox_logo(408, 176);
    ui_set_position(UI_SAFE_LEFT, 196);
    ui_printf("X/O Exit");
    ui_sync();
}

static int confirmation_chord(unsigned int held, unsigned int pressed)
{
    return (held & (PAD_L1 | PAD_R1)) == (PAD_L1 | PAD_R1) &&
           (pressed & PAD_CROSS) != 0;
}

static void run_installer(psx_revision_t revision,
                          const system_version_result_t *version,
                          int storage_init_result)
{
    static app_preflight_t preflight[APP_COUNT];
    static app_install_result_t install[APP_COUNT];
    struct padButtonStatus buttons;
    unsigned int previous = 0;
    unsigned int completed = 0;
    unsigned int failed_index = 0;
    int finished = 0;

    wait_for_usb(revision);
    draw_loading(revision, "CHECKING SYSTEM AND FOUR PACKAGES");
    memset(preflight, 0, sizeof(preflight));
    if (storage_init_result >= 0)
        scan_packages(preflight);
    draw_preflight(revision, version, storage_init_result, preflight);

    for (;;) {
        int state = padGetState(0, 0);

        if ((state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) &&
            padRead(0, 0, &buttons) != 0) {
            unsigned int current = 0xffffu ^ buttons.btns;
            unsigned int pressed = current & ~previous;

            previous = current;
            if (!finished && storage_init_result >= 0 &&
                all_packages_ready(preflight) &&
                confirmation_chord(current, pressed)) {
                unsigned int index;
                progress_state_t progress;

                memset(install, 0, sizeof(install));
                memset(&progress, 0, sizeof(progress));
                progress.revision = revision;
                progress.last_app_index = APP_COUNT;
                progress.last_percent = 101u;
                progress.start_time = GetTimerSystemTime();
                for (index = 0; index < APP_COUNT; ++index) {
                    progress.app_index = index;

                    app_install(&app_packages[index], revision,
                                &preflight[index], &install[index],
                                draw_progress, &progress);
                    if (!install[index].success) {
                        failed_index = index;
                        break;
                    }
                    ++completed;
                }
                finished = 1;
                draw_result(revision, install, completed, failed_index);
            } else if (!finished && (pressed & PAD_TRIANGLE) != 0 &&
                       storage_init_result >= 0) {
                draw_loading(revision, "RESCANNING USB PACKAGES");
                wait_for_usb(revision);
                memset(preflight, 0, sizeof(preflight));
                scan_packages(preflight);
                draw_preflight(revision, version, storage_init_result,
                               preflight);
            } else if ((pressed & PAD_CIRCLE) != 0 ||
                       (finished && (pressed & PAD_CROSS) != 0)) {
                return;
            }
        }
        DelayThread(16000);
    }
}

int main(int argc, char **argv)
{
    pad_diagnostics_t pad;
    static system_version_result_t version;
    static storage_result_t storage;
    int storage_result;
    int version_result;

    (void)argc;
    (void)argv;
    init_scr();
    ui_init();
    initialize_pad(&pad);
    draw_storage_loading();
    storage_result = storage_initialize_existing_system(&storage);
    version_result = storage_result;
    if (storage_result >= 0) {
        draw_revision_detection();
        version_result = system_version_detect(&version);
    }
    if (version_result >= 0 && version.revision != PSX_REVISION_NONE)
        run_installer(version.revision, &version, storage_result);
    else {
        draw_detection_failed(&version, version_result, storage_result);
        wait_for_exit();
    }
    padPortClose(0, 0);
    return 0;
}
