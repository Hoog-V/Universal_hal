/**
* \file            hal_esm.h
* \brief           ESM (Error Signaling Module) module include file
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
#ifndef HAL_ESM_H
#define HAL_ESM_H
#ifndef DISABLE_ESM_MODULE

/* Extern c for compiling with c++*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <error_handling.h>
#include <esm/esm_platform_specific.h>

/**
 * @brief Initializes the ESM (Error Signaling Module) driver: installs the
 *        platform's high/low priority error interrupt handlers, and
 *        optionally clears any error status left over from before init.
 * @param clear_pending_errors 0: leave pending error status alone (use this
 *        if the RTOS/boot path already clears it before your init code
 *        runs); 1: read and clear all error group status registers here.
 * @return UHAL_STATUS_OK once installed.
 */
uhal_status_t esm_init(uint8_t clear_pending_errors);

/**
 * @brief Registers a callback to be invoked when a specific ESM error
 *        fires, after the driver's own interrupt handler has cleared its
 *        status bit. Only group 1 and group 2 errors are supported (group 3
 *        errors don't raise an interrupt this driver handles).
 * @param params Group/error number to watch, and the callback + argument to
 *        invoke.
 * @param[out] notify_index Set to the registered slot's index on success;
 *        pass to esm_deregister_notifier() to remove it later.
 * @return UHAL_STATUS_OK on success, UHAL_STATUS_INVALID_PARAMETERS if
 *         params/notify_index is NULL, UHAL_STATUS_ERROR if all notifier
 *         slots are in use.
 */
uhal_status_t esm_register_notifier(const esm_notify_params_t *params, int32_t *notify_index);

/**
 * @brief Deregisters a notifier previously registered with
 *        esm_register_notifier().
 * @param notify_index Index returned by esm_register_notifier().
 * @return UHAL_STATUS_OK on success, UHAL_STATUS_INVALID_PARAMETERS if
 *         notify_index is out of range.
 */
uhal_status_t esm_deregister_notifier(int32_t notify_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IFNDEF DISABLE_ESM_MODULE */
#endif /* HAL_ESM_H */
