#ifdef 
#include “lpc17xx.h”
#endef

//Librerías de CMSIS
#include “lpc17xx_gpio.h”
#include “lpc17xx_timer.h”
#include “lpc17xx_pinsel.h”
#include “lpc17xx_nvic.h”
#include “lpc17xx_exti.h”

//Declaración de variables
uint32_t PRESCALER=499;


//Declaración de funciones
void config_PINSEL(void);
void config_GPIO(void);
void config_EXTINT3(void);
void config_timer0(void);
void config_matchA(void);
void config_matchB(void);

//Interrupciones
void TIM0_Handler(void);
void EINT3_IRQHandler(void);

//Definición de funciones

void config_PINSEL(){
	//Seteo de P0.0 a P0.3 como salida GPIO
    //Definiciones de estructura
    PINSEL_CFG_Type pinsel_cfg;
    //Parámetros 
    pinsel_cfg.Portnum = PINSEL_PORT_0;
	pinsel_cfg.Pinnum = PINSEL_PIN_0;
	pinsel_cfg.Funcnum = PINSEL_FUNC_0;
	pinsel_cfg.Pinmode = PINSEL_PINMODE_PULLUP;
	pinc_cfg.OpenDrain =PINSEL_PINMODE_NORMAL:
	
	//Inicializar struct
	//P0.0
	PINSEL_ConifgPin(&pinsel_cfg); 
	//P0.1
	pinsel_cfg.Pinnum = PINSEL_PIN_1;
	PINSEL_ConfigPin(&pinsel_cfg);
	//P0.2
	pinsel_cfg.Pinnum = PINSEL_PIN_2;
	PINSEL_ConfigPin(&pinsel_cfg);
	//P0.3
	pinsel_cfg.Pinnum = PINSEL_PIN_3;
	PINSEL_ConfigPin(&pinsel_cfg);

    //Seteo para EINT3
    pinsel_cfg.Portnum = PINSEL_PORT_2;
	pinsel_cfg.Pinnum = PINSEL_PIN_13;
	pinsel_cfg.Funcnum = PINSEL_FUNC_1;
	pinsel_cfg.Pinmode = PINSEL_PINMODE_PULLUP;
	pinc_cfg.OpenDrain =PINSEL_PINMODE_NORMAL;
	//Inicializar struct
	PINSEL_ConfigPin(&pinsel_cfg);
}

//Seteo como salida los puertos  pines 
void config_GPIO(){
    GPIO_SetDir(PINSEL_PORT_0, (1<<0), 1);
    GPIO_SetDir(PINSEL_PORT_0, (1<<1), 1);
    GPIO_SetDir(PINSEL_PORT_0, (1<<2), 1);
    GPIO_SetDir(PINSEL_PORT_0, (1<<3), 1);
}

void config_EINT3(){
	//Defincion de estrcutura
	EXTI_InitType eint_cfg;
	//Parámetros
	eint_cfg.EXTI_line = EXTI_EINT3;
	eint_cfg.EXTI_Mode= EXTI_MODE_EDGE_SENSITIVE;
	eint_cfg.EXTI_Priority= EXTI_POLARITY_HIGH_RISING_EDGE;
	//Inicializamos
	EXTI_ClearExtiFlag(EXTI_EINT3);
	EXTI_Config(&eint_cfg);
}

void config_timer0(){
	//Def struct
	TIM_TIMERCFG_TYPE timer_cfg;
	//Parametros
	timer_cfg.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer_cfg.PrescaleValue = PRESCALER;   //para un total de 5 veces 
	//Inicializamos
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE; &timer_cfg);
}

void config_matchA(){
	//Def struct
	TIM_MATCHCFG_Type match_cfg;
	//Parametros
	match_cfg.MatchChannel = 0;
	match_cfg.IntOnMatch = 1;
	match_cfg.StopOnMatch = 0; 
	match_cfg.ResetOnMatch = 0;
	match_cfg.ExtMatchOutPutType = TIM_EXTMATCH_NOTHING;
	match_cfg.MatchValue = 1;
	//Inicializamos
	TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
	//Config Match 1
	match_cfg.MatchChannel = 1;
	match_cfg.MatchValue = 2;
    TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
	//Config Match 2
    match_cfg.MatchChannel = 2;
	match_cfg.MatchValue = 3;
    TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
	//Config Match 3
	match_cfg.MatchChannel = 3;
	match_cfg.MatchValue = 4;
	match_cfg.ResetOnMatch = 1;
    TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
}

