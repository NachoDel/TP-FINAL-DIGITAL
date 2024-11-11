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
Uint32_t BUFFER_DATOS_ADC[30]; //Para solicitar estos datos, deben ser tratados
Uint16_t BUFFER_ENVIAR[30]; //Para solicitar estos datos, deben ser tratados
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
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
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
	dma_cfg.SrcMemAddr          = (Uint32_t) &BUFFER_ENVIAR;
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

	NVIC_EnableIRQ(DMA_IRQn);
}
void config_UART(){
	UART_CFG_Type uart_cfg;
	uart_cfg.Baud_rate = 9600;
	uart_cfg.Parity    = UART_PARITY_NONE;
	uart_cfg.Databits  = UART_DATABIT_8;
	uart_cfg.Stopbits  = UART_STOPBIT_1;
	UART_Init(LPC_UART0, &uart_cfg);

	//Configuracion FIFO para trabajr con DMA
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
	UARTFIFOConfigStruct.FIFO_DMAMode		=	ENABLE;
	UARTFIFOConfigStruct.FIFO_Level			=	UART_FIFO_TRGLEV0;
	UARTFIFOConfigStruct.FIFO_ResetRxBuf	=	ENABLE;
	UARTFIFOConfigStruct.FIFO_ResetTxBuf	=	ENABLE;
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);				// Inicializa FIFO
	UART_TxCmd(LPC_UART0, ENABLE);									// Habilita transmision

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
void DMA_IRQHandler(){
	//Canal 0 -> termino de mandar los 30 datos del ADC al buffer
	if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC,0)){
		GPDMA_ChannelCmd(0, DISABLE);
		GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC,0);
	
		convertir_datos_ADC(); //Una vez que se termino de cargar el buffer de ADC, se convierten los datos
	
		GPDMA_ChannelCmd(1, ENABLE);
	}
	else if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC,1)){ //si ya termino de mandar todos los datos al UART, apago el canal del DMA
			GPDMA_ChannelCmd(1, DISABLE);
			GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC,1);
	}
	else{
		GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC,2);
	}

}
// ----- FUNCIONES VARIAS -----//	
//Funcion encargada de extraer los 12 bits con el resultado de la conversion
//desde el buffer, indice "pos"
uint16_t extract_ADC_result_from_Buffer(uint8_t pos){
	// Extract bits 4 to 15 from the ADC value
	return (uint16_t)((BUFFER_DATOS_ADC[pos] >> 4) & 0xFFF); //Si tira error, cambiar por ADC_ChannelGetData(&LPC_ADC);
}

//Funcion de tabla, para traducir conversion
uint16_t tabla_conversion(uint16_t valor){ //SE DEBE VER COMO ES LA CONVERSION DEL SENSOR A VOLTAJE
	if(valor < 1000){
		return 0;
	}
	else if(valor < 2000){
		return 1;
	}
	else if(valor < 3000){
		return 2;
	}
	else if(valor < 4000){
		return 3;
	}
	else if(valor < 5000){
		return 4;
	}
	else if(valor < 6000){
		return 5;
	}
	else if(valor < 7000){
		return 6;
	}
	else if(valor < 8000){
		return 7;
	}
	else if(valor < 9000){
		return 8;
	}
	else if(valor < 10000){
		return 9;
	}
	else if(valor < 11000){
		return 10;
	}
	else if(valor < 12000){
		return 11;
	}
	else if(valor < 13000){
		return 12;
	}
	else if(valor < 14000){
		return 13;
	}
	else if(valor < 15000){
		return 14;
	}
	else if(valor < 16000){
		return 15;
	}
	else if(valor < 17000){
		return 16;
	}
	else if(valor < 18000){
		return 17;
	}
	else if(valor < 19000){
		return 18;
	}
	else if(valor < 20000){
		return 19;
	}
	else if(valor < 21000){
		return 20;
	}
	else if(valor < 22000){
		return 21;
	}
	else if(valor < 23000){
		return 22;
	}
	else if(valor < 24000){
		return 23;
	}
	else if(valor < 25000){
		return 24;
	}
	else if(valor < 26000){
		return 25;
	}
	else if(valor < 27000){
		return 26;
	}
	else if(valor < 28000){
		return 27;
	}
	else if(valor < 29000){
		return 28;
	}
	else if(valor < 30000){
		return 29;
	}
	else{
		return 30;
	}
}

//Logica encargada de encender led LLAMAR EN BUCLE MAIN
void encender_led(uint16_t valor){
	if(valor < 15){
		GPIO_ClearValue(PINSEL_PORT_0, 1<<0); //apago P0.1
		GPIO_SetValue(PINSEL_PORT_0, 0<<0);	  //prendo P0.0 
	}
	else{
		GPIO_ClearValue(PINSEL_PORT_0, 0<<0); //apago P0.1
		GPIO_SetValue(PINSEL_PORT_0, 1<<0); //Prendo P0.1                     
	}
}

//Funcion encargada de tomar los datos del buffer_ADC, transofrmarlos en datos de la tabla, y guardarlos en BUFFER_ENVIAR
void convertir_datos_ADC(){
	for(uint8_t i = 0; i < 30; i++){
		BUFFER_ENVIAR[i] = tabla_conversion(extract_ADC_result_from_Buffer(i));
	}
}

//Transmision UART
//CREO QUE ESTO NO HACE FALTA, YA QUE EL DMA SE ENCARGA DE ESTO (enviar datos al UART)
// void transmitir_UART(){
// 	UART_Send(LPC_UART0, BUFFER_ENVIAR, 30, BLOCKING);
// } VER CHAT GPT