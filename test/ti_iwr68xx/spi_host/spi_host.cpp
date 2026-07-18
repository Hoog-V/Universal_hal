#include <bit_manipulation.h>
#include <cstring>
#include "AWR6843.h"
#include "AWR6843_MIBSPI.h"
#include "gmock/gmock.h"
#include "hal_spi_host.h"

/* gpio_iwr68xx.c's own GPIO_PORT()/GPIO_PIN() macros are file-local, not
 * exposed via a header -- reproduce the same lower/upper-nibble split from
 * bit_manipulation.h directly (same helpers test/ti_iwr68xx/gpio/gpio.cpp
 * already uses). */
#define CS_PORT(pin) GET_LOWER_4_BITS_OF_BYTE(pin)
#define CS_PIN(pin)  GET_UPPER_4_BITS_OF_BYTE(pin)

GIO_Type      Mock_GIO = {};
IOMUX_Type    Mock_IOMUX = {};
MIBSPI_Type   Mock_MIBSPI_A = {};
MIBSPI_Type   Mock_MIBSPI_B = {};
GIO_Type*     GIO = &Mock_GIO;
IOMUX_Type*   IOMUX = &Mock_IOMUX;
MIBSPI_Type*  MIBSPI_A = &Mock_MIBSPI_A;
MIBSPI_Type*  MIBSPI_B = &Mock_MIBSPI_B;

/* Chip select is a plain GPIO pin (see spi_host_iwr68xx.c's file header) --
 * port 0 is as good as any for a test. */
static constexpr gpio_pin_t kChipSelectPin = GPIO_PIN_5;

class ti_iwr68xx_spi_host : public ::testing::Test {
   protected:
    void SetUp() override {
        memset((void*)GIO, 0x00, sizeof(Mock_GIO));
        memset((void*)IOMUX, 0x00, sizeof(Mock_IOMUX));
        memset((void*)MIBSPI_A, 0x00, sizeof(Mock_MIBSPI_A));
        memset((void*)MIBSPI_B, 0x00, sizeof(Mock_MIBSPI_B));
    }
};

TEST_F(ti_iwr68xx_spi_host, init_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_init(SPI_PERIPHERAL_MIBSPIB, SPI_CLK_SOURCE_USE_DEFAULT, 200000000U, 1000000UL, SPI_BUS_OPT_MODE_0));
}

TEST_F(ti_iwr68xx_spi_host, init_zero_frequencies_rejected) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_init(SPI_PERIPHERAL_MIBSPIA, SPI_CLK_SOURCE_USE_DEFAULT, 0U, 1000000UL, SPI_BUS_OPT_MODE_0));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_init(SPI_PERIPHERAL_MIBSPIA, SPI_CLK_SOURCE_USE_DEFAULT, 200000000U, 0UL, SPI_BUS_OPT_MODE_0));
}

TEST_F(ti_iwr68xx_spi_host, init_lsb_first_not_implemented) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS,
              spi_host_init(SPI_PERIPHERAL_MIBSPIA, SPI_CLK_SOURCE_USE_DEFAULT, 200000000U, 1000000UL, SPI_BUS_OPT_LSB_FIRST));
}

TEST_F(ti_iwr68xx_spi_host, init_master_mode_and_pin_functions) {
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_init(SPI_PERIPHERAL_MIBSPIA, SPI_CLK_SOURCE_USE_DEFAULT, 200000000U, 1000000UL, SPI_BUS_OPT_MODE_0));

    EXPECT_NE(0U, MIBSPI_A->SPIGCR1 & SPIGCR1_MASTER);
    EXPECT_NE(0U, MIBSPI_A->SPIGCR1 & SPIGCR1_CLKMOD);
    EXPECT_NE(0U, MIBSPI_A->SPIGCR1 & SPIGCR1_SPIEN); /* enabled only after the rest is configured */
    EXPECT_EQ((uint32_t)(SPIPC0_CLKFUN | SPIPC0_SIMOFUN0 | SPIPC0_SOMIFUN0), MIBSPI_A->SPIPC0);
    EXPECT_EQ((uint32_t)(7U << SPIFMT0_CHARLEN_SHIFT), MIBSPI_A->SPIFMT0 & ~(0xFFU << SPIFMT0_PRESCALE_SHIFT));
}

TEST_F(ti_iwr68xx_spi_host, init_computes_prescale_from_clock_ratio) {
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_init(SPI_PERIPHERAL_MIBSPIA, SPI_CLK_SOURCE_USE_DEFAULT, 200000000U, 1000000UL, SPI_BUS_OPT_MODE_0));
    const uint32_t expected_prescale = (200000000U / 1000000UL) - 1U;
    EXPECT_EQ(expected_prescale, (MIBSPI_A->SPIFMT0 >> SPIFMT0_PRESCALE_SHIFT) & 0xFFU);
}

TEST_F(ti_iwr68xx_spi_host, init_mode_2_sets_polarity_and_phase_bits) {
    /* MODE_2: CPOL=1, CPHA=0 -- bit 1 (POLARITY) set, bit 0 (PHASE) clear. */
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_init(SPI_PERIPHERAL_MIBSPIA, SPI_CLK_SOURCE_USE_DEFAULT, 200000000U, 1000000UL, SPI_BUS_OPT_MODE_2));
    EXPECT_NE(0U, MIBSPI_A->SPIFMT0 & SPIFMT0_POLARITY);
    EXPECT_EQ(0U, MIBSPI_A->SPIFMT0 & SPIFMT0_PHASE);
}

