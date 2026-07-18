include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.x
)

# CMake 3.14+
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    fff
    GIT_REPOSITORY https://github.com/meekrosoft/fff.git
    GIT_TAG master
)

FetchContent_MakeAvailable(fff)

add_library(board_sdk_ti_iwr68xx INTERFACE "mocks/AWR6843_GIO.h"
                                     "mocks/AWR6843_IOMUX.h"
                                     "mocks/AWR6843_types.h"
                                     "mocks/AWR6843.h")

target_include_directories(board_sdk_ti_iwr68xx INTERFACE mocks/)


add_library(universal_hal_gpio_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/gpio/gpio_iwr68xx.c
                                          ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/pinmux/pinmux_iwr68xx.c)

target_compile_definitions(universal_hal_gpio_ti_iwr68xx PUBLIC "DISABLE_I2C_HOST_MODULE"
                                         "DISABLE_I2C_SLAVE_MODULE"
                                         "DISABLE_SPI_HOST_MODULE"
                                         "DISABLE_SPI_SLAVE_MODULE"
                                         "DISABLE_DMA_MODULE"
                                         "DISABLE_DMA_HANDLER"
                                         "DISABLE_UART_MODULE")

target_include_directories(universal_hal_gpio_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                ${UHAL_FILEPATH}/hal/)

target_link_libraries(universal_hal_gpio_ti_iwr68xx board_sdk_ti_iwr68xx)



add_executable(TI_IWR68XX_GPIO_TEST gpio/gpio.cpp)
target_link_libraries(TI_IWR68XX_GPIO_TEST universal_hal_gpio_ti_iwr68xx board_sdk_ti_iwr68xx gtest_main gmock_main)
# target_compile_definitions(TI_IWR68XX_GPIO_TEST PUBLIC "FFF_GENERATE")

add_executable(TI_IWR68XX_PINMUX_TEST pinmux/pinmux.cpp)
target_link_libraries(TI_IWR68XX_PINMUX_TEST universal_hal_gpio_ti_iwr68xx board_sdk_ti_iwr68xx gtest_main gmock_main)

# -- UART --------------------------------------------------------------
add_library(universal_hal_uart_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/uart/uart_iwr68xx.c)
target_include_directories(universal_hal_uart_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                 ${UHAL_FILEPATH}/hal/)
target_link_libraries(universal_hal_uart_ti_iwr68xx board_sdk_ti_iwr68xx)

add_executable(TI_IWR68XX_UART_TEST uart/uart.cpp)
target_link_libraries(TI_IWR68XX_UART_TEST universal_hal_uart_ti_iwr68xx board_sdk_ti_iwr68xx gtest_main gmock_main)

# -- DMA -----------------------------------------------------------------
add_library(universal_hal_dma_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/dma/dma_iwr68xx.c)
target_include_directories(universal_hal_dma_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                ${UHAL_FILEPATH}/hal/)
target_link_libraries(universal_hal_dma_ti_iwr68xx board_sdk_ti_iwr68xx)

add_executable(TI_IWR68XX_DMA_TEST dma/dma.cpp)
target_link_libraries(TI_IWR68XX_DMA_TEST universal_hal_dma_ti_iwr68xx board_sdk_ti_iwr68xx gtest_main gmock_main)

# -- Mailbox ---------------------------------------------------------------
add_library(universal_hal_mailbox_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/mailbox/mailbox_iwr68xx.c)
target_include_directories(universal_hal_mailbox_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                    ${UHAL_FILEPATH}/hal/)
target_link_libraries(universal_hal_mailbox_ti_iwr68xx board_sdk_ti_iwr68xx)

add_executable(TI_IWR68XX_MAILBOX_TEST mailbox/mailbox.cpp)
target_link_libraries(TI_IWR68XX_MAILBOX_TEST universal_hal_mailbox_ti_iwr68xx board_sdk_ti_iwr68xx gtest_main gmock_main)

