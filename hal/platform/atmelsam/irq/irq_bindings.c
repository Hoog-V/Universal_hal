#include <stddef.h>
#include <sam.h>
#include <irq/sercom_stuff.h>
#include "gpio/gpio_irq_handler.h"
#include "i2c_host/i2c_host_irq_handler.h"
#include "i2c_slave/i2c_slave_irq_handler.h"
#include "spi_host/spi_host_irq_handler.h"
#include "spi_slave/spi_slave_irq_handler.h"
#include "bit_manipulation.h"

void enable_irq_handler(IRQn_Type irq_type, uint8_t priority) {
        NVIC_DisableIRQ(irq_type);
        NVIC_ClearPendingIRQ(irq_type);

        NVIC_SetPriority(irq_type, priority);
        NVIC_EnableIRQ(irq_type);
}



/**
 * @brief Each SERCOM peripheral gets its own SercomBusTransaction.
 *        This will eventually be replaced with a ringbuffer implementation
 *        so the non-blocking functions can function as they should.
 *
 * @todo Replace this implementation with a ringbuffer
 */
volatile bustransaction_t sercom_bustrans_buffer[6] = {{SERCOMACT_NONE, 0, NULL, NULL, 0, 0}, {SERCOMACT_NONE, 0, NULL, NULL, 0, 0},
                                                       {SERCOMACT_NONE, 0, NULL, NULL, 0, 0}, {SERCOMACT_NONE, 0, NULL, NULL, 0, 0},
                                                       {SERCOMACT_NONE, 0, NULL, NULL, 0, 0}, {SERCOMACT_NONE, 0, NULL, NULL, 0, 0}};

static inline void default_sercom_isr_handler(const void* const hw, volatile bustransaction_t* transaction) {
    Sercom* sercom_instance = ((Sercom*)hw);
    switch (transaction->transaction_type) {
        case SERCOMACT_IDLE_I2CS: {
            i2c_slave_handler(sercom_instance, transaction);
            break;
        }
        case SERCOMACT_I2C_DATA_TRANSMIT_NO_STOP:
        case SERCOMACT_I2C_DATA_TRANSMIT_STOP: {
            i2c_host_data_send_irq(sercom_instance, transaction);
            break;
        }
        case SERCOMACT_I2C_DATA_RECEIVE_STOP: {
            i2c_host_data_recv_irq(sercom_instance, transaction);
            break;
        }
        case SERCOMACT_SPI_DATA_RECEIVE: {
            spi_host_data_recv_irq(sercom_instance, transaction);
            break;
        }
        case SERCOMACT_SPI_DATA_TRANSMIT: {
            spi_host_data_send_irq(sercom_instance, transaction);
            break;
        }
        case SERCOMACT_IDLE_SPI_HOST: {
            uint8_t spi_intflag = sercom_instance->SPI.INTFLAG.reg;
            sercom_instance->SPI.INTFLAG.reg = spi_intflag;
            break;
        }
        case SERCOMACT_IDLE_SPI_SLAVE: {
            const uint8_t spi_intflag = sercom_instance->SPI.INTFLAG.reg;
            if (BITMASK_COMPARE(spi_intflag, SERCOM_SPI_INTFLAG_TXC)) {
                spi_slave_data_send_irq(sercom_instance, transaction);
            } else if (BITMASK_COMPARE(spi_intflag, SERCOM_SPI_INTFLAG_RXC)) {
                spi_slave_data_recv_irq(sercom_instance, transaction);
            } else if (BITMASK_COMPARE(spi_intflag, SERCOM_SPI_INTFLAG_SSL)) {
                spi_slave_chip_select_irq(sercom_instance, transaction);
            }
        }
        default: {
            uint8_t SPI_INTFLAG = sercom_instance->SPI.INTFLAG.reg;
            sercom_instance->SPI.INTFLAG.reg = SPI_INTFLAG;
            break;
        }
    }
}

__attribute__((used)) void SERCOM5_Handler(void) {
    volatile bustransaction_t* bustransaction = &sercom_bustrans_buffer[5];
    Sercom*                    sercom_instance = SERCOM5;
    default_sercom_isr_handler(sercom_instance, bustransaction);
}

__attribute__((used)) void SERCOM4_Handler(void) {
    volatile bustransaction_t* bustransaction = &sercom_bustrans_buffer[4];
    Sercom*                    sercom_instance = SERCOM4;
    default_sercom_isr_handler(sercom_instance, bustransaction);
}

__attribute__((used)) void SERCOM3_Handler(void) {
    volatile bustransaction_t* bustransaction = &sercom_bustrans_buffer[3];
    Sercom*                    sercom_instance = SERCOM3;
    default_sercom_isr_handler(sercom_instance, bustransaction);
}

__attribute__((used)) void SERCOM2_Handler(void) {
    volatile bustransaction_t* bustransaction = &sercom_bustrans_buffer[2];
    Sercom*                    sercom_instance = SERCOM2;
    default_sercom_isr_handler(sercom_instance, bustransaction);
}

__attribute__((used)) void SERCOM1_Handler(void) {
    volatile bustransaction_t* bustransaction = &sercom_bustrans_buffer[1];
    Sercom*                    sercom_instance = SERCOM1;
    default_sercom_isr_handler(sercom_instance, bustransaction);
}

__attribute__((used)) void SERCOM0_Handler(void) {
    volatile bustransaction_t* bustransaction = &sercom_bustrans_buffer[0];
    Sercom*                    sercom_instance = SERCOM0;
    default_sercom_isr_handler(sercom_instance, bustransaction);
}

__attribute__((used)) void EIC_Handler(void) {
    gpio_irq_handler(EIC);
}

__attribute__((used)) void NonMaskableInt_Handler(void) {
    gpio_irq_handler(EIC);
}