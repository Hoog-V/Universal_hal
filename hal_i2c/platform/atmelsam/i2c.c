#include <hal_i2c.h>
#include <stddef.h>
#include <sercom_dep.h>

void i2c_master_handler(const void *const hw, SercomData_t* Data) {
    Sercom* SercomInst = ((Sercom*)hw);
    bool hasToTransmit = (Data->CurrAction == SERCOMACT_I2C_DATA_TRANSMIT_STOP || Data->CurrAction == SERCOMACT_I2C_DATA_TRANSMIT_NO_STOP);
    if(hasToTransmit && Data->buf_cnt < Data->buf_size && Data->buf != NULL) {
	    SercomInst->I2CM.DATA.reg = Data->buf[Data->buf_cnt];
	    Data->buf_cnt++;
    }  else {
        const bool hasStop = (Data->CurrAction == SERCOMACT_I2C_DATA_TRANSMIT_STOP || Data->CurrAction == SERCOMACT_I2C_DATA_RECEIVE_STOP);
        if(hasStop) {
            SercomInst->I2CM.CTRLB.reg |= (SERCOM_I2CM_CTRLB_CMD(0x3));
            Data->CurrAction = SERCOMACT_NONE;
        }
        Data->buf_cnt = 0;
	    SercomInst->I2CM.INTFLAG.reg = 0x01;
    }
}

void i2c_slave_handler(const void *const hw, SercomData_t* Data) {
    Sercom* SercomInst = ((Sercom*)hw);
    const bool addressMatchInt = SercomInst->I2CS.INTFLAG.reg & SERCOM_I2CS_INTFLAG_AMATCH;
    const bool stopInt = SercomInst->I2CS.INTFLAG.reg & SERCOM_I2CS_INTFLAG_PREC;
    const bool dataReadyInt = SercomInst->I2CS.INTFLAG.reg & SERCOM_I2CS_INTFLAG_DRDY;
    const bool isReadTransaction = SercomInst->I2CS.STATUS.bit.DIR;
    if(addressMatchInt) {
        i2c_slave_adressmatch_irq(hw);
    }
    if(stopInt) {
        i2c_slave_stop_irq(hw);
    }
    if(dataReadyInt && isReadTransaction) {
        i2c_slave_data_send_irq(hw);
    }
    if(dataReadyInt && !isReadTransaction) {
        i2c_slave_data_recv_irq(hw);
    }
}

void i2c_master_send_data_irq(const void *const hw) {
    ((Sercom*)hw)->I2CM.INTFLAG.reg = 0x01;
    ((Sercom*)hw)->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    ((Sercom*)hw)->I2CM.CTRLB.reg |= (SERCOM_I2CM_CTRLB_CMD(0x3));
}
void i2c_master_recv_data_irq(const void *const hw) {
    ((Sercom*)hw)->I2CM.INTFLAG.reg = 0x01;
    ((Sercom*)hw)->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    ((Sercom*)hw)->I2CM.CTRLB.reg |= (SERCOM_I2CM_CTRLB_CMD(0x3));
}

void i2c_slave_data_recv_irq(const void *const hw) {
    ((Sercom*)hw)->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_DRDY;
}

void i2c_slave_data_send_irq(const void *const hw) {
    ((Sercom*)hw)->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_DRDY;
}

void i2c_slave_stop_irq(const void *const hw) {
    ((Sercom*)hw)->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_PREC;
}
void i2c_slave_adressmatch_irq(const void *const hw) {
    ((Sercom*)hw)->I2CS.INTFLAG.reg = SERCOM_I2CS_INTFLAG_AMATCH;
}

void SERCOM5_Handler(void)
{
	if(SERCOMData[5].CurrInterface == SERCOM_INT_I2CM) {
    i2c_master_handler(SERCOM5, &SERCOMData[5]);
	}
}

