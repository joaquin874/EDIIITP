/**
 * Programar el microcontrolador LPC1769 en un codigo de lenguaje C para que mediante su ADC digitalice una
 * senial analogica cuyo ancho de banda es de 16Khz. La senial analogica tiene una amplitud de pico maxima positiva de
 * 3.3v. Los datos deben ser guardados utilizando el Hardware GPDMA en la primera mitad de la memoria SRAM
 * ubicada en el bloque AHB SRAM bank 0, de manera tal que permita almacenar todos los datos posibles que esta
 * memoria permita. Los datos debe ser almacenados como un buffer circular conservando siempre las ultimas muestras.
 *
 * Por otro lado se tiene una forma de onda como se muestra en la imagen a continuacion. Esta senial debe ser
 * generada por una funcion y debe ser reproducida por el DAC desde la segunda mitad de AHB SRAM - bank 0 memoria
 * utilizando DMA de tal forma que se logre un periodo de 614uS logrando la maxima resolucion y maximo rango de tension.
 *
 * Durante operacion normal se debe generar por el DAC la forma de onda mencionada como wave_form. Se debe indicar cual es
 * el minimo incremento de tension de salida de esa forma de onda.
 *
 * Cuando se interrumpe una EXINT conectada a un pin, el ADC configurado debe completar el ciclo de conversion que
 * estaba realizando, y ser detenido, a continuacion se comienzan a sacar las muestras del ADC por el DAC utilizando DMA y
 * desde las posiciones de memoria originales.
 *
 * Cuando interrumpe nuevamente en el mismo pin, se vuelve a repetir la senial de DAC generada por la forma de onda
 * wave_form previamente almacenada y se arranca de nuevo la conversion de datos del ADC. Se alterna asi entre los dos estados
 * del sistema con cada interrupcion externa.
 *
 * Suponer una frecuencia de core cclk de 80Mhz.
*/

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_dac.h"

#define SRAM0_0 0x2007C000
#define SRAM0_1 0x2007E000

uint8_t edge = 1; //1 = rising y 2 = falling
//uint32_t typeDMA = 0; //mod 0 ADC-DAC    mod !0 ADC-MEM
GPDMA_LLI_Type LLI1, LLI2, LLI3, LLI4;

void waveFormGenerator(void){
    static uint16_t value = 511;
    uint32_t *address1 = (uint32_t *) SRAM0_0;
    int i = 0;
    while(i < (SRAM0_1-SRAM0_0)){
        *address1 = value;
        if(edge == 1){
            value++;
            if(value >= 1024){
                edge = 2;
            }
        }
        else if(edge == 2){
            value--;
            if(value <= 0){
                edge = 1;
            }
        }
        address1++;
        i++;
    }
    return;
}

