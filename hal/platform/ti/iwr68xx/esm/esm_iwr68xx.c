/**
* \file            esm_iwr68xx.c
* \brief           Source file which implements the standard ESM API functions
*/
/*
*  Copyright 2024 (C) Victor Hogeweij <hogeweyv@gmail.com>
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
* This file is part of the Universal Hal Framework.
*
* Author:          Victor Hogeweij <hogeweyv@gmail.com>
*/
#include <esm/esm_platform_specific.h>
#include <hal_esm.h>
#include <AWR6843.h>
#include <AWR6843_ESM.h>

/* SYS/BIOS's R4F VIM Hwi module -- ported from the mmWave SDK's
 * ESM_init()/ESM_highpriority_FIQ()/ESM_lowpriority_IRQ() (esm.c), which
 * register the MSS ESM error lines through exactly this API. Unlike
 * gpio/pinmux/uart/soc, this module is inherently RTOS-coupled: the R4F VIM
 * FIQ/IRQ lines are dispatched by SYS/BIOS's Hwi_create(), there's no raw
 * register alternative that doesn't reimplement SYS/BIOS's own vector
 * table hookup. */
#include <ti/sysbios/family/arm/v7r/vim/Hwi.h>
#include <xdc/runtime/Error.h>

static esm_notify_params_t notify_params[ESM_MAX_NOTIFIERS];

static void esm_process_interrupt(uint32_t vec) {
    uint32_t group_number;
    uint32_t error_number;
    uint32_t index;
    uint32_t recognized;

    recognized = 1U;
    if (vec < 32U) {
        /* Group 1, errors 0-31 -- write-1-to-clear, only this bit. */
        ESM->ESMSR1 = (1UL << vec);
        group_number = 1U;
        error_number = vec;
    } else if (vec < 64U) {
        /* Group 2, errors 0-31. */
        vec -= 32U;
        ESM->ESMSR2 = (1UL << vec);
        group_number = 2U;
        error_number = vec;
    } else if (vec < 96U) {
        /* Group 1, errors 32-63. */
        vec -= 64U;
        ESM->ESMSR4 = (1UL << vec);
        group_number = 1U;
        error_number = vec + 32U;
    } else {
        recognized = 0U;
        group_number = 0U;
        error_number = 0U;
    }

    if (recognized != 0U) {
        for (index = 0U; index < ESM_MAX_NOTIFIERS; index++) {
            if ((notify_params[index].notify != (esm_callback_t)0) &&
                (notify_params[index].error_number == error_number) &&
                (notify_params[index].group_number == group_number)) {
                notify_params[index].notify(notify_params[index].arg);
                break;
            }
        }
    }
}

/* Group 3 errors don't raise a FIQ/IRQ and so never reach here -- same as
 * the SDK's ESM_highpriority_FIQ/ESM_lowpriority_IRQ. */
void interrupt esm_high_priority_fiq(void) {
    uint32_t vec;
    vec = ESM->ESMIOFFHR - 1U;
    esm_process_interrupt(vec);
}

void esm_low_priority_irq(xdc_UArg arg) {
    uint32_t vec;
    (void)arg;
    vec = ESM->ESMIOFFLR - 1U;
    esm_process_interrupt(vec);
}

uhal_status_t esm_init(uint8_t clear_pending_errors) {
    Hwi_Params hwi_params;
    Error_Block error_block;
    Hwi_Handle hwi_handle;
    uint32_t status;

    Error_init(&error_block);

    Hwi_Params_init(&hwi_params);
    hwi_params.type = Hwi_Type_FIQ;
    hwi_handle = Hwi_create(SOC_XWR68XX_MSS_ESM_HIGH_PRIORITY_INT,
                             (Hwi_FuncPtr)esm_high_priority_fiq, &hwi_params, &error_block);
    if (hwi_handle == NULL) {
        return UHAL_STATUS_ERROR;
    }

    Hwi_Params_init(&hwi_params);
    hwi_params.type = Hwi_Type_IRQ;
    hwi_handle = Hwi_create(SOC_XWR68XX_MSS_ESM_LOW_PRIORITY_INT,
                             (Hwi_FuncPtr)esm_low_priority_irq, &hwi_params, &error_block);
    if (hwi_handle == NULL) {
        return UHAL_STATUS_ERROR;
    }

    if (clear_pending_errors != 0U) {
        status = ESM->ESMSR1;
        ESM->ESMSR1 = status;
        status = ESM->ESMSR2;
        ESM->ESMSR2 = status;
        status = ESM->ESMSR3;
        ESM->ESMSR3 = status;
        status = ESM->ESMSR4;
        ESM->ESMSR4 = status;
        status = ESM->ESMSSR2;
        ESM->ESMSSR2 = status;
    }

    return UHAL_STATUS_OK;
}

uhal_status_t esm_register_notifier(const esm_notify_params_t *params, int32_t *notify_index) {
    xdc_UInt key;
    int32_t index;
    uhal_status_t result;

    if ((params == (const esm_notify_params_t *)0) || (notify_index == (int32_t *)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    key = Hwi_disable();

    result = UHAL_STATUS_ERROR;
    for (index = 0; index < (int32_t)ESM_MAX_NOTIFIERS; index++) {
        if (notify_params[index].notify == (esm_callback_t)0) {
            break;
        }
    }

    if (index < (int32_t)ESM_MAX_NOTIFIERS) {
        /* Group 2 errors are unmasked by default; group 1 must be
         * explicitly unmasked here (same as ESM_registerNotifier()). */
        if (params->group_number == 1U) {
            if (params->error_number < 32U) {
                ESM->ESMIESR1 |= (1UL << params->error_number);
            } else if (params->error_number < 64U) {
                ESM->ESMIESR4 |= (1UL << (params->error_number - 32U));
            }
        }
        notify_params[index] = *params;
        *notify_index = index;
        result = UHAL_STATUS_OK;
    }

    Hwi_restore(key);
    return result;
}

uhal_status_t esm_deregister_notifier(int32_t notify_index) {
    xdc_UInt key;
    uint32_t group_number;
    uint32_t error_number;

    if ((notify_index < 0) || (notify_index >= (int32_t)ESM_MAX_NOTIFIERS)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    key = Hwi_disable();

    /* ESMIECR1/4 are write-1-to-clear (matches ESMIESR1/4's write-1-to-set,
     * the same set/clear register-pair convention this chip's GIO ENASET/
     * ENACLR use) -- deliberately deviates from the mmWave SDK's
     * ESM_deregisterNotifier(), which writes a 0 to the target bit via
     * CSL_FINSR() and leaves the interrupt enabled. That looks like a
     * latent bug in the reference driver, not an intentional behavior; not
     * observable from this project (nothing here ever deregisters a group 1
     * notifier), but there's no reason to carry the bug forward. */
    group_number = notify_params[notify_index].group_number;
    error_number = notify_params[notify_index].error_number;
    if (group_number == 1U) {
        if (error_number < 32U) {
            ESM->ESMIECR1 |= (1UL << error_number);
        } else if (error_number < 64U) {
            ESM->ESMIECR4 |= (1UL << (error_number - 32U));
        }
    }
    notify_params[notify_index].group_number = 0U;
    notify_params[notify_index].error_number = 0U;
    notify_params[notify_index].arg = (void *)0;
    notify_params[notify_index].notify = (esm_callback_t)0;

    Hwi_restore(key);
    return UHAL_STATUS_OK;
}
