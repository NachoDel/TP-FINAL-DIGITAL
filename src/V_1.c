//-------- HEADERS --------//
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_UART.h"


#define OUTPUT 1
#define INPUT 0
//-------- VARIABLES GLOBALES --------//
uint16_t ADC_CLKDIV = 1000; //selector de rate para el adc
uint32_t PRESCALE   = 1000; //prescaler para 1ms
uint32_t BUFFER_DATOS_ADC[100]; //Para solicitar estos datos, deben ser tratados
uint16_t BUFFER_ENVIAR[100]; //Para solicitar estos datos, deben ser tratados
uint32_t MATCH0_VALUE = 1000; //1 segundo
uint32_t MATCH1_VALUE = 1500; //1 segundo y medio
uint16_t temp_avg = 0; //variable usada para calcular el avg de las 100 mediciones de temp NOTA: Guarda a su vez el acumulado usado para hacer el calculo final
uint16_t temp_avg_UART = 0; //variable usada para mandar el valor por uart

//-------- FUNCIONES --------//
void config_PINSEL(void);
void config_DAC(void);
void config_ADC(void);
void config_DMA(void);
void config_DMA_ADC(void);
void config_UART(void);
void config_TIMER0(void);

void TIMER0_IRQHandler(void);

uint16_t extract_ADC_result_from_Buffer(uint8_t);
uint16_t tabla_conversion(uint16_t);
void convertir_datos_ADC(void);
void enviarValorDAC(uint16_t);
void conversion_DAC(uint16_t);
void send_temp_avg(void);
void apagando_las_luces(void);
void encender_led_rojo();
void encender_led_amarillo();
void encender_led_azul();
//-------- DEFINICION DE FUNCIONES --------//
// ----- CONFIGURACION -----//
void config_PINSEL(){
	PINSEL_CFG_Type pin_cfg;

// ____LED AMARILLO____
	pin_cfg.Portnum	=	0;
	pin_cfg.Pinnum	=	4;
	pin_cfg.Funcnum	=	0;
	pin_cfg.Pinmode	=	PINSEL_PINMODE_PULLUP;
	GPIO_SetDir(PINSEL_PORT_0, 1<<4, OUTPUT);
    PINSEL_ConfigPin(&pin_cfg);

// ____LED ROJO____
	pin_cfg.Pinnum	=	10;
	GPIO_SetDir(PINSEL_PORT_0, 1<<10, OUTPUT);
	PINSEL_ConfigPin(&pin_cfg);

// ____LED AZUL____
	pin_cfg.Pinnum	=	11;
	GPIO_SetDir(PINSEL_PORT_0, 1<<11, OUTPUT);
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
	pin_cfg.Pinnum	=	15;
	pin_cfg.Funcnum	=	1;
	PINSEL_ConfigPin(&pin_cfg);
}
void config_DAC(){
	LPC_DAC->DACCTRL = 0x0000; //Limpia toda config que haya en el DAC
    LPC_SC->PCONP |= (1 << 15); // Habilitar el bloque DAC
	DAC_CONVERTER_CFG_Type dac_cfg;
	dac_cfg.DBLBUF_ENA	   =   	  0;
	dac_cfg.CNT_ENA        =      0;
	dac_cfg.DMA_ENA        =      0;
	DAC_Init(LPC_DAC);
	DAC_ConfigDAConverterControl(LPC_DAC, &dac_cfg);
	DAC_SetBias(LPC_DAC,0);

}
void config_ADC(){
    ADC_Init(LPC_ADC, ADC_CLKDIV);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);
}

void config_DMA(){
	LPC_SC->PCONP |= (1 << 29);  // Habilitar GPDMA (bit 29 en PCONP)
	config_DMA_ADC();
}
void config_DMA_ADC(){
	GPDMA_Channel_CFG_Type dma_cfg;
	// ----- ADC ------//
	dma_cfg.ChannelNum          = 0;
	dma_cfg.SrcMemAddr          = 0;
	dma_cfg.DstMemAddr          = (uint32_t)BUFFER_DATOS_ADC;
	dma_cfg.TransferSize        = 100;
	dma_cfg.TransferWidth       = 0;
	dma_cfg.TransferType        = GPDMA_TRANSFERTYPE_P2M;
	dma_cfg.SrcConn             = GPDMA_CONN_ADC;
	dma_cfg.DstConn             = 0;
	dma_cfg.DMALLI       	    = 0;
	GPDMA_Setup(&dma_cfg);

	GPDMA_ChannelCmd(0,ENABLE);
}

