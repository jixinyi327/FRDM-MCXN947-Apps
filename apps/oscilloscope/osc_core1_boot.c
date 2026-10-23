/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "osc_core1_boot.h"

#include <string.h>

#include "fsl_common.h"
#include "mcmgr.h"
#include "osc_shared.h"

extern const uint8_t core1_image_start[];
extern uint32_t core1_image_size;
extern uint32_t __RPMSG_SH_MEM_START__[];

bool osc_core1_start(void)
{
    uint32_t image_size = core1_image_size;
    void *boot_address = (void *)(
        (uintptr_t)__RPMSG_SH_MEM_START__ + OSC_SHARED_MEMORY_SIZE);
    mcmgr_status_t status;

    if (0U == image_size)
    {
        return false;
    }

    memcpy(boot_address, core1_image_start, image_size);
    __DSB();
    __ISB();

    status = MCMGR_StartCore(
        kMCMGR_Core1, boot_address, 0U, kMCMGR_Start_Synchronous);
    return status == kStatus_MCMGR_Success;
}