void SERCOM4_Handler(void)
{
	if(SERCOMData[4].CurrInterface == SERCOM_INT_I2CM) {
    i2c_master_handler(SERCOM4, &SERCOMData[4]);
	} else if(SERCOMData[4].CurrInterface == SERCOM_INT_I2CS) {
        i2c_slave_handler(SERCOM4, &SERCOMData[4]);
    }
}
void SERCOM3_Handler(void)
{
	if(SERCOMData[3].CurrInterface == SERCOM_INT_I2CM) {
    i2c_master_handler(SERCOM3, &SERCOMData[3]);
	}
}

void SERCOM2_Handler(void)
{
	if(SERCOMData[2].CurrInterface == SERCOM_INT_I2CM) {
    i2c_master_handler(SERCOM2, &SERCOMData[2]);
	}
}

void SERCOM1_Handler(void)
{
	if(SERCOMData[1].CurrInterface == SERCOM_INT_I2CM) {
    i2c_master_handler(SERCOM1, &SERCOMData[1]);
	}
}


static inline void sercomi2cm_wait_for_sync(const void *const hw, const uint32_t reg)
{
	while (((Sercom *)hw)->I2CM.SYNCBUSY.reg & reg) {
	};
}

static inline void sercomi2cs_wait_for_sync(const void *const hw, const uint32_t reg)
{
	while (((Sercom *)hw)->I2CS.SYNCBUSY.reg & reg) {
	};
}

static inline void disableSercom(const void *const hw) {
    ((Sercom*)hw)->I2CM.CTRLA.reg &= ~SERCOM_I2CM_CTRLA_ENABLE;
    const uint32_t waitflags = (SERCOM_I2CM_SYNCBUSY_SWRST | SERCOM_I2CM_SYNCBUSY_ENABLE);
    sercomi2cm_wait_for_sync(hw, waitflags);
}

uint16_t sercomi2cm_read_STATUS_BUSSTATE_bf(const void *const hw) {
	sercomi2cm_wait_for_sync(hw, SERCOM_I2CM_SYNCBUSY_SYSOP);
	return (((Sercom *)hw)->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSSTATE_Msk) >> SERCOM_I2CM_STATUS_BUSSTATE_Pos;
}

static inline void hri_sercomi2cm_clear_STATUS_reg(const void *const hw, uint16_t mask) {
	 ((Sercom*)hw)->I2CM.STATUS.reg = mask;
	 sercomi2cm_wait_for_sync(hw, SERCOM_I2CM_SYNCBUSY_SYSOP);
}