void config_UART(){
	LPC_SC->PCONP |= (1 << 4);  // Habilita el clock para UART1
	// Selecciona el reloj del sistema para UART
	LPC_SC->PCLKSEL0 &= ~(3 << 8);  // PCLK_UART1 = CCLK
	LPC_SC->PCLKSEL0 |= (1 << 8);

	UART_CFG_Type UARTConfigStruct;
	UART_ConfigStructInit(&UARTConfigStruct); // Configuración por defecto de UART

	UART_Init(LPC_UART1, &UARTConfigStruct);  // Inicializa UART1 con la configuración
	UART_TxCmd(LPC_UART1, ENABLE);            // Habilita la transmisión en UART1

}
void config_TIMER0(){
	TIM_TIMERCFG_Type timer0_cfg;
	timer0_cfg.PrescaleOption      = TIM_PRESCALE_USVAL;
	timer0_cfg.PrescaleValue       = PRESCALE;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer0_cfg);

	//MATCH 0 para DMA
	TIM_MATCHCFG_Type match0_cfg;
	match0_cfg.MatchChannel        = 0;
	match0_cfg.IntOnMatch          = ENABLE;
	match0_cfg.StopOnMatch         = DISABLE;
	match0_cfg.ResetOnMatch        = DISABLE;
	match0_cfg.ExtMatchOutputType  = TIM_EXTMATCH_NOTHING;
	match0_cfg.MatchValue          = MATCH0_VALUE;
	TIM_ConfigMatch(LPC_TIM0, &match0_cfg);

	//MATCH 1 para UART
	match0_cfg.MatchChannel        = 1;
	match0_cfg.IntOnMatch          = ENABLE;
	match0_cfg.StopOnMatch         = DISABLE;
	match0_cfg.ResetOnMatch        = ENABLE;
	match0_cfg.ExtMatchOutputType  = TIM_EXTMATCH_NOTHING;
	match0_cfg.MatchValue          = MATCH1_VALUE;
	TIM_ConfigMatch(LPC_TIM0, &match0_cfg);
	TIM_Cmd(LPC_TIM0, ENABLE);

	NVIC_EnableIRQ(TIMER0_IRQn);
}
// ----- FUNCIONES DE INTERRUPCION -----//
void TIMER0_IRQHandler(){
	//MATCH1 --> UART
	if(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)){
		TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
		send_temp_avg();
	}
	//MATCH0 --> DMA
	else{
		TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
		config_DMA_ADC();
	}
}

// ----- FUNCIONES VARIAS -----//
// FUNCIONES PARA EL ADC------------------------
//Funcion encargada de extraer los 12 bits con el resultado de la conversion
//desde el buffer, indice "pos"
uint16_t extract_ADC_result_from_Buffer(uint8_t pos){
	// Extract bits 4 to 15 from the ADC value
	uint16_t a_conversion = 0;
	a_conversion = ((BUFFER_DATOS_ADC[pos] >> 4) & 0xFFF); //valor recortado de 12 bits ADC
	return a_conversion;
}

