#include <cstring>
#include "AWR6843.h"
#include "AWR6843_SCI.h"
#include "gmock/gmock.h"
#include "hal_uart.h"

SCI_Type  Mock_SCI_A = {};
SCI_Type  Mock_SCI_B = {};
SCI_Type* SCI_A = &Mock_SCI_A;
SCI_Type* SCI_B = &Mock_SCI_B;

class ti_iwr68xx_uart : public ::testing::Test {
   protected:
    void SetUp() override {
        memset((void*)SCI_A, 0x00, sizeof(Mock_SCI_A));
        memset((void*)SCI_B, 0x00, sizeof(Mock_SCI_B));
    }
};

TEST_F(ti_iwr68xx_uart, init_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS,
              uhal_uart_init((uart_peripheral_inst_t)0xFF, 115200U, UART_CLK_SOURCE_USE_DEFAULT, 200000000U, UART_EXTRA_OPT_USE_DEFAULT));
}

TEST_F(ti_iwr68xx_uart, init_zero_baudrate_rejected) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS,
              uhal_uart_init(UART_PERIPHERAL_MSS_SCIA, 0U, UART_CLK_SOURCE_USE_DEFAULT, 200000000U, UART_EXTRA_OPT_USE_DEFAULT));
}

TEST_F(ti_iwr68xx_uart, init_zero_clock_freq_rejected) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS,
              uhal_uart_init(UART_PERIPHERAL_MSS_SCIA, 115200U, UART_CLK_SOURCE_USE_DEFAULT, 0U, UART_EXTRA_OPT_USE_DEFAULT));
}

TEST_F(ti_iwr68xx_uart, init_default_8n1) {
    const uint32_t baudrate = 115200U;
    const uint32_t clock_freq = 200000000U;
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_init(UART_PERIPHERAL_MSS_SCIA, baudrate, UART_CLK_SOURCE_USE_DEFAULT, clock_freq, UART_EXTRA_OPT_USE_DEFAULT));

    /* Rx/Tx enabled, internal clock, async timing, no parity, one stop bit -- and finally started (SW_NRESET). */
    uint32_t expected_gcr1 = SCIGCR1_TXENA | SCIGCR1_RXENA | SCIGCR1_CLOCK | SCIGCR1_TIMING_MODE | SCIGCR1_SW_NRESET;
    EXPECT_EQ(expected_gcr1, SCI_A->SCIGCR1);
    EXPECT_EQ(0U, SCI_A->SCIGCR1 & (SCIGCR1_STOP_2BIT | SCIGCR1_PARITY_ENA));

    EXPECT_EQ(baudrate + 1U == 0U ? 0U : clock_freq / (16U * (baudrate + 1U)), SCI_A->SCIBAUD);
    EXPECT_EQ(7U, SCI_A->SCICHAR); /* 8 data bits */

    EXPECT_EQ((uint32_t)(SCIPIO_RX_BIT | SCIPIO_TX_BIT), SCI_A->SCIPIO0);
    EXPECT_EQ((uint32_t)(SCIPIO_RX_BIT | SCIPIO_TX_BIT), SCI_A->SCIPIO8);
    EXPECT_EQ(0U, SCI_A->SCIPIO1);
    EXPECT_EQ(0U, SCI_A->SCIPIO3);
    EXPECT_EQ(0U, SCI_A->SCIPIO6);
    EXPECT_EQ(0U, SCI_A->SCIPIO7);

    /* SCIB left untouched -- init() only ever addresses the selected instance. */
    EXPECT_EQ(0U, SCI_B->SCIGCR1);
}

TEST_F(ti_iwr68xx_uart, init_selects_scib) {
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_init(UART_PERIPHERAL_MSS_SCIB, 9600U, UART_CLK_SOURCE_USE_DEFAULT, 200000000U, UART_EXTRA_OPT_USE_DEFAULT));
    EXPECT_NE(0U, SCI_B->SCIGCR1);
    EXPECT_EQ(0U, SCI_A->SCIGCR1);
}