void i2c_init(const I2CInst* I2C_instance, const unsigned long baudate) {
    const bool InvalidSercomInstNum = (I2C_instance->Sercom_inst_num < 0 || I2C_instance->Sercom_inst_num > 5);
    const bool InvalidSercomInst = (I2C_instance->SercomInst == NULL);
    const bool InvalidClockGen = (I2C_instance->ClockGenSlow < 0 || I2C_instance->ClockGenSlow > 6 || I2C_instance->ClockGenFast < 0 || I2C_instance->ClockGenFast > 6);
if (InvalidSercomInst || InvalidSercomInstNum || InvalidClockGen){ 
    return;
}
// Set the clock system
#ifdef __SAMD51__

#else
PM->APBCMASK.reg |= 1 << (PM_APBCMASK_SERCOM0_Pos + I2C_instance->Sercom_inst_num);
GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(I2C_instance->ClockGenSlow)  | GCLK_CLKCTRL_ID_SERCOMX_SLOW | GCLK_CLKCTRL_CLKEN;
while (GCLK->STATUS.bit.SYNCBUSY)
		;
GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(I2C_instance->ClockGenFast) | ((GCLK_CLKCTRL_ID_SERCOM0_CORE_Val+I2C_instance->Sercom_inst_num) << GCLK_CLKCTRL_ID_Pos) | GCLK_CLKCTRL_CLKEN;
GCLK->GENDIV.reg = GCLK_GENDIV_DIV(0x01) | GCLK_GENDIV_ID(I2C_instance->ClockGenFast);
while (GCLK->STATUS.bit.SYNCBUSY)
		;
#endif


const bool SlaveConfiguration = (I2C_instance->OpMode == I2COpModeSlave && I2C_instance->I2CAddr != 0);
if(SlaveConfiguration) {
    i2c_set_slave_mode(I2C_instance, I2C_instance->I2CAddr);
	SERCOMData[I2C_instance->Sercom_inst_num].CurrInterface = SERCOM_INT_I2CS;
} else {
    Sercom* SercomInst = I2C_instance->SercomInst;
    const bool SercomEnabled = SercomInst->I2CM.CTRLA.bit.ENABLE;
    if(SercomEnabled) disableSercom(SercomInst);
    SercomInst->I2CM.CTRLA.reg =  (SERCOM_I2CM_CTRLA_SWRST |SERCOM_I2CM_CTRLA_MODE(5));
    const uint32_t waitflags = (SERCOM_I2CM_SYNCBUSY_SWRST | SERCOM_I2CM_SYNCBUSY_ENABLE);
    sercomi2cm_wait_for_sync(SercomInst, waitflags);
    SercomInst->I2CM.CTRLA.reg = ( 0 << SERCOM_I2CM_CTRLA_LOWTOUTEN_Pos      /* SCL Low Time-Out: disabled */
	        | 0 << SERCOM_I2CM_CTRLA_INACTOUT_Pos /* Inactive Time-Out: 0 */
	        | 0 << SERCOM_I2CM_CTRLA_SCLSM_Pos    /* SCL Clock Stretch Mode: disabled */
	        | 0 << SERCOM_I2CM_CTRLA_SPEED_Pos    /* Transfer Speed: 0 */
	        | 0 << SERCOM_I2CM_CTRLA_SEXTTOEN_Pos /* Slave SCL Low Extend Time-Out: disabled */
	        | 0 << SERCOM_I2CM_CTRLA_MEXTTOEN_Pos /* Master SCL Low Extend Time-Out: 0 */
	        | 0b10 << SERCOM_I2CM_CTRLA_SDAHOLD_Pos  /* SDA Hold Time: 0 */
	        | 0 << SERCOM_I2CM_CTRLA_PINOUT_Pos   /* Pin Usage: disabled */
	        | 0 << SERCOM_I2CM_CTRLA_RUNSTDBY_Pos /* Run In Standby: disabled */
	        | 5 << SERCOM_I2CM_CTRLA_MODE_Pos);

    sercomi2cm_wait_for_sync(SercomInst, SERCOM_I2CM_SYNCBUSY_MASK);
	SercomInst->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    SercomInst->I2CM.BAUD.reg = 0xFF;
	int timeout         = 65535;
	int timeout_attempt = 4;
	SercomInst->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
	sercomi2cm_wait_for_sync(SercomInst, SERCOM_I2CM_SYNCBUSY_SWRST | SERCOM_I2CM_SYNCBUSY_ENABLE);
	while (sercomi2cm_read_STATUS_BUSSTATE_bf(SercomInst) != 0x1) {
		timeout--;

		if (timeout <= 0) {
			if (--timeout_attempt)
				timeout = 65535;
			else
				return;
			hri_sercomi2cm_clear_STATUS_reg(SercomInst, SERCOM_I2CM_STATUS_BUSSTATE(0x1));
		}
	}
	SercomInst->I2CM.INTENSET.reg = SERCOM_I2CM_INTENSET_MB | SERCOM_I2CM_INTENSET_SB;
	SERCOMData[I2C_instance->Sercom_inst_num].CurrInterface = SERCOM_INT_I2CM;
}	
	const enum IRQn irq_type = (SERCOM0_IRQn + I2C_instance->Sercom_inst_num);
	NVIC_EnableIRQ(irq_type);
	NVIC_SetPriority(irq_type, 2);
}

void i2c_deinit(const I2CInst* I2C_instance) {
	disableSercom(I2C_instance->SercomInst);
}

void i2c_set_baudrate(const I2CInst* I2C_instance, const unsigned long baudate) {

}

