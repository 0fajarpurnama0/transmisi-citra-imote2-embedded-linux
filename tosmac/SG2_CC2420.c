/*
 *   driver/tosmac/SG2_CC2420.c
 *
 *   TOS_MAC Driver based on CC2420 radio chip
 *
 *   Author: Gefan Zhang <gefan@soe.ucsc.edu>
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/system.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/stargate2.h>
#include <asm/arch/ssp.h>
#include "../platx/pmic.h"

#include "SG2_CC2420.h"
#include "CC2420Const.h"

static struct ssp_dev SG2_ssp_dev;
static DEFINE_SPINLOCK(SG2_ssp_lock);

// FIXME: THis function should be removed and replaced
// with calls to mdelay or something -- it's 
// virtually meaningless in the context of the PXA
void SG2_wait(void)
{
    asm volatile("nop");
    asm volatile("nop");
}

static void SG2_set_pin_directions(void)
{
    pxa_gpio_mode(SG2_GPIO22_CC_RSTN | GPIO_OUT); 
    GPSR(SG2_GPIO22_CC_RSTN) = GPIO_bit(SG2_GPIO22_CC_RSTN);
    pxa_gpio_mode(SG2_GPIO114_CC_FIFO | GPIO_IN);
    pxa_gpio_mode(SG2_GPIO116_CC_CCA | GPIO_IN);
    pxa_gpio_mode(SG2_GPIO0_CC_FIFOP | GPIO_IN);
    GPSR(SG2_GPIO0_CC_FIFOP) = GPIO_bit(SG2_GPIO0_CC_FIFOP);
    pxa_gpio_mode(SG2_GPIO16_CC_SFD | GPIO_IN);
    pxa_gpio_mode(SG2_GPIO39_CC_SFRM | GPIO_OUT);   //CSn

    // Oscilloscope trigger for debugging
    //pxa_gpio_mode(93 | GPIO_OUT);
    //GPCR(93) = GPIO_bit(93);
}


uint8_t CC2420_cmd_strobe(uint8_t addr)
{
    uint8_t status = 0;
    unsigned long flag;

    spin_lock_irqsave(&SG2_ssp_lock, flag);
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);

    /* trigger for debugging */
    //GPSR(93) = GPIO_bit(93);
    //GPCR(93) = GPIO_bit(93);

    /* Activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    udelay(1);
    
    /* set command strobe */
//    while (!(SSSR_P((&SG2_ssp_dev)->port) & SSSR_TNF))
//	cpu_relax();

    SSDR_P((&SG2_ssp_dev)->port) = addr;

    /* the BSY and TFL fields need to be tested in the same read */
    while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL))
      udelay(1);

    /* Deactivate CSn */
    GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    /* read status of  CC2420  */
    status = SSDR_P((&SG2_ssp_dev)->port);
    
    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    return status;
}

uint8_t CC2420_write_reg(uint8_t addr, uint16_t data) 
{
    uint8_t status = 0;
    unsigned long flag;

    spin_lock_irqsave(&SG2_ssp_lock, flag);
    
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
//    while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_RNE){
//	tmp = SSDR_P((&SG2_ssp_dev)->port);
//	printk(KERN_INFO "rx is not empty\n");
//    }
    /* Activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    /* set cc2420 16 bits registers */
//    while (!(SSSR_P((&SG2_ssp_dev)->port) & SSSR_TNF))
//	cpu_relax();
    SSDR_P((&SG2_ssp_dev)->port) = addr;
    SSDR_P((&SG2_ssp_dev)->port) = ((data >> 8) & 0xFF);
    SSDR_P((&SG2_ssp_dev)->port) = (data & 0xFF);
    while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL))
      udelay(1);

    /* Deactivate CSn */
    GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);
    
    /* read status of CC2420 */
    status = SSDR_P((&SG2_ssp_dev)->port);

    /* empty the PXA receive fifo again*/
    ssp_flush(&SG2_ssp_dev);

    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    
    return status;
}

uint16_t CC2420_read_reg(uint8_t addr) 
{
    uint16_t data = 0;
    uint8_t tmp;
    unsigned long flag;
    
    spin_lock_irqsave(&SG2_ssp_lock, flag);
    /* empty the receive fifo */
    ssp_flush(&SG2_ssp_dev);

    /* activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    /* set cc2420 16 bits registers */
//    while (!(SSSR_P((&SG2_ssp_dev)->port) & SSSR_RNE))
//	cpu_relax();
    SSDR_P((&SG2_ssp_dev)->port) = (addr | 0x40);
    SSDR_P((&SG2_ssp_dev)->port) = 0;
    SSDR_P((&SG2_ssp_dev)->port) = 0;
    while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL));
    udelay(1);
    /* deactivate CSn */
    GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    tmp =  SSDR_P((&SG2_ssp_dev)->port);
    data =  SSDR_P((&SG2_ssp_dev)->port);
    data = ((data<<8) & 0xFF00);
    data |= SSDR_P((&SG2_ssp_dev)->port);

    /* empty the PXA receive fifo again*/
    ssp_flush(&SG2_ssp_dev);
    
    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    return data;
}

