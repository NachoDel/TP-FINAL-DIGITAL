//-------- HEADERS --------//
#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_UART.h"

//-------- VARIABLES GLOBALES --------//
Uint16_t ADC_CLKDIV = 1000; //selector de rate para el adc
Uint32_t PRESCALE   =   399; //prescaler para 1ms
Uint16_t BUFFER_DATOS_ADC[30]; //SE  DEBE TRATAR LOS DATOS ANTES DE GUARDARLOS
Uint32_t MATCH0_VALUE = 500; //0,5 segundos

//-------- FUNCIONES --------//
void config_PINSEL(void);
void config_ADC(void);
void config_DAC(void);
void config_DMA(void);
void config_TIMER0(void);
void config_UART(void);


//-------- DEFINICION DE FUNCIONES --------//
// ----- CONFIGURACION -----//
void config_PINSEL(){
	PINSEL_CFG_Type pin_cfg;

// ____LED AZUL____
	pin_cfg.Portnum	=	0;
	pin_cfg.Pinnum	=	0;
	pin_cfg.Funcnum	=	0;
	pin_cfg.Pinmode	=	PULLUP;
	GPIO_SetDir(PINSEL_PORT_0, 1<<0, OUTPUT)
    PINSEL_ConfigPin(&pin_cfg);

// ____LED ROJO____
	pin_cfg.Pinnum	=	0;
	GPIO_SetDir(PINSEL_PORT_0, 1<<1, OUTPUT)
	PINSEL_ConfigPin(&pin_cfg);

// ____ADC____
	pin_cfg.Pinnum	=	23;
	pin_cfg.Funcnum	=	1;
	PINSEL_ConfigPin(&pin_cfg);

// ____DAC____
	pin_cfg.Pinnum	=	26;
	pin_cfg.Funcnum	=	2;
	PINSEL_ConfigPin(&pin_cfg);

// ____UART____
	pin_cfg.Pinnum	=	2;
	pin_cfg.Funcnum	=	1;
	PINSEL_ConfigPin(&pin_cfg);
}
void config_DAC(){
	DAC_CONVERTER_TYPE dac_cfg;
	dac_cfg.DBLUF_ENA	   =   	  0;
	dac_cfg.CNT_ENA        =      1;
	dac_cfg.CNT_ENA        =      1;
	DAC_Init(LPC_DAC);
	DAC_ConfigDACConverterControl(LPC_DAC, &dac_cfg);
	DAC_SetBias(LPC_DAC,DISABLE);

}
void config_ADC(){
    ADC_Init(LPC_ADC, ADC_CLKDIV);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
//	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
    ADC_BurstCmd(LPC_ADC, ENABLE);
	NVIC_EnableIRQ(ADC_IRQn);
}
void config_DMA(){
	GPDMA_Channel_cfg_Type dma_cfg;
	// ----- ADC ------//
	dma_cfg.ChannelNum          = 0;    
	dma_cfg.SrcMemAddr          = 0;
	dma_cfg.DstMemAddr          = (Uint32_t) &BUFFER_DATOS_ADC;
	dma_cfg.TransferSize        = 16;
	dma_cfg.TransferWidth       = 0;
	dma_cfg.TransferType        = GPDMA_TRANSFERTYPE_P2M;
	dma_cfg.SrcConn             =GPDM_CONN_ADC;
	dma_cfg.DstConn             = 0;
	dma_cfg.DMALLI       	    = 0;
	GPDMA_Setup(&dma_cfg)
	// ----- UART ------//
	dma_cfg.ChannelNum          = 1;    
	dma_cfg.SrcMemAddr          = (Uint32_t) &BUFFER_DATOS_ADC;
	dma_cfg.DstMemAddr          = 0;
	dma_cfg.TransferSize        = 16;
	dma_cfg.TransferWidth       = 0;
	dma_cfg.TransferType        = GPDMA_TRANSFERTYPE_M2P;
	dma_cfg.SrcConn             = 0;
	dma_cfg.DstConn             = GPDMA_CONN_UART0_Tx;
	dma_cfg.DMALLI       	    = 0;
	GPDMA_Setup(&dma_cfg)
	// ----- DAC ------//
	dma_cfg.ChannelNum          = 2;    
	dma_cfg.SrcMemAddr          = BUFFER_DATOS_ADC;
	dma_cfg.DstMemAddr          = 0;
	dma_cfg.TransferSize        = 16;
	dma_cfg.TransferWidth       = 0;
	dma_cfg.TransferType        = GPDMA_TRANSFERTYPE_M2P;
	dma_cfg.SrcConn             = 0;
	dma_cfg.DstConn             = GPDMA_CONN_DAC;
	dma_cfg.DMALLI       	    = 0;
	GPDMA_Setup(&dma_cfg)

}
void config_UART(){
	UART_CFG_Type uart_cfg;
	uart_cfg.Baud_rate = 9600;
	uart_cfg.Parity    = UART_PARITY_NONE;
	uart_cfg.Databits  = UART_DATABIT_8;
	uart_cfg.Stopbits  = UART_STOPBIT_1;
	UART_Init(LPC_UART0, &uart_cfg);

}
void config_TIMER0(){
	TIM_TIMERCFG_Type timer0_cfg;
	timer0_cfg.PrescaleOption      = TIM_PRESCALE_TICKVAL;
	timer0_cfg.PrescaleValue       = PRESCALE;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer0_cfg);
	

	TIM_MATCHCFG_Type match0_cfg;
	match0_cfg.MatchChannel        = 0;
	match0_cfg.IntOnMatch          = ENABLE;
	match0_cfg.StopOnMatch         = DISABLE;
	match0_cfg.ResetOnMatch        = ENABLE;
	match0_cfg.ExtMatchOutputType  = TIM_EXTMATCH_NOTHING;
	match0_cfg.MatchValue          = MATCH0_VALUE;
	TIM_ConfigMatch(LPC_TIM0, &match0_cfg);
	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}
// ----- FUNCIONES DE INTERRUPCION -----//
void TIMER0_IRQHandler(){
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	// ----- ADC -----//
	ADC_StartCmd(LPC_ADC, ADC_START_NOW);
	// ----- DMA -----//
	GPDMA_ChannelCmd(0, ENABLE);
}
void ADC_IRQHandler(){
	LPC_ADC->INTEN = 0;
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, DISABLE);
}

	
























