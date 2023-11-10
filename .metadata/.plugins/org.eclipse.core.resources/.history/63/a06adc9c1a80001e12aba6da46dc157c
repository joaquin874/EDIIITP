#include "LPC17xx.h"

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

#define PIN22  ((uint32_t) (1<<22))
#define PORT0  ((uint8_t)  (0))
#define OUTPUT ((uint8_t)  (1))

void delay(uint32_t times);

int main(void)
{
	PINSEL_CFG_Type pinselCfg;
	pinselCfg.Portnum = PINSEL_PORT_0;
	pinselCfg.Pinnum = PINSEL_PIN_22;
	pinselCfg.Funcnum = PINSEL_FUNC_0;
	pinselCfg.Pinmode = PINSEL_PINMODE_NORMAL;

	PINSEL_ConfigPin(&pinselCfg);

	GPIO_SetDir(PORT0, PIN22, OUTPUT);

	while(1)
	{
		GPIO_SetValue(PORT0, PIN22);
		delay(1000);
		GPIO_ClearValue(PORT0, PIN22);
		delay(1000);

	}


    return 0 ;
}

void delay(uint32_t times)
{
	for(int i=0; i<times; i++)
		for(int j=0; j<times; j++);

}
