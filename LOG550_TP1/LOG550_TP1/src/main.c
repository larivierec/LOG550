/**
*/

/*
	Comments: 
	interrupt adc
	interruip pb0
	interrupt reception et fin de seriel -> reception fait

	timer 1 ou 2 -> seulement un timer

*/

#include <asf.h>
#include "compiler.h"

#define TC_CHANNEL_1                0
#define TC_CHANNEL_2				1
#define TIMER_COUNTER               (&AVR32_TC)
#define TIMER_COUNTER_1_IRQ		    AVR32_TC_IRQ0
#define TIMER_COUNTER_2_IRQ			AVR32_TC_IRQ1
#define P_HIGH						AVR32_INTC_INT3
#define P_MED						AVR32_INTC_INT2
#define P_LOW						AVR32_INTC_INT1
#define P_LOWEST					AVR32_INTC_INT0
#define FPBA                        FOSC0
#define FALSE						0

/************************************************************************/
/*  Le numero du PIN correspondant au LED sur le UC3A					*/
/*	voir uc3a0512.h														*/
/************************************************************************/

#define LED0_LOCAL					59
#define LED1_LOCAL					60
#define LED2_LOCAL					61
#define LED3_LOCAL					62
#define LED4_LOCAL					51
#define LED5_LOCAL					52
#define LED6_LOCAL					53
#define LED7_LOCAL					54

/************************************************************************/
/* Le numero du pin du push button                                      */
/************************************************************************/

#define PUSH_BUTTON_0				88

/************************************************************************/
/* Usable boolean bits				                                    */
/************************************************************************/

#define PUSHED_PB_0                 1 << 0

/************************************************************************/
/* ADC Light Channel configurations     UC3A0512.h                      */
/************************************************************************/

#define LIGHT_CHANNEL				2

// Fonctions preprocesseur
#define TOGGLE_LED(ledNumber)		(LED_Toggle(ledNumber))

// Variables globales
// Configuration du peripherique TC
static const tc_waveform_opt_t WAVEFORM_OPT_1 =
{
	.channel  = TC_CHANNEL_1,

	.bswtrg   = TC_EVT_EFFECT_NOOP,
	.beevt    = TC_EVT_EFFECT_NOOP,
	.bcpc     = TC_EVT_EFFECT_NOOP,
	.bcpb     = TC_EVT_EFFECT_NOOP,

	.aswtrg   = TC_EVT_EFFECT_NOOP,
	.aeevt    = TC_EVT_EFFECT_NOOP,
	.acpc     = TC_EVT_EFFECT_NOOP,
	.acpa     = TC_EVT_EFFECT_NOOP,
	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
	.enetrg   = FALSE,
	.eevt     = 0,
	.eevtedg  = TC_SEL_NO_EDGE,
	.cpcdis   = FALSE,
	.cpcstop  = FALSE,
	.burst    = FALSE,
	.clki     = FALSE,
	.tcclks   = TC_CLOCK_SOURCE_TC4
};

static const tc_waveform_opt_t WAVEFORM_OPT_2 =
{
	.channel  = TC_CHANNEL_2,

	.bswtrg   = TC_EVT_EFFECT_NOOP,
	.beevt    = TC_EVT_EFFECT_NOOP,
	.bcpc     = TC_EVT_EFFECT_NOOP,
	.bcpb     = TC_EVT_EFFECT_NOOP,

	.aswtrg   = TC_EVT_EFFECT_NOOP,
	.aeevt    = TC_EVT_EFFECT_NOOP,
	.acpc     = TC_EVT_EFFECT_NOOP,
	.acpa     = TC_EVT_EFFECT_NOOP,
	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
	.enetrg   = FALSE,
	.eevt     = 0,
	.eevtedg  = TC_SEL_NO_EDGE,
	.cpcdis   = FALSE,
	.cpcstop  = FALSE,
	.burst    = FALSE,
	.clki     = FALSE,
	.tcclks   = TC_CLOCK_SOURCE_TC4
};