//Funcion de tabla, para traducir conversion LLAMADA pasando dato del ADC
uint16_t tabla_conversion(uint16_t valor){
	uint16_t adcValue = 0;
	adcValue = valor * 10;
    if (adcValue >= 1242 && adcValue < 1365) return 10;   // Rango para 10°C
    if (adcValue >= 1365 && adcValue < 1488) return 11;   // Rango para 11°C
    if (adcValue >= 1488 && adcValue < 1611) return 12;   // Rango para 12°C
    if (adcValue >= 1611 && adcValue < 1735) return 13;   // Rango para 13°C
    if (adcValue >= 1735 && adcValue < 1858) return 14;   // Rango para 14°C
    if (adcValue >= 1858 && adcValue < 1981) return 15;   // Rango para 15°C
    if (adcValue >= 1981 && adcValue < 2104) return 16;   // Rango para 16°C
    if (adcValue >= 2104 && adcValue < 2227) return 17;   // Rango para 17°C
    if (adcValue >= 2227 && adcValue < 2350) return 18;   // Rango para 18°C
    if (adcValue >= 2350 && adcValue < 2473) return 19;   // Rango para 19°C
    if (adcValue >= 2473 && adcValue < 2596) return 20;   // Rango para 20°C
    if (adcValue >= 2596 && adcValue < 2719) return 21;   // Rango para 21°C
    if (adcValue >= 2719 && adcValue < 2842) return 22;   // Rango para 22°C
    if (adcValue >= 2842 && adcValue < 2965) return 23;   // Rango para 23°C
    if (adcValue >= 2965 && adcValue < 3088) return 24;   // Rango para 24°C
    if (adcValue >= 3088 && adcValue < 3211) return 25;   // Rango para 25°C
    if (adcValue >= 3211 && adcValue < 3334) return 26;   // Rango para 26°C
    if (adcValue >= 3334 && adcValue < 3457) return 27;   // Rango para 27°C
    if (adcValue >= 3457 && adcValue < 3580) return 28;   // Rango para 28°C
    if (adcValue >= 3580 && adcValue < 3703) return 29;   // Rango para 29°C
    if (adcValue >= 3703 && adcValue < 3826) return 30;   // Rango para 30°C
    if (adcValue >= 3826 && adcValue < 3949) return 31;   // Rango para 31°C
    if (adcValue >= 3949 && adcValue < 4072) return 32;   // Rango para 32°C
    if (adcValue >= 4072 && adcValue < 4195) return 33;   // Rango para 33°C
    if (adcValue >= 4195 && adcValue < 4318) return 34;   // Rango para 34°C
    if (adcValue >= 4318 && adcValue < 4441) return 35;   // Rango para 35°C
    if (adcValue >= 4441 && adcValue < 4564) return 36;   // Rango para 36°C
    if (adcValue >= 4564 && adcValue < 4687) return 37;   // Rango para 37°C
    if (adcValue >= 4687 && adcValue < 4810) return 38;   // Rango para 38°C
    if (adcValue >= 4810 && adcValue < 4933) return 39;   // Rango para 39°C
    if (adcValue >= 4933 && adcValue < 5056) return 40;   // Rango para 40°C
    if (adcValue >= 5056 && adcValue < 5179) return 41;   // Rango para 41°C
    if (adcValue >= 5179 && adcValue < 5302) return 42;   // Rango para 42°C
    if (adcValue >= 5302 && adcValue < 5425) return 43;   // Rango para 43°C
    if (adcValue >= 5425 && adcValue < 5548) return 44;   // Rango para 44°C
    if (adcValue >= 5548 && adcValue < 5671) return 45;   // Rango para 45°C
    if (adcValue >= 5671 && adcValue < 5794) return 46;   // Rango para 46°C
    if (adcValue >= 5794 && adcValue < 5917) return 47;   // Rango para 47°C
    if (adcValue >= 5917 && adcValue < 6040) return 48;   // Rango para 48°C
    if (adcValue >= 6040 && adcValue < 6163) return 49;   // Rango para 49°C
    if (adcValue >= 6163 && adcValue < 6286) return 50;   // Rango para 50°C
    if (adcValue >= 6286 && adcValue < 6409) return 51;   // Rango para 51°C
    if (adcValue >= 6409 && adcValue < 6532) return 52;   // Rango para 52°C
    if (adcValue >= 6532 && adcValue < 6655) return 53;   // Rango para 53°C
    if (adcValue >= 6655 && adcValue < 6778) return 54;   // Rango para 54°C
    if (adcValue >= 6778 && adcValue < 6901) return 55;   // Rango para 55°C
    if (adcValue >= 6901 && adcValue < 7024) return 56;   // Rango para 56°C
    if (adcValue >= 7024 && adcValue < 7147) return 57;   // Rango para 57°C
    if (adcValue >= 7147 && adcValue < 7270) return 58;   // Rango para 58°C
    if (adcValue >= 7270 && adcValue < 7393) return 59;   // Rango para 59°C
    if (adcValue >= 7393 && adcValue < 7516) return 60;   // Rango para 60°C
    if (adcValue >= 7516 && adcValue < 7639) return 61;   // Rango para 61°C
    if (adcValue >= 7639 && adcValue < 7762) return 62;   // Rango para 62°C
    if (adcValue >= 7762 && adcValue < 7885) return 63;   // Rango para 63°C
    if (adcValue >= 7885 && adcValue < 8008) return 64;   // Rango para 64°C
    if (adcValue >= 8008 && adcValue < 8131) return 65;   // Rango para 65°C
    if (adcValue >= 8131 && adcValue < 8254) return 66;   // Rango para 66°C
    if (adcValue >= 8254 && adcValue < 8377) return 67;   // Rango para 67°C
    if (adcValue >= 8377 && adcValue < 8500) return 68;   // Rango para 68°C
    if (adcValue >= 8500 && adcValue < 8623) return 69;   // Rango para 69°C
    if (adcValue >= 8623 && adcValue < 8746) return 70;   // Rango para 70°C
    if (adcValue >= 8746 && adcValue < 8869) return 71;   // Rango para 71°C
    if (adcValue >= 8869 && adcValue < 8992) return 72;   // Rango para 72°C
    if (adcValue >= 8992 && adcValue < 9115) return 73;   // Rango para 73°C
    if (adcValue >= 9115 && adcValue < 9238) return 74;   // Rango para 74°C
    if (adcValue >= 9238 && adcValue < 9361) return 75;   // Rango para 75°C
    if (adcValue >= 9361 && adcValue < 9484) return 76;   // Rango para 76°C
    if (adcValue >= 9484 && adcValue < 9607) return 77;   // Rango para 77°C
    if (adcValue >= 9607 && adcValue < 9730) return 78;   // Rango para 78°C
    if (adcValue >= 9730 && adcValue < 9853) return 79;   // Rango para 79°C
    if (adcValue >= 9853 && adcValue < 9976) return 80;   // Rango para 80°C
    if (adcValue >= 9976 && adcValue < 10099) return 81;  // Rango para 81°C
    if (adcValue >= 10099 && adcValue < 10222) return 82; // Rango para 82°C
    if (adcValue >= 10222 && adcValue < 10345) return 83; // Rango para 83°C
    if (adcValue >= 10345 && adcValue < 10468) return 84; // Rango para 84°C
    if (adcValue >= 10468 && adcValue < 10591) return 85; // Rango para 85°C
    if (adcValue >= 10591 && adcValue < 10714) return 86; // Rango para 86°C
    if (adcValue >= 10714 && adcValue < 10837) return 87; // Rango para 87°C
    if (adcValue >= 10837 && adcValue < 10960) return 88; // Rango para 88°C
    if (adcValue >= 10960 && adcValue < 11083) return 89; // Rango para 89°C
    if (adcValue >= 11083 && adcValue < 11206) return 90; // Rango para 90°C
    if (adcValue >= 11206 && adcValue < 11329) return 91; // Rango para 91°C
    if (adcValue >= 11329 && adcValue < 11452) return 92; // Rango para 92°C
    if (adcValue >= 11452 && adcValue < 11575) return 93; // Rango para 93°C
    if (adcValue >= 11575 && adcValue < 11698) return 94; // Rango para 94°C
    if (adcValue >= 11698 && adcValue < 11821) return 95; // Rango para 95°C
    if (adcValue >= 11821 && adcValue < 11944) return 96; // Rango para 96°C
    if (adcValue >= 11944 && adcValue < 12067) return 97; // Rango para 97°C
    if (adcValue >= 12067 && adcValue < 12190) return 98; // Rango para 98°C
    if (adcValue >= 12190 && adcValue < 12313) return 99; // Rango para 99°C
    if (adcValue >= 12313 && adcValue < 12435) return 100; // Rango para 100°C
    return 1;  // Valor fuera de rango
}

