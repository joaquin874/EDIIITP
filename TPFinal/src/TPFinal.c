/*
 * TP FINAL EDII
 */

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"


#include <cr_section_macros.h>

#define SRAM0 0x2007C000
#define BaudRate 38400
#define SAMPLES 100
#define PCLK_DAC_IN_MHZ 25


uint32_t dac_pwm[SAMPLES];
uint32_t dac_pwm_sg90[SAMPLES*5];

/**
 * Generates a PWM signal
 * Based on the duty_cycle value, this function writes a 1 or a 0 to the memory address
 * 100 times, in order to generate a PWM signal that can variate from a 1% to 100% duty cycle.
 */
void pwm_generator(uint8_t duty_cycle){
    //uint32_t *mem_address = (uint32_t *) SRAM0;

    for(int duty_index = 0; duty_index < 100; duty_index++){
        if(duty_index < duty_cycle){
            dac_pwm[duty_index] = (1023 << 6);
        }
        else{
        	dac_pwm[duty_index] = 0;
        }
    }
}

/**
 * Generates a PWM signal by
 * Based on the state value, this function writes a 1 or a 0 to the memory address
 * 100 times, in order to generate a PWM signal that can variate from a 1% to 100% duty cycle.
 * Servomotor SG90 works on 50Hz ===> T = 20ms
 * Duty Cycle 1ms - 2ms = 5% - 10%
 * state = 1 open
 * state = 0 closed
 */
void pwm_servo_generator(uint8_t state){
    if(state == 1){
        int duty_cycle = 5;
        for(int i = 0; i < 500; i++){
            if(i < duty_cycle){
                dac_pwm_sg90[i] = (1023 << 6);
            }
            else{
        	    dac_pwm_sg90[i] = 0;
            }
            if((i+1) % 100 == 0){
                duty_cycle++;
            }
        }
    }
    else if(state == 0){
        int duty_cycle = 10;
        for(int i = 0; i < 500; i++){
            if(i < duty_cycle){
                dac_pwm_sg90[i] = (1023 << 6);
            }
            else{
        	    dac_pwm_sg90[i] = 0;
            }
            if((i+1) % 100 == 0){
                duty_cycle--;
            }
        }
    }

}
/**
 * Configures external interrupt 0 (EINT0) on pin P2.10.
 * Sets the pin mode to pull-up resistor.
 * Sets the interrupt mode to edge-sensitive.
 * Sets the interrupt polarity to active high.
 * Enables the interrupt in the NVIC.
 */
void eint_config(){
    LPC_PINCON->PINSEL4 |= (1<<20); //P2.10 as EINT0
    LPC_PINCON->PINMODE4 |= (3<<20);

    LPC_SC->EXTMODE = 1;
    LPC_SC->EXTPOLAR = 1;
    LPC_SC->EXTINT = 1;

    NVIC_EnableIRQ(EINT0_IRQn);
    return;
}


/**
 * This function initializes the ADC module with a frequency of 200kHz, disables burst mode,
 * powers down the ADC, enables channel 0, starts conversion on 8 channels and configures the
 * start edge as falling edge.
 */
void adc_config(void){
	LPC_PINCON->PINSEL1 |= (1<<16);
	LPC_PINCON->PINMODE1 |= (2<<16);

	ADC_Init(LPC_ADC, 100000);
    //ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_ChannelCmd(LPC_ADC, 1, ENABLE);
    ADC_StartCmd(LPC_ADC, 4);
    ADC_EdgeStartConfig(LPC_ADC, 0);
    ADC_IntConfig(LPC_ADC, 1, ENABLE);
    NVIC_EnableIRQ(ADC_IRQn);
    return;
}


 /* Configures the DAC (Digital-to-Analog Converter) to output analog signals through pin P0.26.
 * This function initializes the DAC, sets the clock for the DAC, and enables the DAC counter and DMA.
 */
