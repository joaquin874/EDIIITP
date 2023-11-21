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

uint8_t error[] = "Error\n\r";
uint8_t msg[] = "Recibido, cambiando nivel\n\r";


uint16_t threshold = 30;

uint32_t umbral = 30;
uint8_t info[1] = "";
uint32_t dac_pwm[SAMPLES];
uint32_t dac_pwm_sg90[SAMPLES*5];

/**
 * Config Pins function
 * Configure all pins (ADC, DAC, UART2)
 */
void pin_config(void){
	PINSEL_CFG_Type PinCfg;
	// Config Tx UART2
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	// Config Rx UART2
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);
	// Config AD0.0
	PinCfg.Funcnum = 1;
	PinCfg.Pinmode = 2;
	PinCfg.Pinnum = 23;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	// Config DAC
	PinCfg.Funcnum = 2;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 26;
	PINSEL_ConfigPin(&PinCfg);
}

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
 * This function initializes the ADC module with a frequency of 200kHz, disables burst mode,
 * powers down the ADC, enables channel 0, starts conversion on 8 channels and configures the
 * start edge as falling edge.
 */
void adc_config(void){
	ADC_Init(LPC_ADC, 100000);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_ChannelCmd(LPC_ADC, 1, ENABLE);
    ADC_StartCmd(LPC_ADC, 4);
    ADC_EdgeStartConfig(LPC_ADC, 0);
    ADC_IntConfig(LPC_ADC, 1, ENABLE);
    NVIC_EnableIRQ(ADC_IRQn);
    return;
}


 /* Configures the DAC (Digital-to-Analog Converter) to output analog signals through pin P0.26.
 * This function initializes the DAC, sets the clock for the DAC, and enables the DAC counter and DMA.
 * f_Signal = 1000Hz ===> T_Signal = 1x10^-3 seg
 * T_Sample = 100Mhz
 * 1 c ---- 1x10^-8seg
 * x c ---- 10x10^-6seg
 * x = 1000
 */
void dac_config(void){
	LPC_SC->PCLKSEL0 |= (1<<22);
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_Init(LPC_DAC);
	DAC_SetDMATimeOut(LPC_DAC,1000);
	DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
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
    LPC_TIM0->TCR |= 0x03; // Enable and Reset TC
    LPC_TIM0->TCR &=~ 0x02; // Disable Reset TC
}

/**
 * Configures the DMA controller to transfer data from SRAM0 to the DACR register of the LPC_DAC peripheral.
 * Uses a linked list item (LLI) to define the transfer parameters.
 * Initializes and sets up the GPDMA channel 0 with the defined parameters.
 */
void dma_config(void){
    GPDMA_LLI_Type LLI;
    LLI.SrcAddr = (uint32_t) &(dac_pwm);
    LLI.DstAddr = (uint32_t) & (LPC_DAC->DACR);
    LLI.NextLLI = (uint32_t) &LLI;
    LLI.Control = SAMPLES      // Transfer size 100 registers
				|(2<<18)    // Source width 32 bits
				|(2<<21)    // Dest width 32 bits
                |(1<<26);   // Source increment
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

/**
 * Configures UART2 to enable communication through TXD3 and RXD3 pins.
 */
void uart_config(){
	UART_CFG_Type      UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
	// Default config
	UART_ConfigStructInit(&UARTConfigStruct);
	// UART2 init
	UART_Init(LPC_UART2, &UARTConfigStruct);
	// FIFO init
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART2, &UARTFIFOConfigStruct);
	UART_TxCmd(LPC_UART2, ENABLE);
	// Enable interruption UART RX
	UART_IntConfig(LPC_UART2, UART_INTCFG_RBR, ENABLE);
	// Enable interruption by UART state line
	UART_IntConfig(LPC_UART2, UART_INTCFG_RLS, ENABLE);
	// Enable UART2 interruption
	NVIC_EnableIRQ(UART2_IRQn);
	return;
}


void ADC_IRQHandler(void){
    if(LPC_ADC->ADDR1 & (1<<31)){
        uint32_t water_level = (LPC_ADC->ADDR1>>6) & 0x3FF;
        water_level = 1024-water_level;
        static int i = 0;
        uint8_t nivel = water_level/100;
        if(water_level < umbral){
            uint8_t string[] = {"Nivel de agua bajo\n\r"};
            UART_Send(LPC_UART2, string, sizeof(string), BLOCKING);
            if(i == 0){
                dac_config();
                dma_config();
            }
			GPDMA_ChannelCmd(7,ENABLE);
        }
		else {
			GPDMA_ChannelCmd(7,DISABLE);
		}
    }
    return;
}

void UART2_IRQHandler(void){
	uint32_t intsrc, tmp, tmp1;
	// Determine the source of interruption
	intsrc = UART_GetIntId(LPC_UART2);
	tmp = intsrc & UART_IIR_INTID_MASK;
	// Evaluate Line Status
	if (tmp == UART_IIR_INTID_RLS){
		tmp1 = UART_GetLineStatus(LPC_UART2);
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE \
				| UART_LSR_BI | UART_LSR_RXFE);
		// Enters an infinite loop if there is an error
		if (tmp1) {
			while(1){};
		}
	}
	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI)){
		UART_Receive(LPC_UART2, info, sizeof(info), NONE_BLOCKING);
		if(info[0] > 47 && info[0] < 53){
			UART_Send(LPC_UART2, "El umbral de deteccion es: ", sizeof("El umbral de deteccion es: "), BLOCKING);
			UART_Send(LPC_UART2, info, sizeof(info), BLOCKING);
			UART_Send(LPC_UART2, "/4\n\r", sizeof("/4\n\r"), BLOCKING);

			if(info[0]-48 == 0){
				umbral = 350;
			}
			else if(info[0]-48 == 1){
				umbral = 700;
			}
			else if(info[0]-48 == 2){
				umbral = 775;
			}
			else if(info[0]-48 == 3){
				umbral = 805;
			}
			else if(info[0]-48 == 4){
				umbral = 830;
			}
		}
	}
	return;
}

int main(void){
	pin_config();
	pwm_generator(50);
	uart_config();
	adc_config();
	timer_config();
	while (1){}
    return 0;
}
