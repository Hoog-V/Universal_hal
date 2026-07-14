/**
* \file            hal_soc.h
* \brief           SoC bring-up module include file
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
/**
 * The SOC module
 *
 * Unlike the other hal_*.h modules (one peripheral, several operations),
 * this one covers whatever chip-specific, one-time bring-up has to happen
 * before other peripherals are reliably usable at all -- clock domains that
 * need to be brought up, debug firewalls, and similar. What that actually
 * means is entirely platform dependent (some chips need none of this), so
 * the only guaranteed operation is soc_init(): call it once, as early as
 * possible, before touching any other peripheral module.
 */
#ifndef HAL_SOC_H
#define HAL_SOC_H
#ifndef DISABLE_SOC_MODULE

/* Extern c for compiling with c++*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <error_handling.h>
#include <soc/soc_platform_specific.h>

/**
 * @brief Performs whatever essential, one-time SoC bring-up this platform
 *        needs before other peripherals can be used reliably.
 * @return UHAL_STATUS_OK once bring-up is complete,
 *         UHAL_STATUS_PERIPHERAL_CLOCK_ERROR if a required clock never
 *         locked within a bounded wait.
 */
uhal_status_t soc_init(void);

/**
 * @brief IWR68xx only: releases the DSP (DSS) core from its post-download
 *        halt so it begins executing. On this chip the DSP power domain
 *        is OFF on POR (AWR6843 TRM s5.4.2) -- the bootloader powers it on
 *        and downloads its program when loading a multicore flash image,
 *        but leaves it halted; this call is what lets it actually run.
 *        Only meaningful for projects that flash a DSS image alongside
 *        this MSS one -- harmless but pointless otherwise. Call once,
 *        after soc_init(), before anything that expects DSS to respond
 *        (e.g. mailbox_init()/mailbox_write() to the DSS link).
 * @return UHAL_STATUS_OK once the DSP reports powered-on,
 *         UHAL_STATUS_ERROR if it never does within a bounded wait.
 */
uhal_status_t soc_unhalt_dss(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IFNDEF DISABLE_SOC_MODULE */
#endif /* HAL_SOC_H */