void config_matchB(){
	//Def struct
	TIM_MATCHCFG_Type match_cfg;
	//Parametros
	match_cfg.MatchChannel = 0;
	match_cfg.IntOnMatch = 1;
	match_cfg.StopOnMatch = 0; 
	match_cfg.ResetOnMatch = 0;
	match_cfg.ExtMatchOutPutType = TIM_EXTMATCH_NOTHING;
	match_cfg.MatchValue = 1; //Levanto señal 2
	//Inicializamos
	TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
	//Config Match 1
	match_cfg.MatchChannel = 1;
	match_cfg.MatchValue = ;2; //bajo señal 1, levanto señal 3
    TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
	//Config Match 2
    match_cfg.MatchChannel = 2;
	match_cfg.MatchValue = 3; //bajo señal 2, levanto señal 4
    TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
	//Config Match 3
	match_cfg.MatchChannel = 3;
	match_cfg.MatchValue = 5; //bajo señal 3, levanto señal 1
	march_cfg.ResetOnMatch = 1;
    TIM_CONFIGMATCH(LPC_TIM0, &match_cfg);
}


void EINT3_IRQHandler(){
	//Limpiamos el flag
    EXTI_ClearEXTIFlag(EXTI_EINT3);
    //Limpio las posibles señales que estaban funcionando
	GPIO_ClearValue(PINSEL_PORT_0; (1<<0) );
	GPIO_ClearValue(PINSEL_PORT_0; (1<<1) );
	GPIO_ClearValue(PINSEL_PORT_0; (1<<2) );
	GPIO_ClearValue(PINSEL_PORT_0; (1<<3) );

    if(variable = 0){
	    variable = 1;
    }
    else{
	    variable = 0;
    }
}

void TIM0_Handler(){
	//SECUENCIA A
	if (variable = 0){
		if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT){
			GPIO_ClearValue(PINSEL_PORT_0; (1<<0) );
			GPIO_SetValue(PINSEL_PORT_0; (1<<1) );
        }

        else if(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT){
            GPIO_ClearValue(PINSEL_PORT_0; (1<<1) );
			GPIO_SetValue(PINSEL_PORT_0; (1<<2 );
        }

        else if(TIM_GetIntStatus(LPC_TIM0, TIM_MR2_INT){
	        GPIO_ClearValue(PINSEL_PORT_0; (1<<2) );
			GPIO_SetValue(PINSEL_PORT_0; (1<<3) );
        }

        else{
	        GPIO_ClearValue(PINSEL_PORT_0; (1<<3) );
		    GPIO_SetValue(PINSEL_PORT_0; (1<<4) );
        }
    }

    //SECUENCIA B
    else{
		if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT){
			GPIO_SetValue(PINSEL_PORT_0; (1<<2) );
        }

        else if(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT){
            GPIO_ClearValue(PINSEL_PORT_0; (1<<1) );
			GPIO_SetValue(PINSEL_PORT_0; (1<<3 );
        }

        else if(TIM_GetIntStatus(LPC_TIM0, TIM_MR2_INT){
	        GPIO_ClearValue(PINSEL_PORT_0; (1<<2) );
			GPIO_SetValue(PINSEL_PORT_0; (1<<4) );
        }

        else{
	        GPIO_ClearValue(PINSEL_PORT_0; (1<<3) );
			GPIO_SetValue(PINSEL_PORT_0; (1<<1) );
        }
    }
}

void int main(){

	SystemInit();

	config_PINSEL();
    config_GPIO();
    config_EXTINT3();
    config_timer0();

    while(true){
		if (variable = 0){
			GPIO_SetValue(PINSEL_PORT_0; (1<<0) );
			config_matchA();
		}
	    else{
            GPIO_SetValue(PINSEL_PORT_0; (1<<0) );
			config_matchB();
        }
    }
    return 0;
}