void configADC(void){
    LPC_PINCON->PINSEL1 |= (1<<14);
    LPC_PINCON->PINMODE1 |= (2<<14);

    ADC_Init(LPC_ADC, 16000); // 16k ancho de banda
    ADC_PowerdownCmd(LPC_ADC, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
    ADC_StartCmd(LPC_ADC, 0);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    NVIC_EnableIRQ(ADC_IRQn);
    NVIC_SetPriority(ADC_IRQn, 0);
    return;
}


//Pclk = 100Mhz ==> Tclk = 1x10^-8seg
//1c ---- 1x10^-8seg
//xc ---- 6,14^-4seg ==> xc = 61400
//DACR = 16bit = 65536 > xc

void configDAC(void){
    LPC_PINCON->PINSEL1 |= (2<<20);  //Config AOUT
    LPC_PINCON->PINMODE1 |= (2<<20);
    DAC_CONVERTER_CFG_Type dacConfig;
    dacConfig.CNT_ENA = SET;
    dacConfig.DMA_ENA = SET;

    DAC_Init(LPC_DAC);
    DAC_SetDMATimeOut(LPC_DAC, 61400);
    DAC_ConfigDAConverterControl(LPC_DAC, &dacConfig);
    return;
}

//dma for 4 channels
void configDMA(void){
    const volatile uint32_t *addr = &LPC_ADC->ADDR0;
    uint32_t checkAddr = (*addr>>0xF) & 0x3FF;
	LLI1.SrcAddr = (uint32_t) SRAM0_1;
	LLI1.DstAddr = (uint32_t) &LPC_DAC->DACR;
	LLI1.NextLLI = (uint32_t) &LLI2;
	LLI1.Control = 0xFFF
				   | (1<<18) //source width 16 bits
				   | (1<<21) //dest width 16 bits
				   | (1<<26); //source increment

    LLI2.SrcAddr = (uint32_t) SRAM0_1 + 0x1000;
	LLI2.DstAddr = (uint32_t) &LPC_DAC->DACR;
	LLI2.NextLLI = (uint32_t) &LLI1;
	LLI2.Control = 0xFFF
				   | (2<<18) //source width 16 bits
				   | (2<<21) //dest width 16 bits
				   | (1<<26); //source increment

    LLI3.SrcAddr = (uint32_t) checkAddr;
	LLI3.DstAddr = (uint32_t) SRAM0_0;
	LLI3.NextLLI = (uint32_t) &LLI4;
	LLI3.Control = 0xFFF
				   | (2<<18) //source width 16 bits
				   | (2<<21) //dest width 16 bits
				   | (1<<27); //destiny increment

    LLI4.SrcAddr = (uint32_t) checkAddr + 0x1000;
	LLI4.DstAddr = (uint32_t) SRAM0_0;
	LLI4.NextLLI = (uint32_t) &LLI3;
	LLI4.Control = 0xFFF
				   | (2<<18) //source width 16 bits
				   | (2<<21) //dest width 16 bits
				   | (1<<27); //destiny increment
	GPDMA_Init();

    //AHB SRAM0 bank 0
	GPDMA_Channel_CFG_Type GPDMACfg;
	GPDMACfg.ChannelNum = 0;
	GPDMACfg.SrcMemAddr = (uint32_t)SRAM0_1;
	GPDMACfg.DstMemAddr = 0;
	GPDMACfg.TransferSize = 0xFFF;
	GPDMACfg.TransferWidth = 0;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_DAC;
	GPDMACfg.DMALLI = (uint32_t)&LLI3;
	GPDMA_Setup(&GPDMACfg);
    //NVIC_EnableIRQ(DMA_IRQn);
    return;
}

void configEINT0(void){
    LPC_PINCON->PINSEL4 |= (1<<20);
    LPC_PINCON->PINMODE4 |= (3<<20);

    LPC_SC->EXTMODE = 1;
    LPC_SC->EXTPOLAR = 1;
    LPC_SC->EXTINT = 1;
    NVIC_EnableIRQ(EINT0_IRQn);
    return;
}

void EINT0_IRQHandler(void){
    static uint32_t mode = 0;
    if(LPC_SC->EXTINT & 1){
        if(mode % 2 == 0){
            //saca conversion de adc
        }
        else{
            //saca triangular
        }
    }
    LPC_SC->EXTINT = 1;
    return;
}

void DMA_IRQHandler(void){
    static uint32_t typeDMA1 = 0;
    static uint32_t typeDMA2 = 0;
    static uint32_t addrDMA;
    static GPDMA_LLI_Type addrLLI;
    if(GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)){
        if(typeDMA1 % 2 == 0){
            addrDMA = SRAM0_0 + 0x1000;
            addrLLI = LLI2;
        }
        else{
            addrDMA = SRAM0_0;
            addrLLI = LLI1;
        }
        GPDMA_Channel_CFG_Type GPDMACfg;
        GPDMACfg.ChannelNum = 0;
        GPDMACfg.SrcMemAddr = 0;
        GPDMACfg.DstMemAddr = (uint32_t)addrDMA;
        GPDMACfg.TransferSize = 0xFFF;
        GPDMACfg.TransferWidth = 0;
        GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
        GPDMACfg.SrcConn = GPDMA_CONN_ADC;
        GPDMACfg.DstConn = 0;
        GPDMACfg.DMALLI = (uint32_t) &addrLLI;
        GPDMA_Setup(&GPDMACfg);
        GPDMA_ChannelCmd(0, DISABLE);
        GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 0);
        typeDMA1++;
    }
    if(GPDMA_IntGetStatus(GPDMA_STAT_INT, 1)){
        if(typeDMA2 % 2 == 0){
            addrDMA = SRAM0_1 + 0x1000;
            addrLLI = LLI3;
        }
        else{
            addrDMA = SRAM0_1;
            addrLLI = LLI2;
        }
        GPDMA_Channel_CFG_Type GPDMACfg;
        GPDMACfg.ChannelNum = 1;
        GPDMACfg.SrcMemAddr = (uint32_t)addrDMA;
        GPDMACfg.DstMemAddr = 0;
        GPDMACfg.TransferSize = 0xFFF;
        GPDMACfg.TransferWidth = 0;
        GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
        GPDMACfg.SrcConn = 0;
        GPDMACfg.DstConn = GPDMA_CONN_DAC;
        GPDMACfg.DMALLI = (uint32_t) &addrLLI;
        GPDMA_Setup(&GPDMACfg);
        GPDMA_ChannelCmd(0, DISABLE);
        GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 1);
        typeDMA2++;
    }
    return;
}

int main(void){
    waveFormGenerator();
    configEINT0();
    configADC();
    configDMA();
    configDAC();
    while(1){

    }
    return 0;
}
