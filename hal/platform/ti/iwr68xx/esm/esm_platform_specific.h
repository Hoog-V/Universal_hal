/**
* \file            esm_platform_specific.h
* \brief           Include file with peripheral specific settings for the ESM module
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
#ifndef ESM_PLATFORM_SPECIFIC
#define ESM_PLATFORM_SPECIFIC

#include "error_handling.h"
#include <stdint.h>

/**
 * @brief Matches the mmWave SDK's ESM_MAX_NOTIFIERS -- the IWR68xx MSS ESM
 *        driver this was ported from supports at most 4 concurrently
 *        registered notifiers.
 */
#define ESM_MAX_NOTIFIERS 4U

/**
 * @brief Callback invoked from esm_register_notifier()'s registered error
 *        interrupt context, after the driver has already cleared that
 *        error's status bit.
 */
typedef void (*esm_callback_t)(void *arg);

/**
 * @brief Which error to watch (group_number/error_number) and what to call
 *        when it fires. Group 1 errors must be explicitly unmasked (done by
 *        esm_register_notifier()); group 2 errors are unmasked by default.
 */
typedef struct {
    uint32_t group_number;
    uint32_t error_number;
    void *arg;
    esm_callback_t notify;
} esm_notify_params_t;

#endif /* ESM_PLATFORM_SPECIFIC */
