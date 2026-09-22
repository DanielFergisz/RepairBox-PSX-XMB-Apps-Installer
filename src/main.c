#include <debug.h>
#include <delaythread.h>
#include <errno.h>
#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <string.h>

#include "apps/apps_ui.h"
#include "build_profile.h"
#include "iop_module_lookup.h"
#include "source_media.h"
#include "ui.h"

typedef struct pad_diagnostics {
    int init_result;
    int open_result;
} pad_diagnostics_t;

static char pad_buffer[256] __attribute__((aligned(64)));
static unsigned char pad_start_stack[16384] __attribute__((aligned(64)));
static volatile int pad_start_stage;
static volatile int pad_start_done;

static void draw_startup(const char *status)
{
    ui_begin();
    ui_printf(RBX_PROGRAM_TITLE "\n\n");
    ui_inverse_status(status);
    ui_printf("Please wait.\n");
    ui_sync();
}

static void initialize_pad(pad_diagnostics_t *pad)
{
    int result = source_media_prepare_controller_stack(&pad_start_stage);

    if (result < 0) {
        pad->init_result = result;
        return;
    }
    if (iop_module_find("sio2man") < 0) {
        pad_start_stage = 1;
        SifLoadModule("rom0:SIO2MAN", 0, NULL);
    }
    if (iop_module_find("padman") < 0) {
        pad_start_stage = 2;
        SifLoadModule("rom0:PADMAN", 0, NULL);
    }
    pad_start_stage = 3;
    pad->init_result = padInit(0);
    if (pad->init_result >= 0) {
        pad_start_stage = 4;
        pad->open_result = padPortOpen(0, 0, pad_buffer);
    }
}

static void pad_start_worker(void *context)
{
    initialize_pad((pad_diagnostics_t *)context);
    pad_start_done = 1;
    ExitDeleteThread();
}

static int initialize_pad_with_status(pad_diagnostics_t *pad)
{
    ee_thread_t thread;
    ee_thread_status_t current_thread;
    int thread_id;
    unsigned int elapsed;

    pad->init_result = -1;
    pad->open_result = -1;
    pad_start_done = 0;
    pad_start_stage = 0;
    memset(&thread, 0, sizeof(thread));
    thread.func = pad_start_worker;
    thread.stack = pad_start_stack;
    thread.stack_size = sizeof(pad_start_stack);
    thread.gp_reg = &_gp;
    if (ReferThreadStatus(GetThreadId(), &current_thread) < 0 ||
        current_thread.current_priority >= 127)
        return -EIO;
    thread.initial_priority = current_thread.current_priority + 1;
    thread_id = CreateThread(&thread);
    if (thread_id < 0)
        return thread_id;
    if (StartThread(thread_id, pad) < 0) {
        DeleteThread(thread_id);
        return -EIO;
    }
    for (elapsed = 0; elapsed < 1000u; ++elapsed) {
        if (pad_start_done)
            return pad->init_result >= 0 && pad->open_result > 0 ? 0 : -EIO;
        DelayThread(10000);
    }
    /* The worker may still own an RPC buffer. Never terminate it here. */
    return -ETIMEDOUT;
}

int main(int argc, char **argv)
{
    pad_diagnostics_t pad;
    int pad_result;

    init_scr();
    ui_init();
    draw_startup("STARTING");
    SifInitRpc(0);
    source_media_detect_preferred(argc, argv);
    pad_result = initialize_pad_with_status(&pad);
    if (pad_result < 0) {
        draw_startup(pad_result == -ETIMEDOUT
                         ? "CONTROLLER START TIMEOUT"
                         : "CONTROLLER START FAILED");
        ui_printf("Error: %d, stage: %d\n", pad_result, pad_start_stage);
        ui_printf("Power off the console to exit.\n");
        ui_sync();
        for (;;)
            DelayThread(1000000);
    }
    (void)source_media_prepare_unrecognized();
    apps_ui_run();
    padPortClose(0, 0);
    return 0;
}
