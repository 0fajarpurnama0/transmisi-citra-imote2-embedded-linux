/*
 * driver/tosmac/SG2_CC2420.h
 * Author: Gefan Zhang
 */

#ifndef _HPLCC2420_H
#define _HPLCC2420_H

#define SUCCESS 1
#define FAIL    0
#define TRUE  1
#define FALSE 0

uint8_t HPLCC2420_init(void);
void    HPLCC2420_exit(void);
uint8_t HPLCC2420_start(void);
uint8_t CC2420_cmd_strobe(uint8_t addr);
uint16_t CC2420_read_reg(uint8_t addr);
uint8_t CC2420_write_reg(uint8_t addr, uint16_t data);
uint8_t CC2420_read_RAM(uint16_t addr, uint8_t length, uint8_t *buffer);
uint8_t CC2420_write_RAM(uint16_t addr, uint8_t length, uint8_t *buffer);
uint8_t CC2420_read_RXFIFO(uint8_t* length, uint8_t *data);
uint8_t CC2420_write_TXFIFO(uint8_t length, uint8_t *data);

#define TEST_FIFO_PIN()  (GPLR(SG2_GPIO114_CC_FIFO) & GPIO_bit(SG2_GPIO114_CC_FIFO))
#define TEST_FIFOP_PIN() (GPLR(SG2_GPIO0_CC_FIFOP) & GPIO_bit(SG2_GPIO0_CC_FIFOP))
#define TEST_CCA_PIN()   (GPLR(SG2_GPIO116_CC_CCA) & GPIO_bit(SG2_GPIO116_CC_CCA))
#define TEST_SFD_PIN()   (GPLR(SG2_GPIO16_CC_SFD) & GPIO_bit(SG2_GPIO16_CC_SFD))

#endif