//Funcion encargada de tomar los datos del buffer_ADC, transofrmarlos en datos de la tabla, y guardarlos en BUFFER_ENVIAR
void convertir_datos_ADC(){
	temp_avg = 0;
	for(uint8_t i = 0; i < 100; i++){
		BUFFER_ENVIAR[i] = tabla_conversion(extract_ADC_result_from_Buffer(i));
		temp_avg = temp_avg + BUFFER_ENVIAR[i];
	}
	temp_avg = temp_avg/100; //SI SE CAMBIAN LA CANTIDAD DE MEDICIONES, CAMBIAR ESTE 100
	temp_avg_UART = temp_avg;
	conversion_DAC(temp_avg); //se manda por el DAC, el valor correspondiente, antes de realizar las siguientes mediciones
}
//-----------------------------------------------

//FUNCIONES PARA DAC --------------------------
//Funcion encargada de enviar un valor por el DAC
void enviarValorDAC(uint16_t valorDAC) {
    if (valorDAC == NULL) return;  // Verifica que el puntero no sea NULL
    // Envia el valor al DAC
    DAC_UpdateValue(LPC_DAC, valorDAC);
}

//Funcion encargada de tomar el average de la temperatura y buscar su valor de salida correspondiente
void conversion_DAC(uint16_t celsius_avg){
	if(celsius_avg >= 70 ){
		//Temperatura alta
		//683 a 1024
		//saca 3,3V
		enviarValorDAC(1023);
		encender_led_rojo();
	}
	else if((celsius_avg >= 50) && (celsius_avg < 70)){
		//Temperatura media
		//342 a 682
		//saca 1,7 V
		enviarValorDAC(605); //era 605 era el valor "minimo"
		encender_led_amarillo();
	}
	else{
		//Temperatura baja
		//0 a 341
		//saca 1 V
		enviarValorDAC(1024); //como dac no cuenta este valor, lo cuenta como 0
		encender_led_azul();
	}
}

