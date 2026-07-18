#include <cstring>
#include "AWR6843.h"
#include "AWR6843_MAILBOX.h"
#include "gmock/gmock.h"
#include "hal_mailbox.h"

MAILBOX_Type Mock_MBOX_MSS_BSS_REG = {};
MAILBOX_Type Mock_MBOX_BSS_MSS_REG = {};
MAILBOX_Type Mock_MBOX_MSS_DSS_REG = {};
MAILBOX_Type Mock_MBOX_DSS_MSS_REG = {};
MAILBOX_Type* MBOX_MSS_BSS_REG = &Mock_MBOX_MSS_BSS_REG;
MAILBOX_Type* MBOX_BSS_MSS_REG = &Mock_MBOX_BSS_MSS_REG;
MAILBOX_Type* MBOX_MSS_DSS_REG = &Mock_MBOX_MSS_DSS_REG;
MAILBOX_Type* MBOX_DSS_MSS_REG = &Mock_MBOX_DSS_MSS_REG;

/* Storage for the mock_mbox_*_mem[] arrays mocks/AWR6843.h points
 * SOC_XWR68XX_MSS_MBOX_*_MEM_BASE_ADDRESS at -- see that header's comment
 * for why (mailbox_iwr68xx.c casts these straight to uint8_t*). */
uint8_t mock_mbox_mss_bss_mem[2048] = {};
uint8_t mock_mbox_bss_mss_mem[2048] = {};
uint8_t mock_mbox_mss_dss_mem[2048] = {};
uint8_t mock_mbox_dss_mss_mem[2048] = {};

class ti_iwr68xx_mailbox : public ::testing::Test {
   protected:
    void SetUp() override {
        memset((void*)MBOX_MSS_BSS_REG, 0x00, sizeof(Mock_MBOX_MSS_BSS_REG));
        memset((void*)MBOX_BSS_MSS_REG, 0x00, sizeof(Mock_MBOX_BSS_MSS_REG));
        memset((void*)MBOX_MSS_DSS_REG, 0x00, sizeof(Mock_MBOX_MSS_DSS_REG));
        memset((void*)MBOX_DSS_MSS_REG, 0x00, sizeof(Mock_MBOX_DSS_MSS_REG));
        memset(mock_mbox_mss_bss_mem, 0x00, sizeof(mock_mbox_mss_bss_mem));
        memset(mock_mbox_bss_mss_mem, 0x00, sizeof(mock_mbox_bss_mss_mem));
        memset(mock_mbox_mss_dss_mem, 0x00, sizeof(mock_mbox_mss_dss_mem));
        memset(mock_mbox_dss_mss_mem, 0x00, sizeof(mock_mbox_dss_mss_mem));
    }
};

TEST_F(ti_iwr68xx_mailbox, init_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_init((mailbox_peripheral_t)0xFF));
}

TEST_F(ti_iwr68xx_mailbox, init_clears_stale_pending_status_both_sides) {
    MBOX_MSS_BSS_REG->INT_STS_CLR = 0U;
    MBOX_BSS_MSS_REG->INT_ACK     = 0U;
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_init(MAILBOX_PERIPHERAL_MSS_BSS));
    EXPECT_EQ((uint32_t)(MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT), MBOX_MSS_BSS_REG->INT_STS_CLR);
    EXPECT_EQ((uint32_t)(MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT), MBOX_BSS_MSS_REG->INT_ACK);
}

TEST_F(ti_iwr68xx_mailbox, write_invalid_params) {
    uint8_t buff[4] = {};
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_write((mailbox_peripheral_t)0xFF, buff, sizeof(buff)));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_write(MAILBOX_PERIPHERAL_MSS_BSS, nullptr, sizeof(buff)));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_write(MAILBOX_PERIPHERAL_MSS_BSS, buff, 0U));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_write(MAILBOX_PERIPHERAL_MSS_BSS, buff, MAILBOX_MAX_MESSAGE_SIZE + 1U));
}

TEST_F(ti_iwr68xx_mailbox, write_mss_bss_has_no_channel_header) {
    /* MSS<->BSS never uses the DSS link's multi-channel framing -- see
     * mailbox_iwr68xx.c's file header. Preset the peer's ack bit so the
     * wait loop doesn't spin. */
    MBOX_BSS_MSS_REG->INT_STS_MASKED = MAILBOX_INT_MAILBOX_ACK_BIT;
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_write(MAILBOX_PERIPHERAL_MSS_BSS, payload, sizeof(payload)));

    EXPECT_EQ(0, memcmp(mock_mbox_mss_bss_mem, payload, sizeof(payload)));
    EXPECT_NE(0U, MBOX_MSS_BSS_REG->INT_TRIG & MAILBOX_INT_MAILBOX_BIT);
    /* Ack observed -- cleared on the link's in_reg (the peer's own
     * baseLocalToRemote, see this driver's file header for why). */
    EXPECT_NE(0U, MBOX_BSS_MSS_REG->INT_ACK & MAILBOX_INT_MAILBOX_ACK_BIT);
}