static const tc_interrupt_t TC_INTERRUPT_0 =
{
	.etrgs = 0,
	.ldrbs = 0,
	.ldras = 0,
	.cpcs  = 1,
	.cpbs  = 0,
	.cpas  = 0,
	.lovrs = 0,
	.covfs = 0
};

static const gpio_map_t USART_GPIO_MAP =
{
	{AVR32_USART1_RXD_0_0_PIN, AVR32_USART1_RXD_0_0_FUNCTION},
	{AVR32_USART1_TXD_0_0_PIN, AVR32_USART1_TXD_0_0_FUNCTION}
};

//Flags

U8 dataAcquisitionStarted = 0;
U8 sensorValueReady = 0;
U8 potValueReady = 0;
U8 startAdcConversion = 0;
U8 lightSensorSent = 0;

//Variables

volatile U8 programRunning;
U32 incomingSerialValue;
U8 compteur;
U8 pushButtonPushed = 0;

volatile U32 lightSensorValue = 0;
volatile U32 potValue = 0;	

volatile avr32_tc_t *tc0 = TIMER_COUNTER;

volatile U8 booleanValues = 8;
volatile int current =1;

__attribute__((__interrupt__))
static void irq_led(void)
{
	// La lecture du registre SR efface le fanion de l'interruption.
	tc_read_sr(TIMER_COUNTER, TC_CHANNEL_1);
	programRunning = 1;
}

__attribute__((__interrupt__))
static void irq_serial_communication(void)
{	
	//Receiving
	if (AVR32_USART1.csr & (AVR32_USART_CSR_RXRDY_MASK))
	{
		incomingSerialValue = (AVR32_USART1.rhr & AVR32_USART_RHR_RXCHR_MASK);
	}
	else
	{
		//Transmitting
		if(potValueReady)
		{
			AVR32_USART1.thr = potValue & AVR32_USART_THR_TXCHR_MASK;
			potValueReady = 0;
		}
		else if(sensorValueReady)
		{
			
			AVR32_USART1.thr = 	lightSensorValue & AVR32_USART_THR_TXCHR_MASK;
			sensorValueReady = 0;
			lightSensorSent = 1;
		}
		else
		{
			AVR32_USART1.idr = AVR32_USART_IDR_TXRDY_MASK;
		}
	}
}

__attribute__((__interrupt__))
static void irq_push_button(void)
{
	if(booleanValues | PUSHED_PB_0)
	{
		//booleanValues |= PUSHED_PB_0;
		booleanValues = PUSHED_PB_0;
		TOGGLE_LED(LED3);
	}
	gpio_clear_pin_interrupt_flag(PUSH_BUTTON_0);
	//TOGGLE_LED(LED5);
}

__attribute__((__interrupt__))
static void irq_adc_channel(void)
{
	U32 statusRegister = AVR32_ADC.sr;
	//light sensor conversion done
	if(statusRegister & AVR32_ADC_IER_EOC2_MASK)
	{
		lightSensorValue = AVR32_ADC.cdr2;
		sensorValueReady = 1;
	}
	//potentiometer conversion done
	else if(statusRegister & AVR32_ADC_IER_EOC1_MASK)
	{
		potValue = AVR32_ADC.cdr1;
		potValueReady = 1;
	}
	startAdcConversion = 0;
}

__attribute__((__interrupt__))
static void irq_adc_timer(void)
{
	tc_read_sr(TIMER_COUNTER, TC_CHANNEL_2);
	if(startAdcConversion == 0)
	{
		startAdcConversion = 1;
	}
}

//need more reading
void adc_init(void)
{
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
	adc_configure(&AVR32_ADC);
	adc_enable(&AVR32_ADC, ADC_LIGHT_CHANNEL);
	adc_enable(&AVR32_ADC, ADC_POTENTIOMETER_CHANNEL);
	
	INTC_register_interrupt(&irq_adc_channel,AVR32_ADC_IRQ,P_HIGH);
	AVR32_ADC.ier = AVR32_ADC_IER_EOC2_MASK | AVR32_ADC_IER_EOC1_MASK;
}