void dac_config_buzzer(void){
	PINSEL_CFG_Type PinCfg;
	/*
	 * Init DAC pin connect
	 * AOUT on P0.26
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	//uint32_t tmp;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
	DAC_ConverterConfigStruct.CNT_ENA =SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_Init(LPC_DAC);
	/* set time out for DAC*/
	//tmp = (PCLK_DAC_IN_MHZ*1000000)/(1*SAMPLES);
	//DAC_SetDMATimeOut(LPC_DAC,tmp);

	/*
    si quiero una senial de 1000Hz
    el tiempo de cada periodo es de 1ms
    el tiempo de cada muestra es de 10us
    1 cuenta ---- 1x10^-8seg
    x cuentas ---- 10x10^-6seg
    x = 1000
    entonces debo sacar una nueva muestra de la senial
    cada 1000 cuentas, que son 10x10^-6seg = 10us
	 */

	DAC_SetDMATimeOut(LPC_DAC,1000);
	DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
	return;
}

void dac_config_s90(void){
	PINSEL_CFG_Type PinCfg;
		/*
		 * Init DAC pin connect
		 * AOUT on P0.26
		 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	uint32_t tmp;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
	DAC_ConverterConfigStruct.CNT_ENA =SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_Init(LPC_DAC);
	/* set time out for DAC*/
	tmp = (PCLK_DAC_IN_MHZ*1000000)/(50*SAMPLES);
	DAC_SetDMATimeOut(LPC_DAC,tmp);
	DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
	return;
}

