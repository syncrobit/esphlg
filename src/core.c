
#include <user_interface.h>

#include "espgoodies/common.h"
#include "espgoodies/ota.h"

#include "common.h"
#include "core.h"


#define TASK_ID_UPDATE 1


static uint32 now;
static uint64 now_ms;
static uint64 now_us;

static bool   running = FALSE;


static void ICACHE_FLASH_ATTR core_task_handler(uint32 task_id, void *param);


void core_init(void) {
    system_task_set_handler(core_task_handler);
}

void core_start(void) {
    if (running) {
        return;
    }

    DEBUG_CORE("started");
    running = TRUE;
    system_task_schedule(TASK_ID_UPDATE, NULL);
}

void core_stop(void) {
    if (!running) {
        return;
    }

    DEBUG_CORE("stopped");
    running = FALSE;
}

void core_update(void) {
    /* Prevent updating and related logic during OTA */
    if (ota_current_state() == OTA_STATE_DOWNLOADING) {
        return;
    }

    now_us = system_uptime_us();
    now_ms = now_us / 1000;
    now = now_ms / 1000;

    // TODO implement me
}

void core_task_handler(uint32 task_id, void *param) {
    switch (task_id) {
        case TASK_ID_UPDATE: {
            if (running) {
                /* Schedule next polling */
                system_task_schedule(TASK_ID_UPDATE, NULL);
                core_update();
            }

            break;
        }
    }
}