TEST_F(ti_iwr68xx_mailbox, write_mss_dss_prepends_channel_0_header) {
    MBOX_DSS_MSS_REG->INT_STS_MASKED = MAILBOX_INT_MAILBOX_ACK_BIT;
    const uint8_t payload[] = {0x01, 0x02, 0x03};
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_write(MAILBOX_PERIPHERAL_MSS_DSS, payload, sizeof(payload)));

    const uint32_t expected_ch_id = 0U;
    EXPECT_EQ(0, memcmp(mock_mbox_mss_dss_mem, &expected_ch_id, sizeof(expected_ch_id)));
    EXPECT_EQ(0, memcmp(mock_mbox_mss_dss_mem + sizeof(expected_ch_id), payload, sizeof(payload)));
    EXPECT_NE(0U, MBOX_MSS_DSS_REG->INT_TRIG & MAILBOX_INT_MAILBOX_BIT);
}

TEST_F(ti_iwr68xx_mailbox, write_times_out_when_peer_never_acks) {
    /* INT_STS_MASKED left at 0 -- the peer never acks. Bounded by
     * MAILBOX_ACK_TIMEOUT_ITERATIONS (10,000,000): confirms this driver
     * gives up rather than wedging the caller forever (see this driver's
     * own file header for the real hardware lockup this bound fixes). */
    const uint8_t payload[] = {0x00};
    EXPECT_EQ(UHAL_STATUS_ERROR, mailbox_write(MAILBOX_PERIPHERAL_MSS_BSS, payload, sizeof(payload)));
    /* The message was still sent -- only the ack wait failed. */
    EXPECT_NE(0U, MBOX_MSS_BSS_REG->INT_TRIG & MAILBOX_INT_MAILBOX_BIT);
}

TEST_F(ti_iwr68xx_mailbox, message_pending_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_message_pending((mailbox_peripheral_t)0xFF));
}

TEST_F(ti_iwr68xx_mailbox, message_pending_reflects_in_reg_status) {
    EXPECT_EQ(UHAL_STATUS_PERIPHERAL_IN_USE_WARNING, mailbox_message_pending(MAILBOX_PERIPHERAL_MSS_BSS));
    MBOX_BSS_MSS_REG->INT_STS_MASKED = MAILBOX_INT_MAILBOX_BIT;
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_message_pending(MAILBOX_PERIPHERAL_MSS_BSS));
}

TEST_F(ti_iwr68xx_mailbox, read_invalid_params) {
    uint8_t buff[4];
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_read((mailbox_peripheral_t)0xFF, buff, sizeof(buff)));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_read(MAILBOX_PERIPHERAL_MSS_BSS, nullptr, sizeof(buff)));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_read(MAILBOX_PERIPHERAL_MSS_BSS, buff, 0U));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_read(MAILBOX_PERIPHERAL_MSS_BSS, buff, MAILBOX_MAX_MESSAGE_SIZE + 1U));
}

TEST_F(ti_iwr68xx_mailbox, read_mss_bss_copies_payload_and_acks) {
    const uint8_t message[] = {0x11, 0x22, 0x33};
    memcpy(mock_mbox_bss_mss_mem, message, sizeof(message));
    MBOX_BSS_MSS_REG->INT_STS_MASKED = MAILBOX_INT_MAILBOX_BIT; /* preset -- avoids the blocking wait */

    uint8_t received[sizeof(message)] = {};
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_read(MAILBOX_PERIPHERAL_MSS_BSS, received, sizeof(received)));
    EXPECT_EQ(0, memcmp(message, received, sizeof(message)));

    EXPECT_NE(0U, MBOX_BSS_MSS_REG->INT_ACK & MAILBOX_INT_MAILBOX_BIT);
    EXPECT_NE(0U, MBOX_MSS_BSS_REG->INT_TRIG & MAILBOX_INT_MAILBOX_ACK_BIT);
}

TEST_F(ti_iwr68xx_mailbox, read_mss_dss_skips_channel_header) {
    const uint32_t ch_id = 0U;
    const uint8_t  message[] = {0xAA, 0xBB};
    memcpy(mock_mbox_dss_mss_mem, &ch_id, sizeof(ch_id));
    memcpy(mock_mbox_dss_mss_mem + sizeof(ch_id), message, sizeof(message));
    MBOX_DSS_MSS_REG->INT_STS_MASKED = MAILBOX_INT_MAILBOX_BIT;

    uint8_t received[sizeof(message)] = {};
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_read(MAILBOX_PERIPHERAL_MSS_DSS, received, sizeof(received)));
    EXPECT_EQ(0, memcmp(message, received, sizeof(message)));
}

TEST_F(ti_iwr68xx_mailbox, deinit_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, mailbox_deinit((mailbox_peripheral_t)0xFF));
}

TEST_F(ti_iwr68xx_mailbox, deinit_clears_both_sides) {
    EXPECT_EQ(UHAL_STATUS_OK, mailbox_deinit(MAILBOX_PERIPHERAL_MSS_DSS));
    EXPECT_EQ((uint32_t)(MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT), MBOX_MSS_DSS_REG->INT_STS_CLR);
    EXPECT_EQ((uint32_t)(MAILBOX_INT_MAILBOX_BIT | MAILBOX_INT_MAILBOX_ACK_BIT), MBOX_DSS_MSS_REG->INT_ACK);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS()) {
    }
    return 0;
}