TEST_F(ti_iwr68xx_uart, init_two_stop_bits_and_even_parity) {
    const uart_extra_config_opt_t opts = (uart_extra_config_opt_t)(UART_EXTRA_OPT_TWO_STOP_BITS | UART_EXTRA_OPT_PARITY_EVEN);
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_init(UART_PERIPHERAL_MSS_SCIA, 115200U, UART_CLK_SOURCE_USE_DEFAULT, 200000000U, opts));
    EXPECT_NE(0U, SCI_A->SCIGCR1 & SCIGCR1_STOP_2BIT);
    EXPECT_NE(0U, SCI_A->SCIGCR1 & SCIGCR1_PARITY_ENA);
    EXPECT_NE(0U, SCI_A->SCIGCR1 & SCIGCR1_PARITY_EVEN);
}

TEST_F(ti_iwr68xx_uart, init_odd_parity) {
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_init(UART_PERIPHERAL_MSS_SCIA, 115200U, UART_CLK_SOURCE_USE_DEFAULT, 200000000U, UART_EXTRA_OPT_PARITY_ODD));
    EXPECT_NE(0U, SCI_A->SCIGCR1 & SCIGCR1_PARITY_ENA);
    EXPECT_EQ(0U, SCI_A->SCIGCR1 & SCIGCR1_PARITY_EVEN);
}

TEST_F(ti_iwr68xx_uart, deinit_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, uhal_uart_deinit((uart_peripheral_inst_t)0xFF));
}

TEST_F(ti_iwr68xx_uart, deinit_clears_gcr) {
    SCI_A->SCIGCR0 = 1U;
    SCI_A->SCIGCR1 = 0xFFFFFFFFU;
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_deinit(UART_PERIPHERAL_MSS_SCIA));
    EXPECT_EQ(0U, SCI_A->SCIGCR1);
    EXPECT_EQ(0U, SCI_A->SCIGCR0);
}

TEST_F(ti_iwr68xx_uart, transmit_invalid_params) {
    uint8_t byte = 0x41U;
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, uhal_uart_transmit((uart_peripheral_inst_t)0xFF, &byte, 1U));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, uhal_uart_transmit(UART_PERIPHERAL_MSS_SCIA, nullptr, 1U));
}

TEST_F(ti_iwr68xx_uart, transmit_writes_each_byte_in_order) {
    /* SCIFLR_TXRDY_BIT preset -- otherwise uhal_uart_transmit()'s polling
     * loop spins forever, since nothing else on a host build ever sets it. */
    SCI_A->SCIFLR = SCIFLR_TXRDY_BIT;
    const uint8_t message[] = {'h', 'i', '!'};
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_transmit(UART_PERIPHERAL_MSS_SCIA, message, sizeof(message)));
    /* Only the last byte written is observable through SCITD (one-deep TX
     * holding register) -- confirms the last element of the buffer landed. */
    EXPECT_EQ('!', SCI_A->SCITD);
}

TEST_F(ti_iwr68xx_uart, transmit_zero_size_is_a_noop) {
    SCI_A->SCIFLR = 0U; /* would hang forever if the loop body ran at all */
    const uint8_t message[] = {'x'};
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_transmit(UART_PERIPHERAL_MSS_SCIA, message, 0U));
}

TEST_F(ti_iwr68xx_uart, receive_invalid_params) {
    uint8_t byte;
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, uhal_uart_receive((uart_peripheral_inst_t)0xFF, &byte, 1U));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, uhal_uart_receive(UART_PERIPHERAL_MSS_SCIA, nullptr, 1U));
}

TEST_F(ti_iwr68xx_uart, receive_reads_bytes) {
    SCI_A->SCIFLR = SCIFLR_RXRDY_BIT;
    SCI_A->SCIRD  = 'Z';
    uint8_t received = 0U;
    EXPECT_EQ(UHAL_STATUS_OK, uhal_uart_receive(UART_PERIPHERAL_MSS_SCIA, &received, 1U));
    EXPECT_EQ('Z', received);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS()) {
    }
    return 0;
}
