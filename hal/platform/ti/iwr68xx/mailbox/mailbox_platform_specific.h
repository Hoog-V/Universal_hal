/**
* \file            mailbox_platform_specific.h
* \brief           Include file with peripheral specific settings for the Mailbox module
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
#ifndef MAILBOX_PLATFORM_SPECIFIC
#define MAILBOX_PLATFORM_SPECIFIC

#include "error_handling.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief The IWR68xx MSS has two inter-core mailbox links: one to the BSS
 *        (radar front-end/RF core) and one to the DSS (C674x DSP core).
 */
typedef enum {
    MAILBOX_PERIPHERAL_MSS_BSS,
    MAILBOX_PERIPHERAL_MSS_DSS
} mailbox_peripheral_t;

/**
 * @brief Maximum single-message size. Matches the mmWave SDK's
 *        MAILBOX_DATA_BUFFER_SIZE (ti/drivers/mailbox/mailbox.h) -- the
 *        underlying shared-memory region is 2048 bytes per direction, of
 *        which the SDK reserves 4 bytes for an optional multi-channel
 *        header this driver doesn't implement (see mailbox_iwr68xx.c);
 *        kept at the same conservative 2044 regardless, so message
 *        layouts stay interchangeable with SDK-side code on the BSS/DSS.
 */
#define MAILBOX_MAX_MESSAGE_SIZE 2044U

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MAILBOX_PLATFORM_SPECIFIC */