void timercounter_init(void)
{
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
	INTC_register_interrupt(&irq_adc_timer,TIMER_COUNTER_2_IRQ,P_HIGH);
	INTC_register_interrupt(&irq_led, TIMER_COUNTER_1_IRQ, P_LOWEST);
	
	current = 2;
	
	tc_init_waveform(tc0, &WAVEFORM_OPT_1);
	tc_write_rc(tc0, TC_CHANNEL_1, (FPBA / 32) / 2000);
	tc_configure_interrupts(tc0, TC_CHANNEL_1, &TC_INTERRUPT_0);
	tc_start(tc0, TC_CHANNEL_1);
	
	tc_init_waveform(tc0, &WAVEFORM_OPT_2);
	tc_write_rc(tc0, TC_CHANNEL_2, (FPBA / 32) / 2000);
	tc_configure_interrupts(tc0, TC_CHANNEL_2, &TC_INTERRUPT_0);
	tc_start(tc0, TC_CHANNEL_2);
	TOGGLE_LED(LED1);
	TOGGLE_LED(LED2);	
}

void usart_init(void)
{
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
	gpio_enable_module(USART_GPIO_MAP,sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	init_dbg_rs232(FOSC0);
	
	INTC_register_interrupt(&irq_serial_communication, AVR32_USART1_IRQ, P_LOWEST);
	
	AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
}

void push_button_init(void)
{
	INTC_register_interrupt(&irq_push_button,(AVR32_GPIO_IRQ_0+PUSH_BUTTON_0/8),AVR32_INTC_INT0);
	gpio_enable_gpio_pin(PUSH_BUTTON_0);
	gpio_enable_pin_interrupt(PUSH_BUTTON_0,GPIO_FALLING_EDGE);
}

void intialization(void)
{
	Disable_global_interrupt();
	
	INTC_init_interrupts();	
	
	adc_init();
	push_button_init();
	usart_init();
	timercounter_init();
	
	Enable_global_interrupt();	
	
	
	print_dbg("Taper S ou X pour demarrer l'acquisition de donnees: \n");
	
	programRunning = 0;
}

int main (void)
{
	intialization();
	
	while(1) // Boucle inifinie a vide !
	{
		if(startAdcConversion == 1 && dataAcquisitionStarted == 1)
		{
			AVR32_ADC.cr = AVR32_ADC_START_MASK;
		}
		
		if(programRunning == 1)
		{
			TOGGLE_LED(LED0);
			programRunning=0;
		}
		//Les valeurs du UART
		
		if(incomingSerialValue == 'x' || incomingSerialValue == 'X')
		{
			TOGGLE_LED(LED3);
			dataAcquisitionStarted = 0;
			incomingSerialValue = 'a';
		}
		
		if(incomingSerialValue == 's' || incomingSerialValue == 'S')
		{
			TOGGLE_LED(LED4);
			dataAcquisitionStarted = 1;
			incomingSerialValue = 'a';
		}
		
		if(sensorValueReady || (potValueReady && lightSensorSent))
		{
			AVR32_USART1.ier = AVR32_USART_IER_TXRDY_MASK;
		}
		
		//If we pressed the PB0, we change the hertz
		if(booleanValues == (booleanValues | PUSHED_PB_0))
		{
			booleanValues &= ~0x01;
			switch(current)
			{
				case 1:
					tc_write_rc(tc0, TC_CHANNEL_1, (FPBA / 32) / 2000);
					tc_write_rc(tc0, TC_CHANNEL_2, (FPBA / 32) / 2000);
					current = 2;
					break;
				case 2:
					tc_write_rc(tc0, TC_CHANNEL_1, (FPBA / 32) / 1000);
					tc_write_rc(tc0, TC_CHANNEL_2, (FPBA / 32) / 1000);
					current = 1;
					break;
			}
		}
	}
}