# -- SPI host ----------------------------------------------------------------
# spi_host_iwr68xx.c drives chip-select via gpio_set_pin_lvl() -- needs
# gpio_iwr68xx.c linked in alongside it (see spi_host_iwr68xx.c's file header).
add_library(universal_hal_spi_host_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/spi_host/spi_host_iwr68xx.c
                                              ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/gpio/gpio_iwr68xx.c)
target_include_directories(universal_hal_spi_host_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                     ${UHAL_FILEPATH}/hal/)
target_link_libraries(universal_hal_spi_host_ti_iwr68xx board_sdk_ti_iwr68xx)

add_executable(TI_IWR68XX_SPI_HOST_TEST spi_host/spi_host.cpp)
target_link_libraries(TI_IWR68XX_SPI_HOST_TEST universal_hal_spi_host_ti_iwr68xx board_sdk_ti_iwr68xx gtest_main gmock_main)

# -- SOC -----------------------------------------------------------------
# soc_iwr68xx.c's CP15 (MPU) register access is hand-written assembly
# (soc_mpu_iwr68xx.asm/_gnu.S) -- not host-buildable and not linked here;
# soc.cpp provides fff fakes for the uhal_mpu_*() functions it declares
# extern, so mpu_config()'s region table/sequencing is testable without them.
add_library(universal_hal_soc_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/soc/soc_iwr68xx.c)
target_include_directories(universal_hal_soc_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                ${UHAL_FILEPATH}/hal/)
target_link_libraries(universal_hal_soc_ti_iwr68xx board_sdk_ti_iwr68xx)

add_executable(TI_IWR68XX_SOC_TEST soc/soc.cpp)
target_link_libraries(TI_IWR68XX_SOC_TEST universal_hal_soc_ti_iwr68xx board_sdk_ti_iwr68xx fff gtest_main gmock_main)

# -- ESM -----------------------------------------------------------------
# esm_iwr68xx.c is inherently RTOS-coupled (SYS/BIOS's Hwi_create(), see its
# own file header) -- mocks/ti/sysbios/family/arm/v7r/vim/Hwi.h and
# mocks/xdc/runtime/Error.h (pulled in via board_sdk_ti_iwr68xx's mocks/
# include dir) stand in for the real xdctools-generated SYS/BIOS package;
# esm.cpp fakes their API with fff.
add_library(universal_hal_esm_ti_iwr68xx ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/esm/esm_iwr68xx.c)
target_include_directories(universal_hal_esm_ti_iwr68xx PUBLIC ${UHAL_FILEPATH}/hal/platform/ti/iwr68xx/
                                                                ${UHAL_FILEPATH}/hal/)
target_link_libraries(universal_hal_esm_ti_iwr68xx board_sdk_ti_iwr68xx)

add_executable(TI_IWR68XX_ESM_TEST esm/esm.cpp)
target_link_libraries(TI_IWR68XX_ESM_TEST universal_hal_esm_ti_iwr68xx board_sdk_ti_iwr68xx fff gtest_main gmock_main)

add_test(
        NAME TI_IWR68XX_GPIO_TEST
        COMMAND TI_IWR68XX_GPIO_TEST
)

add_test(
        NAME TI_IWR68XX_PINMUX_TEST
        COMMAND TI_IWR68XX_PINMUX_TEST
)

add_test(
        NAME TI_IWR68XX_UART_TEST
        COMMAND TI_IWR68XX_UART_TEST
)

add_test(
        NAME TI_IWR68XX_DMA_TEST
        COMMAND TI_IWR68XX_DMA_TEST
)

add_test(
        NAME TI_IWR68XX_MAILBOX_TEST
        COMMAND TI_IWR68XX_MAILBOX_TEST
)

add_test(
        NAME TI_IWR68XX_SPI_HOST_TEST
        COMMAND TI_IWR68XX_SPI_HOST_TEST
)

add_test(
        NAME TI_IWR68XX_SOC_TEST
        COMMAND TI_IWR68XX_SOC_TEST
)

add_test(
        NAME TI_IWR68XX_ESM_TEST
        COMMAND TI_IWR68XX_ESM_TEST
)