//---------------------------------------------

//FUNCIONES PARA UART
void send_temp_avg(){
    // Mensaje inicial
    char msg[] = "Fer, la temperatura es: ";

    // Envía el mensaje inicial
    for (int j = 0; msg[j] != '\0'; j++) {
        while (!(LPC_UART1->LSR & (1 << 5)));  // Espera a que THR esté libre
        LPC_UART1->THR = msg[j];  // Envía cada carácter del mensaje inicial
    }

    // Buffer temporal para almacenar los caracteres de temp_avg
    char buffer[6];
    int i = 0;

    // Convierte temp_avg a texto manualmente y almacena en buffer
    uint16_t value = temp_avg_UART;
    do {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    } while (value > 0 && i < sizeof(buffer) - 1);

    // Invierte el orden de los caracteres en buffer (porque el número se construye al revés)
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    // Envía el valor de temperatura convertido a caracteres
    for (int j = 0; j < i; j++) {
        while (!(LPC_UART1->LSR & (1 << 5)));  // Espera a que THR esté libre
        LPC_UART1->THR = buffer[j];  // Envía cada carácter del valor de temp_avg
    }

    // Envía un salto de línea al final
    while (!(LPC_UART1->LSR & (1 << 5)));  // Espera a que THR esté libre
    LPC_UART1->THR = '\n';
}
//-------------------

//FUNCIONES LED
//funcion que apaga los leds
void apagando_las_luces(){
	 GPIO_ClearValue(PINSEL_PORT_0, 1<<10); //apago P0.1
	 GPIO_ClearValue(PINSEL_PORT_0, 1<<4); //apago P0.1
	 GPIO_ClearValue(PINSEL_PORT_0, 1<<11); //apago P0.1
}
//Funcion que enciende LED
//Logica encargada de encender led
void encender_led_rojo(){
	apagando_las_luces();
	GPIO_SetValue(PINSEL_PORT_0, 1<<10);	  //prendo P0.10
}
//Logica encargada de encender led
void encender_led_amarillo(){
	apagando_las_luces();
	GPIO_SetValue(PINSEL_PORT_0, 1<<4);	  //prendo P0.4
}
//Logica encargada de encender led
void encender_led_azul(){
	apagando_las_luces();
	GPIO_SetValue(PINSEL_PORT_0, 1<<11);	  //prendo P0.11
}
//--------------------------------------

//------------ MAIN ---------------------
int main(void) {
    // Initialize configurations
    config_PINSEL();
    config_ADC();
    config_DAC();
    config_DMA();
    config_TIMER0();
    config_UART();
    // Main loop
    while (1) {
        // Main loop code
    	convertir_datos_ADC();
    }

}