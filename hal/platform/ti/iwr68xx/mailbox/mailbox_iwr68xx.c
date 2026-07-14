/**
* \file            mailbox_iwr68xx.c
* \brief           Source file which implements the standard Mailbox API functions
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
* Protocol ported from the mmWave SDK's Mailbox_write()/Mailbox_read()/
* Mailbox_readFlush() and its box-full/box-empty ISR processing functions
* (ti/drivers/mailbox/src/mailbox.c) -- this driver replicates what those
* ISRs do by polling the same status bits directly instead of registering
* interrupt handlers (this peripheral's IRQ registration needs SYS/BIOS's
* Hwi_create(), the same reason esm_iwr68xx.c is optional).
*
* The MSS<->DSS link (only) uses the SDK's "multi-channel" framing: a
* 4-byte channel-ID header prepended to every message. This isn't optional
* -- it's required for wire-compatibility with the actual DSS binary this
* project flashes (example/Barebones_MSS/prebuilt/placeholder_dss.bin is
* byte-identical to a vital-signs-tracking demo's built DSS image, whose
* MSS and DSS sides both call Mailbox_open() with chType=MAILBOX_CHTYPE_MULTI,
* chId=MAILBOX_CH_ID_0 -- see mss_main.c/dss_main.c in that demo's firmware
* tree). Skipping the header would desync every message by 4 bytes against
* that DSS image. MSS<->BSS never uses this framing (matches the SDK: the
* multi-channel path is explicitly skipped whenever remoteEndpoint==BSS).
*
* Each of the four hardware register blocks (MSS_BSS, BSS_MSS, MSS_DSS,
* DSS_MSS -- see AWR6843.h) multiplexes two single-bit doorbell lines, BOTH
* driven by the SAME core -- the block's "owner": bit 0 ("mailbox") is that
* core's new-message-for-my-peer flag; bit 1 ("mailbox ack") is that SAME
* core's I-just-read-and-consumed-my-peer's-last-message flag. E.g. on the
* MSS_DSS block, MSS alone drives both bits -- bit 0 when MSS has a new
* message for DSS, bit 1 once MSS has finished reading DSS's last message
* (which arrived over the *other* block, DSS_MSS). This was gotten wrong
* in an earlier revision of this file (assumed bit 1 on a block reflects
* the *reader* acking, i.e. DSS acking on MSS_DSS) -- confirmed against
* the mmWave SDK's own per-core Mailbox_HwCfg values
* (ti/drivers/mailbox/platform/mailbox_xwr68xx.c): each core's
* Mailbox_readFlush() always writes bit 1 on its OWN baseLocalToRemote (=
* the block it uses for its own outgoing messages), never on the block the
* incoming message arrived on. So: to know "did my peer finish reading the
* message I just sent", poll bit 1 on the block YOU use to *receive from*
* them (your in_reg) -- that's the identical physical register as their
* own baseLocalToRemote/out_reg. Confirmed on real hardware: after DSS
* read+processed an MSS-sent message (and replied over the mailbox),
* DSS_MSS's INT_STS_MASKED read back 0x2 (bit 1 set) while MSS_DSS's
* stayed 0 -- exactly this model, and the opposite of what mailbox_write()
* used to poll.
*/
#include <hal_mailbox.h>
#include <AWR6843.h>
#include <string.h>

/* Matches the mmWave SDK's mailbox_internal.h/mailbox.h -- see this file's
 * header for why the DSS link needs this. */
#define MAILBOX_MULTI_CH_HEADER_SIZE 4U
#define MAILBOX_CH_ID_0               0U

typedef struct {
    volatile MAILBOX_Type* out_reg; /* register block this core triggers to send */
    volatile MAILBOX_Type* in_reg;  /* register block the remote core triggers */
    uint8_t* out_mem;                /* shared memory this core writes messages into */
    uint8_t* in_mem;                  /* shared memory the remote core writes messages into */
    uint8_t header_size;               /* MAILBOX_MULTI_CH_HEADER_SIZE for the DSS link, 0 for BSS */
} mailbox_link_t;

static uhal_status_t get_link(const mailbox_peripheral_t mailbox_peripheral, mailbox_link_t* const link) {
    switch (mailbox_peripheral) {
        case MAILBOX_PERIPHERAL_MSS_BSS:
            link->out_reg = MBOX_MSS_BSS_REG;
            link->in_reg = MBOX_BSS_MSS_REG;
            link->out_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_MSS_BSS_MEM_BASE_ADDRESS;
            link->in_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_BSS_MSS_MEM_BASE_ADDRESS;
            link->header_size = 0U;
            return UHAL_STATUS_OK;
        case MAILBOX_PERIPHERAL_MSS_DSS:
            link->out_reg = MBOX_MSS_DSS_REG;
            link->in_reg = MBOX_DSS_MSS_REG;
            link->out_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_MSS_DSS_MEM_BASE_ADDRESS;
            link->in_mem = (uint8_t*)SOC_XWR68XX_MSS_MBOX_DSS_MSS_MEM_BASE_ADDRESS;
            link->header_size = MAILBOX_MULTI_CH_HEADER_SIZE;
            return UHAL_STATUS_OK;
        default:
            return UHAL_STATUS_INVALID_PARAMETERS;
    }
}