/* read RAM in CC2420, not being tested yet*/
uint8_t CC2420_read_RAM(uint16_t addr, uint8_t length, uint8_t *buffer)
{
    uint32_t temp32;
    uint8_t i=0;
    unsigned long flag;

    spin_lock_irqsave(&SG2_ssp_lock, flag);
   
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    
    /* activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    SSDR_P((&SG2_ssp_dev)->port) = ((addr & 0x7F) | 0x80);
    SSDR_P((&SG2_ssp_dev)->port) = (((addr >> 1) & 0xC0) | 0x20);
    
    while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_BSY);
    
    while(length > 16){
       for(i = 0; i < 16; i++) {
          SSDR_P((&SG2_ssp_dev)->port) = 0;
       }
       while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_BSY);
       for(i = 0; i < 16; i++) {
          temp32 = SSDR_P((&SG2_ssp_dev)->port);
	  *buffer++ = temp32;
       }
       length -= 16;
    }
    for(i = 0; i < length; i++) {
       SSDR_P((&SG2_ssp_dev)->port) = 0;
    }
    while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_BSY);
    for(i = 0; i < length; i++) {
       temp32 = SSDR_P((&SG2_ssp_dev)->port);
       *buffer++ = temp32;
    }
 
    /* deactivate CSn */
    GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    //ret = SUCCESS;
    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    return SUCCESS;
}

uint8_t CC2420_write_RAM(uint16_t addr, uint8_t length, uint8_t *buffer)
{
    uint8_t i =0;
    uint8_t tmp;
    unsigned long flag; 
    spin_lock_irqsave(&SG2_ssp_lock, flag);
    
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);

    /* activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);
    
    SSDR_P((&SG2_ssp_dev)->port) = ((addr & 0x7F) | 0x80);
    SSDR_P((&SG2_ssp_dev)->port) = ((addr >> 1) & 0xC0);
 
    while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_BSY);
    
    while(length > 16) {
	for (i=0; i<16; i++) {
    	    SSDR_P((&SG2_ssp_dev)->port) = *buffer++;
	}
        while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_BSY);
        length -= 16;
    }

    for(i=0;i<length;i++) {
       SSDR_P((&SG2_ssp_dev)->port) = *buffer++;
    }
    while(SSSR_P((&SG2_ssp_dev)->port) & SSSR_BSY);
    /* clear FIFO */
    for(i=0; i<16; i++) {
       tmp = SSDR_P((&SG2_ssp_dev)->port);// = (CC2420_TXFIFO);
    }

    /* deactivate CSn */
    GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    return SUCCESS;
}

uint8_t CC2420_read_RXFIFO(uint8_t *length, uint8_t *data)
{
    uint32_t temp32;
    uint8_t status;
    uint8_t pktlen;
    uint8_t ret;
    int i;
    unsigned long flag;

    spin_lock_irqsave(&SG2_ssp_lock, flag);
   
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    
    /* activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    SSDR_P((&SG2_ssp_dev)->port) = (CC2420_RXFIFO | 0x40);
    SSDR_P((&SG2_ssp_dev)->port) = 0;
    
    while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY| SSSR_TFL));
    status = SSDR_P((&SG2_ssp_dev)->port);
    pktlen = SSDR_P((&SG2_ssp_dev)->port);

    pktlen++; // include length
    pktlen = (pktlen < *length) ? pktlen : *length;
    *length = pktlen;
    *data++ = pktlen;
    pktlen--; // but don't read it back again

    if(pktlen > 0){
        while(pktlen > 16){
	  for(i = 0; i < 16; i++)
	    SSDR_P((&SG2_ssp_dev)->port) = 0xFF;
	  while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY|SSSR_TFL));
	  for(i = 0; i < 16; i++) {
	    temp32 = SSDR_P((&SG2_ssp_dev)->port);
	    *data++ = temp32;
	  }
	  pktlen -= 16;
        }
        for(i = 0; i < pktlen; i++)
          SSDR_P((&SG2_ssp_dev)->port) = 0xFF;

        while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL));
        for(i = 0; i < pktlen; i++) {
           temp32 = SSDR_P((&SG2_ssp_dev)->port);
	   *data++ = temp32;
        }

 	udelay(1);
        /* deactivate CSn */
        GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);
        ret = SUCCESS;
    }
    else {
        GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);
        ret = FAIL;   
    }
    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    return ret;
}


