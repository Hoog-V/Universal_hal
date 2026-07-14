/**
* \file            hal_mailbox.h
* \brief           Mailbox (inter-processor doorbell) module include file
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
*
* Unlike the other hal_*.h modules, no other platform in this HAL has a
* Mailbox peripheral (it's specific to multi-core SoCs with independent
* CPUs sharing a package, like the AWR68xx's MSS/DSS/BSS cores) -- this
* generic API was authored from scratch for that use case rather than
* generalized from an existing platform, modeled loosely on the mmWave
* SDK's Mailbox_open/write/read/readFlush shape (adapted to this HAL's
* uhal_status_t returns and peripheral-enum-selection convention).
*/
#ifndef HAL_MAILBOX_H
#define HAL_MAILBOX_H

#ifndef DISABLE_MAILBOX_MODULE

#include <error_handling.h>
#include <stddef.h>
#include "mailbox/mailbox_platform_specific.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Function to initialize a mailbox link to the given remote core.
 * @param mailbox_peripheral Which inter-core link to open (e.g. MSS<->BSS, MSS<->DSS)
 * @return UHAL_STATUS_OK when no errors have occurred
 */
uhal_status_t mailbox_init(const mailbox_peripheral_t mailbox_peripheral);

/**
 * @brief Function to send a message to the remote core, then wait for it to
 *        acknowledge having read the previous message this link sent (if
 *        any is still pending) before writing the new one.
 * @param mailbox_peripheral Which inter-core link to send on
 * @param write_buff Pointer to a buffer containing the message to send
 * @param size The amount of bytes to send -- must not exceed
 *             MAILBOX_MAX_MESSAGE_SIZE (mailbox_platform_specific.h)
 * @return UHAL_STATUS_OK when no errors have occurred
 */
uhal_status_t mailbox_write(const mailbox_peripheral_t mailbox_peripheral, const uint8_t* write_buff, const size_t size);

/**
 * @brief Function to check whether a new message has arrived from the
 *        remote core, without blocking.
 * @param mailbox_peripheral Which inter-core link to check
 * @return UHAL_STATUS_OK if a message is waiting to be read,
 *         UHAL_STATUS_PERIPHERAL_IN_USE_WARNING if none has arrived yet
 */
uhal_status_t mailbox_message_pending(const mailbox_peripheral_t mailbox_peripheral);

/**
 * @brief Function to read a pending message from the remote core and
 *        acknowledge it (letting the remote core send its next message).
 *        Call mailbox_message_pending() first, or be prepared to block
 *        until one arrives.
 * @param mailbox_peripheral Which inter-core link to read from
 * @param read_buff Pointer to a buffer to copy the message into
 * @param size The amount of bytes to copy -- must not exceed
 *             MAILBOX_MAX_MESSAGE_SIZE (mailbox_platform_specific.h)
 * @return UHAL_STATUS_OK when no errors have occurred
 */
uhal_status_t mailbox_read(const mailbox_peripheral_t mailbox_peripheral, uint8_t* read_buff, const size_t size);

/**
 * @brief Function to de-initialize a mailbox link.
 * @param mailbox_peripheral Which inter-core link to close
 * @return UHAL_STATUS_OK when no errors have occurred
 */
uhal_status_t mailbox_deinit(const mailbox_peripheral_t mailbox_peripheral);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DISABLE_MAILBOX_MODULE */

#endif /* HAL_MAILBOX_H */
