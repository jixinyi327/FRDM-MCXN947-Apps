/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "osc_shared.h"

#include <string.h>

#include "fsl_common.h"

osc_shared_state_t *osc_shared_get(void)
{
    return (osc_shared_state_t *)(void *)__RPMSG_SH_MEM_START__;
}

void osc_shared_initialize(void)
{
    osc_shared_state_t *shared = osc_shared_get();

    memset(shared, 0, sizeof(*shared));
    shared->version = OSC_SHARED_VERSION;
    shared->data.sample_rate_hz = OSC_SAMPLE_RATE_HZ;
    __DMB();
    shared->magic = OSC_SHARED_MAGIC;
    __DMB();
}

void osc_shared_publish(const osc_snapshot_t *snapshot)
{
    osc_shared_state_t *shared = osc_shared_get();
    uint32_t sequence = shared->sequence;

    if (0U != (sequence & 1U))
    {
        sequence++;
    }
    shared->sequence = sequence + 1U;
    __DMB();
    shared->data = *snapshot;
    __DMB();
    shared->sequence = sequence + 2U;
    __DMB();
}

bool osc_shared_read(osc_snapshot_t *snapshot)
{
    osc_shared_state_t *shared = osc_shared_get();
    uint32_t sequence_before;
    uint32_t sequence_after;
    uint32_t attempt;

    if ((OSC_SHARED_MAGIC != shared->magic) ||
        (OSC_SHARED_VERSION != shared->version))
    {
        return false;
    }

    for (attempt = 0U; attempt < 4U; attempt++)
    {
        sequence_before = shared->sequence;
        if (0U != (sequence_before & 1U))
        {
            continue;
        }
        __DMB();
        *snapshot = shared->data;
        __DMB();
        sequence_after = shared->sequence;
        if ((sequence_before == sequence_after) &&
            (0U == (sequence_after & 1U)))
        {
            return true;
        }
    }

    return false;
}