uint8_t CC2420_write_TXFIFO(uint8_t length, uint8_t *data)
{
    int i;
    unsigned long flag; 
    spin_lock_irqsave(&SG2_ssp_lock, flag);

//    printk(KERN_INFO "length is %d\n", length);
    
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    /* activate CSn */
    GPCR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);
    ndelay(1);    

    SSDR_P((&SG2_ssp_dev)->port) = CC2420_TXFIFO;
    while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL));
    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    
    while(length > 16) {
	for (i=0; i<16; i++) {
    	    SSDR_P((&SG2_ssp_dev)->port) = *data++;
	}
        while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL));
        ssp_flush(&SG2_ssp_dev);	//clear RXFIFO, avoid ROR exception
        length -= 16;
    }
    for(i=0;i<length;i++) {
       SSDR_P((&SG2_ssp_dev)->port) = *data++;
    }
    while(SSSR_P((&SG2_ssp_dev)->port) & (SSSR_BSY | SSSR_TFL));

    /* clear FIFO */
//   for(i=0; i<16; i++) {
//     tmp = SSDR_P((&SG2_ssp_dev)->port);
// }
    
    /* deactivate CSn */
    GPSR(SG2_GPIO39_CC_SFRM) = GPIO_bit(SG2_GPIO39_CC_SFRM);

    /* empty the PXA receive fifo */
    ssp_flush(&SG2_ssp_dev);
    spin_unlock_irqrestore(&SG2_ssp_lock, flag);
    return SUCCESS;
}

uint8_t HPLCC2420_init(void)
{
    // set GPIO pin directions
    SG2_set_pin_directions();
    // set GPIO pins' alt functions according to CC2420
    pxa_gpio_mode(SG2_GPIO34_SSPSCLK);  // CLK
    pxa_gpio_mode(SG2_GPIO35_SSPTXD);   // SI
    pxa_gpio_mode(SG2_GPIO41_SSPRXD);   // SO
   
    return SUCCESS;
}

uint8_t HPLCC2420_start()
{
   int result;
   uint8_t value;
   // SSP3 init
   result = ssp_init(&SG2_ssp_dev,3);
   if (result)
	printk(KERN_ERR "Unable to register SSP3 handler!\n");
   else {
        ssp_disable(&SG2_ssp_dev);
	// Serial clock rate = 6.5MHz (13M/2)
	// Frame Format = SPI
	// Data Size = 8-bit 
        ssp_config(&SG2_ssp_dev, (SSCR0_Motorola | (SSCR0_DSS & 0x07 )),
		   (SSCR1_TRAIL | SSCR1_TxTresh(8) | SSCR1_RxTresh(8)),
		   0, SSCR0_SCR & (1 << 8));
	SSTO_P((&SG2_ssp_dev)->port) = (96*8);
        ssp_enable(&SG2_ssp_dev);
   }

   // power on and VREG_EN is also set
   readPMIC(PMIC_B_REG_CONTROL_1, &value,1);
   value |= BRC1_LDO5_EN ;
   writePMIC(PMIC_B_REG_CONTROL_1, value);
   SG2_wait();
  
   if(!is_stargate2()) {
	printk(KERN_INFO "This board is IMote2\n");
        GPSR(IM2_GPIO115_CC_VREG) = GPIO_bit(IM2_GPIO115_CC_VREG);
	udelay(600); 
   } else
	printk(KERN_INFO "This board is SG2\n");   
   
   //toggle RESETn 
   GPCR(SG2_GPIO22_CC_RSTN) = GPIO_bit(SG2_GPIO22_CC_RSTN);
   SG2_wait();
   GPSR(SG2_GPIO22_CC_RSTN) = GPIO_bit(SG2_GPIO22_CC_RSTN);
   SG2_wait();

   return SUCCESS;
}

void HPLCC2420_exit(void)
{
    uint8_t value; 

   /* disable SSP3 port */
   ssp_exit(&SG2_ssp_dev);

//   if(!is_stargate2()) {
//        GPCR(IM2_GPIO115_CC_VREG) = GPIO_bit(IM2_GPIO115_CC_VREG);
//	udelay(600); 
//   }


   /* power off */
   readPMIC(PMIC_B_REG_CONTROL_1, &value,1);
   value &= ~BRC1_LDO5_EN ;
   writePMIC(PMIC_B_REG_CONTROL_1, value);
}


EXPORT_SYMBOL(HPLCC2420_init);
EXPORT_SYMBOL(HPLCC2420_exit);
EXPORT_SYMBOL(CC2420_cmd_strobe);
EXPORT_SYMBOL(CC2420_read_reg);
EXPORT_SYMBOL(CC2420_write_reg);
EXPORT_SYMBOL(CC2420_read_RXFIFO);
EXPORT_SYMBOL(CC2420_write_TXFIFO);
EXPORT_SYMBOL(CC2420_read_RAM);
EXPORT_SYMBOL(CC2420_write_RAM);