void confPin(void){
	PINSEL_CFG_Type PinCfg;
	/*
	 * Init DAC pin connect
	 * AOUT on P0.26
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	return;
}
/**
 * Configures Timer1 with a prescaler of 99 and a match value of 500000.
 * This means the timer will interrupt every 0.5 seconds.
 * Enables reset and interrupt on match value.
 * Clears interrupt flags and enables Timer1 interrupt.
 * CCLK = 100MHz and PCLK = 100MHz
 * PR = 999 ====> f_res = 100KHz ===> T_res = 1x10^-5 s
 * 1x10^-5 s ------- 1c
 * 30 s ------------ xc = 3000000 counts
 */
void timer_config(void){
    LPC_SC->PCLKSEL0 |= (1<<2); //PCLK = CCLK
    LPC_TIM0->PR = 999; //Prescaler = 99
    LPC_TIM0->MR1 = 30000; //30 Seconds
    LPC_TIM0->MCR |= (1<<4); //Reset on MR1
    LPC_TIM0->EMR |= (3<<6); //Toggle on MR1
    LPC_TIM0->TCR |= 0x03; //Reseteo y habilito el timer0
    LPC_TIM0->TCR &=~ 0x02; //Deshabilito el reset del timer0
}


/**
 * Configures the DMA controller to transfer data from SRAM0 to the DACR register of the LPC_DAC peripheral.
 * Uses a linked list item (LLI) to define the transfer parameters.
 * Initializes and sets up the GPDMA channel 0 with the defined parameters.
 */
void dma_config_buzzer(void){
    GPDMA_LLI_Type LLI;
    LLI.SrcAddr = (uint32_t) &(dac_pwm);
    LLI.DstAddr = (uint32_t) & (LPC_DAC->DACR);
    LLI.NextLLI = (uint32_t) &LLI;
    LLI.Control = SAMPLES      //transfer size 100 registers
				|(2<<18)    //source width 32 bits
				|(2<<21)    //dest width 32 bits
                |(1<<26);   //source increment
    GPDMA_Init();
    GPDMA_Channel_CFG_Type GPDMACfg1;
    GPDMACfg1.ChannelNum = 7;
	GPDMACfg1.SrcMemAddr = (uint32_t) &(dac_pwm);
	GPDMACfg1.DstMemAddr = 0;
	GPDMACfg1.TransferSize = SAMPLES;
	GPDMACfg1.TransferWidth = 0;
	GPDMACfg1.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg1.SrcConn = 0;
	GPDMACfg1.DstConn = GPDMA_CONN_DAC;
	GPDMACfg1.DMALLI = (uint32_t)&LLI;
	GPDMA_Setup(&GPDMACfg1);
	GPDMA_ChannelCmd(7, ENABLE);
    return;
}

void dma_config_sg90(void){
    GPDMA_LLI_Type LLI;
    LLI.SrcAddr = (uint32_t) &(dac_pwm_sg90);
    LLI.DstAddr = (uint32_t) & (LPC_DAC->DACR);
    LLI.NextLLI = (uint32_t) &LLI;
    LLI.Control = SAMPLES*5      //transfer size 500 registers
				|(2<<18)    //source width 32 bits
				|(2<<21)    //dest width 32 bits
                |(1<<26);   //source increment
    GPDMA_Init();
    GPDMA_Channel_CFG_Type GPDMACfg1;
    GPDMACfg1.ChannelNum = 1;
	GPDMACfg1.SrcMemAddr = (uint32_t) &(dac_pwm_sg90);
	GPDMACfg1.DstMemAddr = 0;
	GPDMACfg1.TransferSize = SAMPLES*5;
	GPDMACfg1.TransferWidth = 0;
	GPDMACfg1.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg1.SrcConn = 0;
	GPDMACfg1.DstConn = GPDMA_CONN_DAC;
	GPDMACfg1.DMALLI = (uint32_t)&LLI;
	GPDMA_Setup(&GPDMACfg1);
	GPDMA_ChannelCmd(1, ENABLE);
    return;
}

/**
 * Configures UART2 to enable communication through TXD3 and RXD3 pins.
 */
void uart_config(){
    LPC_PINCON->PINSEL0 |= 0x02; //TXD3
    LPC_PINCON->PINSEL0 |= (0x02<<2); //RXD3
    UART_CFG_Type UARTConfigStruct;
    UART_FIFO_CFG_Type UARTFIFOConfigStruct;
    UART_ConfigStructInit(&UARTConfigStruct);

    UART_Init(LPC_UART3, &UARTConfigStruct);

    UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
    UART_FIFOConfig(LPC_UART3, &UARTFIFOConfigStruct);
	UART_TxCmd(LPC_UART3, ENABLE);
	return;
}


/**
 * Interrupt handler for External Interrupt 0 (EINT0)
 * 
 * This function toggles the power state of the ADC module on every interrupt trigger.
 */
void EINT0_IRQHandler(void){
	static uint16_t mode = 0;
    if(mode % 2 == 0){
        ADC_PowerdownCmd(LPC_ADC, DISABLE);
    }
    else{
        ADC_PowerdownCmd(LPC_ADC, ENABLE);
    }
    mode++;
    LPC_SC->EXTINT = 1;
    return;
}

//
void ADC_IRQHandler(void){
	if(LPC_ADC->ADDR1 & (1<<31)){
		uint32_t water_level = (LPC_ADC->ADDR1>>6) & 0x3FF;
		static int i = 0;
		if(water_level < 300){
			uint8_t string[] = "refill\n\r";
			UART_Send(LPC_UART3, string, sizeof(string), BLOCKING);
//			UART_Send(LPC_UART3, string, sizeof(string), BLOCKING);
//			UART_Send(LPC_UART3, string, sizeof(string), BLOCKING);

			//dac_config_buzzer();
			dac_config_buzzer();
			dma_config_buzzer();
				//GPDMA_ChannelCmd(0, ENABLE);
		}
		else {
			GPDMA_ChannelCmd(7,DISABLE);
		}
	//LPC_ADC->ADINTEN |= 1;
	}
	return;
}


int main(void){
	pwm_generator(50);
	uart_config();
	adc_config();
	timer_config();


	//GPDMA_ChannelCmd(1, DISABLE);
	//eint_config();
	while (1){
//			for(int i = 0; i < 100000000; i++){
//				int i =0;
//			}
		}
    return 0;
}