uhal_status_t mailbox_init(const mailbox_peripheral_t mailbox_peripheral) {
    mailbox_link_t link;
    if (get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    /* Clear any stale pending status left over from before this core booted. */
    link.out_reg->INT_STS_CLR = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    return UHAL_STATUS_OK;
}

/* Bound on the ack-wait loop below -- the mmWave SDK's own Mailbox_write()
 * (MAILBOX_MODE_BLOCKING) waits forever, but this HAL has no OS/scheduler
 * dependency to fall back on if the remote core never acks (e.g. it's
 * still stuck in its own boot/init, or was never flashed/running at all)
 * -- an unbounded wait here would permanently wedge any caller (confirmed:
 * GCC_FreeRTOS_VitalSigns_MSS's CLI task locked up solid, unrecoverable
 * without a board reset, sending a config message to a DSS that never
 * acked). Large enough that a live peer's prompt ack is never mistaken for
 * a timeout; this is a raw instruction-count spin, not calibrated to a
 * real time unit. */
#define MAILBOX_ACK_TIMEOUT_ITERATIONS 10000000U

uhal_status_t mailbox_write(const mailbox_peripheral_t mailbox_peripheral, const uint8_t* write_buff, const size_t size) {
    mailbox_link_t link;
    uint32_t timeout;
    if ((get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) || (write_buff == (const uint8_t*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((size == 0U) || (size > MAILBOX_MAX_MESSAGE_SIZE)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    if (link.header_size != 0U) {
        const uint32_t ch_id = MAILBOX_CH_ID_0;
        (void)memcpy((void*)link.out_mem, (const void*)&ch_id, sizeof(ch_id));
    }
    (void)memcpy((void*)(link.out_mem + link.header_size), (const void*)write_buff, size);

    /* Trigger the "mailbox" line -- tells the remote core a message is ready. */
    link.out_reg->INT_TRIG = MAILBOX_INT_MAILBOX_BIT;

    /* Wait for the remote core's ack that it read this message -- see this
     * file's header for why that's link.in_reg (the remote peer's own
     * baseLocalToRemote, the identical physical register from their side),
     * not link.out_reg -- up to MAILBOX_ACK_TIMEOUT_ITERATIONS before
     * giving up. */
    for (timeout = MAILBOX_ACK_TIMEOUT_ITERATIONS; timeout != 0U; timeout--) {
        if ((link.in_reg->INT_STS_MASKED & MAILBOX_INT_MAILBOX_ACK_BIT) != 0U) {
            break;
        }
    }
    if (timeout == 0U) {
        return UHAL_STATUS_ERROR;
    }
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_ACK_BIT;

    return UHAL_STATUS_OK;
}

uhal_status_t mailbox_message_pending(const mailbox_peripheral_t mailbox_peripheral) {
    mailbox_link_t link;
    if (get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((link.in_reg->INT_STS_MASKED & MAILBOX_INT_MAILBOX_BIT) != 0U) {
        return UHAL_STATUS_OK;
    }
    return UHAL_STATUS_PERIPHERAL_IN_USE_WARNING;
}

uhal_status_t mailbox_read(const mailbox_peripheral_t mailbox_peripheral, uint8_t* read_buff, const size_t size) {
    mailbox_link_t link;
    if ((get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) || (read_buff == (uint8_t*)0)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    if ((size == 0U) || (size > MAILBOX_MAX_MESSAGE_SIZE)) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }

    /* Block until a message arrives -- call mailbox_message_pending() first
     * to avoid blocking, see hal_mailbox.h. */
    while ((link.in_reg->INT_STS_MASKED & MAILBOX_INT_MAILBOX_BIT) == 0U) {
    }

    (void)memcpy((void*)read_buff, (const void*)(link.in_mem + link.header_size), size);

    /* Clear our own pending status for this message... */
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_BIT;
    /* ...then tell the remote core it can send another one. */
    link.out_reg->INT_TRIG = MAILBOX_INT_MAILBOX_ACK_BIT;

    return UHAL_STATUS_OK;
}

uhal_status_t mailbox_deinit(const mailbox_peripheral_t mailbox_peripheral) {
    mailbox_link_t link;
    if (get_link(mailbox_peripheral, &link) != UHAL_STATUS_OK) {
        return UHAL_STATUS_INVALID_PARAMETERS;
    }
    link.out_reg->INT_STS_CLR = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    link.in_reg->INT_ACK = MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT;
    return UHAL_STATUS_OK;
}