void i2c_set_slave_mode(const I2CInst* I2C_instance, const unsigned short addr) {
Sercom* SercomInst = I2C_instance->SercomInst;
const bool SercomEnabled = SercomInst->I2CM.CTRLA.bit.ENABLE;
if(SercomEnabled) disableSercom(SercomInst);
SercomInst->I2CS.CTRLA.reg = (SERCOM_I2CS_CTRLA_SWRST | SERCOM_I2CS_CTRLA_MODE(4));
sercomi2cs_wait_for_sync(SercomInst, SERCOM_I2CS_SYNCBUSY_SWRST);
SercomInst->I2CS.CTRLA.reg = (0 << SERCOM_I2CS_CTRLA_LOWTOUTEN_Pos      /* SCL Low Time-Out: disabled */
	        | 0 << SERCOM_I2CS_CTRLA_SCLSM_Pos    /* SCL Clock Stretch Mode: disabled */
	        | 0 << SERCOM_I2CS_CTRLA_SPEED_Pos    /* Transfer Speed: 0 */
	        | 0 << SERCOM_I2CS_CTRLA_SEXTTOEN_Pos /* Slave SCL Low Extend Time-Out: disabled */
	        | 0b10 << SERCOM_I2CS_CTRLA_SDAHOLD_Pos  /* SDA Hold Time: 0 */
	        | 0 << SERCOM_I2CS_CTRLA_PINOUT_Pos   /* Pin Usage: disabled */
	        | 0 << SERCOM_I2CS_CTRLA_RUNSTDBY_Pos /* Run In Standby: disabled */
	        | 4 << SERCOM_I2CS_CTRLA_MODE_Pos);
SercomInst->I2CS.CTRLB.reg |= SERCOM_I2CS_CTRLB_SMEN;
sercomi2cs_wait_for_sync(SercomInst, SERCOM_I2CS_SYNCBUSY_MASK);
SercomInst->I2CS.ADDR.reg = (0 << SERCOM_I2CS_ADDR_ADDRMASK_Pos       /* Address Mask: 0 */
	                      | 0 << SERCOM_I2CS_ADDR_TENBITEN_Pos /* Ten Bit Addressing Enable: disabled */
	                      | 0 << SERCOM_I2CS_ADDR_GENCEN_Pos   /* General Call Address Enable: disabled */
                          | (addr) << SERCOM_I2CS_ADDR_ADDR_Pos);
SercomInst->I2CS.CTRLA.reg |= SERCOM_I2CS_CTRLA_ENABLE;
SercomInst->I2CS.INTENSET.reg = SERCOM_I2CS_INTENSET_AMATCH | SERCOM_I2CS_INTENSET_PREC | SERCOM_I2CS_INTENSET_DRDY;
}

void i2c_write_non_blocking(const I2CInst* I2C_instance, const unsigned short addr, const unsigned char* write_buff, const unsigned char size, bool stop_bit) {
    const bool prevTransactionFinished = SERCOMData[I2C_instance->Sercom_inst_num].CurrAction == SERCOMACT_NONE;
	sercomi2cm_wait_for_sync((I2C_instance->SercomInst), SERCOM_I2CM_SYNCBUSY_SYSOP);
	SERCOMData[I2C_instance->Sercom_inst_num].buf = write_buff;
	SERCOMData[I2C_instance->Sercom_inst_num].buf_size = size;
	SERCOMData[I2C_instance->Sercom_inst_num].CurrAction = stop_bit ? SERCOMACT_I2C_DATA_TRANSMIT_STOP : SERCOMACT_I2C_DATA_TRANSMIT_NO_STOP;
    I2C_instance->SercomInst->I2CM.ADDR.reg = (addr << 1);
	sercomi2cm_wait_for_sync((I2C_instance->SercomInst), SERCOM_I2CM_SYNCBUSY_SYSOP);
}

void i2c_write_blocking(const I2CInst* I2C_instance, const unsigned char addr, const unsigned char* write_buff, const unsigned char size, bool stop_bit) {
	i2c_write_non_blocking(I2C_instance, addr, write_buff, size, stop_bit);
	while(!(I2C_instance->SercomInst->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSSTATE(0x1) ||
            I2C_instance->SercomInst->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_CLKHOLD));
}



char i2c_read_blocking(const I2CInst* I2C_instance, const unsigned short addr, unsigned char* read_buff) {

 return -1;
}

char i2c_read_non_blocking(const I2CInst* I2C_instance, const unsigned short addr, unsigned char* read_buff) {
 return -1;
}