TEST_F(ti_iwr68xx_spi_host, deinit_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_deinit(SPI_PERIPHERAL_MIBSPIB));
}

TEST_F(ti_iwr68xx_spi_host, deinit_disables_and_resets) {
    MIBSPI_A->SPIGCR1 = SPIGCR1_SPIEN | SPIGCR1_MASTER;
    MIBSPI_A->SPIGCR0 = 1U;
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_deinit(SPI_PERIPHERAL_MIBSPIA));
    EXPECT_EQ(0U, MIBSPI_A->SPIGCR1 & SPIGCR1_SPIEN);
    EXPECT_EQ(0U, MIBSPI_A->SPIGCR0);
}

TEST_F(ti_iwr68xx_spi_host, start_transaction_invalid_peripheral) {
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_start_transaction(SPI_PERIPHERAL_MIBSPIB, kChipSelectPin, SPI_EXTRA_OPT_USE_DEFAULT));
}

TEST_F(ti_iwr68xx_spi_host, start_transaction_drives_cs_low) {
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_start_transaction(SPI_PERIPHERAL_MIBSPIA, kChipSelectPin, SPI_EXTRA_OPT_USE_DEFAULT));
    EXPECT_NE(0U, GIO->GIOPORT[CS_PORT(kChipSelectPin)].GIODCLR.bit.DCLR & (1U << CS_PIN(kChipSelectPin)));
}

TEST_F(ti_iwr68xx_spi_host, end_transaction_drives_cs_high) {
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_end_transaction(SPI_PERIPHERAL_MIBSPIA, kChipSelectPin));
    EXPECT_NE(0U, GIO->GIOPORT[CS_PORT(kChipSelectPin)].GIODSET.bit.DSET & (1U << CS_PIN(kChipSelectPin)));
}

TEST_F(ti_iwr68xx_spi_host, write_blocking_invalid_params) {
    uint8_t buff[1] = {0};
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_write_blocking(SPI_PERIPHERAL_MIBSPIB, buff, sizeof(buff)));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_write_blocking(SPI_PERIPHERAL_MIBSPIA, nullptr, sizeof(buff)));
}

TEST_F(ti_iwr68xx_spi_host, write_blocking_shifts_each_byte) {
    /* SPIBUF_RXEMPTY clear -- otherwise spi_transfer_byte()'s poll loop
     * spins forever waiting for a byte that never arrives on a host build. */
    MIBSPI_A->SPIBUF = 0U;
    const uint8_t tx[] = {0x12, 0x34, 0x56};
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_write_blocking(SPI_PERIPHERAL_MIBSPIA, tx, sizeof(tx)));
    /* Only the last write is observable through the one-deep SPIDAT1
     * register -- always "no hardware CS" (CSNR_NONE), see
     * spi_host_iwr68xx.c's file header for why. */
    const uint32_t expected_dat1 = (0x56U << SPIDAT1_TXDATA_SHIFT) | (SPIDAT1_CSNR_NONE << SPIDAT1_CSNR_SHIFT);
    EXPECT_EQ(expected_dat1, MIBSPI_A->SPIDAT1);
}

TEST_F(ti_iwr68xx_spi_host, write_non_blocking_falls_back_to_blocking) {
    MIBSPI_A->SPIBUF = 0U;
    const uint8_t tx[] = {0xAB};
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_write_non_blocking(SPI_PERIPHERAL_MIBSPIA, tx, sizeof(tx)));
    EXPECT_EQ((uint32_t)((0xABU << SPIDAT1_TXDATA_SHIFT) | (SPIDAT1_CSNR_NONE << SPIDAT1_CSNR_SHIFT)), MIBSPI_A->SPIDAT1);
}

TEST_F(ti_iwr68xx_spi_host, read_blocking_invalid_params) {
    uint8_t buff[1];
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_read_blocking(SPI_PERIPHERAL_MIBSPIB, buff, sizeof(buff)));
    EXPECT_EQ(UHAL_STATUS_INVALID_PARAMETERS, spi_host_read_blocking(SPI_PERIPHERAL_MIBSPIA, nullptr, sizeof(buff)));
}

TEST_F(ti_iwr68xx_spi_host, read_blocking_shifts_dummy_byte_and_captures_rx) {
    MIBSPI_A->SPIBUF = 0x00CDU; /* RXEMPTY clear, RXDATA = 0xCD */
    uint8_t rx[2] = {};
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_read_blocking(SPI_PERIPHERAL_MIBSPIA, rx, sizeof(rx)));
    EXPECT_EQ(0xCD, rx[0]);
    EXPECT_EQ(0xCD, rx[1]);
    /* 0xFF dummy TX drives the clock while reading -- see spi_transfer_byte(). */
    EXPECT_EQ((uint32_t)((0xFFU << SPIDAT1_TXDATA_SHIFT) | (SPIDAT1_CSNR_NONE << SPIDAT1_CSNR_SHIFT)), MIBSPI_A->SPIDAT1);
}

TEST_F(ti_iwr68xx_spi_host, read_non_blocking_falls_back_to_blocking) {
    MIBSPI_A->SPIBUF = 0x0042U;
    uint8_t rx[1] = {};
    EXPECT_EQ(UHAL_STATUS_OK, spi_host_read_non_blocking(SPI_PERIPHERAL_MIBSPIA, rx, sizeof(rx)));
    EXPECT_EQ(0x42, rx[0]);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS()) {
    }
    return 0;
}
