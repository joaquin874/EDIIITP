/*
 * TP FINAL EDII
 */


#include "LPC17xx.h"


//#include <cr_section_macros.h>

#define SRAM0 0x2007C000

//function that generates samples of the signal to be outputed by the DAC
void signal_generator(){
    uint16_t *mem_address = (uint16_t *) SRAM0;
    *mem_address = 0x1023;
    mem_address++;
    *mem_address = 0x0000;
    return;
}

void pin_config(){
    LPC_GPIO1->FIODIR |= 0xFF0000;
    LPC_PINCON->PINSEL4 |= ( (1<<20));
}

void eint_config(){
    LPC_PINCON->PINSEL4 |= (1<<20);

    LPC_SC->EXTMODE |= 0x03;
    LPC_SC->EXTPOLAR |= 0x03;
    LPC_SC->EXTINT |= 0x03;

    LPC_SC->EXTMODE |= (1<<3);
    LPC_SC->EXTPOLAR |= (1<<3);
    NVIC_EnableIRQ(EINT3_IRQn);
    LPC_SC->EXTINT |= (1<<3);
}

/*
void adc_config(){

}
*/

/**
 * Configures the DAC (Digital-to-Analog Converter) to output analog signals through pin P0.26.
 * This function initializes the DAC, sets the clock for the DAC, and enables the DAC counter and DMA.
 */
void dac_config(){
    LPC_PINCON->PINSEL1 |= (2<<20);  //Config AOUT
    LPC_PINCON->PINMODE1 |= (2<<20);
    DAC_CONVERTER_CFG_Type dacConfig;
    dacConfig.CNT_ENA = SET;
    dacConfig.DMA_ENA = SET;

    DAC_Init(LPC_DAC);
    LPC_SC->PCLKSEL0 |= (1<<22);
    DAC_SetDMATimeOut(LPC_DAC, 100000000);
    DAC_ConfigDAConverterControl(LPC_DAC, &dacConfig);
}

/**
 * Configures Timer1 with a prescaler of 99 and a match value of 500000.
 * This means the timer will interrupt every 0.5 seconds.
 * Enables reset and interrupt on match value.
 * Clears interrupt flags and enables Timer1 interrupt.
 */
void timer_config(){
    LPC_SC->PCLKSEL0 |= (1<<2); //PCLK = CCLK
    LPC_TIM0->PR = 99; //Prescaler = 99
    LPC_TIM0->MR0 = 5000000;
    LPC_TIM0->MCR |= (1<<1); //Reset on MR0
    LPC_TIM0->EMR |= (1<<0); //Toggle on MR0
    LPC_TIM0->EMR |= (0x03<<4); //Toggle on MR0
    LPC_TIM0->TCR |= 0x03; //Reseteo y habilito el timer0
    LPC_TIM0->TCR &=~ 0x02; //Deshabilito el reset del timer0
}


/**
 * Configures the DMA controller to transfer data from SRAM0 to the DACR register of the LPC_DAC peripheral.
 * Uses a linked list item (LLI) to define the transfer parameters.
 * Initializes and sets up the GPDMA channel 0 with the defined parameters.
 */
void dma_config(){
    GPDMA_LLI_Type LLI;
    LLI.SrcAddr = (uint32_t) SRAM0;
    LLI.DstAddr = (uint32_t) DACR &LPC_DAC->DACR;
    LLI.NextLLI = (uint32_t) &LLI;
    LLI.Control = 0x02
				|(2<<18) //source width 32 bits
				|(2<<21) //dest width 32 bits
                |(1<<26); //source increment
    GPDMA_Init();
    GPDMA_Channel_CFG_Type gpdma_cfg1;
    GPDMACfg1.ChannelNum = 0;
	GPDMACfg1.SrcMemAddr = (uint32_t)SRAM0;
	GPDMACfg1.DstMemAddr = 0;
	GPDMACfg1.TransferSize = 0x02;
	GPDMACfg1.TransferWidth = 0;
	GPDMACfg1.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg1.SrcConn = 0;
	GPDMACfg1.DstConn = GPDMA_CONN_DAC;
	GPDMACfg1.DMALLI = (uint32_t)&LLI;
	GPDMA_Setup(&GPDMACfg1);
    GPDMA_ChannelCmd(1, ENABLE);
}
/*
void uart_config(){

}
*/

int main(void) {
    pin_config();
    //adc_config();
    dac_config();
    //timer_config();
    dma_config();
    while(1) {
        //TODO
    }
    return 0 ;
}