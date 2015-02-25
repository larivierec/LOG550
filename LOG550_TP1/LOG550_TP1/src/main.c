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
#define FPBA                        (FOSC0 >> 4)
#define FALSE						0

#define FOURHERTZ_TC5				FPBA / 40 //Every second, 4 clicks
#define TWOKHERTZ_TC4				FPBA / 4000
#define ONEKHERTZ_TC4				FPBA / 2000
#define FOURKHERTZ_TC4				FPBA / 9000



/************************************************************************/
/* Le numero du pin du push button                                      */
/************************************************************************/

#define PUSH_BUTTON_0				88

/************************************************************************/
/* Usable boolean bits				                                    */
/************************************************************************/

#define PUSHED_PB_0                 1 << 0
#define DEPASSEMENT_UART			1 << 1
#define DEPASSEMENT_ADC				1 << 2
#define INDICATION_LED				1 << 3
#define SENSOR_VAL_RDY				1 << 4
#define POTENTIOMETER_VAL_RDY		1 << 5
#define START_ADC_CONVERSION		1 << 6
#define DATA_ACQUISITION_RDY		1 << 7
#define ALREADY_ON_FLAG				1 << 8

#define CLEAR_PUSH_BUTTON			(booleanValues &= ~0x01)
#define CLEAR_DEPASSEMENT_UART_FLAG	(booleanValues &= ~0x02)
#define CLEAR_DEPASSEMENT_ADC_FLAG	(booleanValues &= ~0x04)
#define CLEAR_LED_FLAG				(booleanValues &= ~0x08)
#define CLEAR_SENSOR_FLAG			(booleanValues &= ~0x10)
#define CLEAR_POTENTIOMETER_FLAG	(booleanValues &= ~0x20)
#define CLEAR_ADC_CONVERSION_FLAG	(booleanValues &= ~0x40)
#define CLEAR_DATA_ACQUISITION_FLAG	(booleanValues &= ~0x80)
#define CLEAR_ALREADY_ON			(booleanValues &= ~0x100)

#define IS_PUSHED					(booleanValues == (booleanValues | PUSHED_PB_0))
#define IS_DEPASSEMENT_UART			(booleanValues == (booleanValues | DEPASSEMENT_UART))
#define IS_DEPASSEMENT_ADC			(booleanValues == (booleanValues | DEPASSEMENT_ADC))
#define IS_LED						(booleanValues == (booleanValues | INDICATION_LED))
#define IS_SENSOR_VAL_RDY			(booleanValues == (booleanValues | SENSOR_VAL_RDY))
#define IS_POTENTIOMETER_VAL_RDY	(booleanValues == (booleanValues | POTENTIOMETER_VAL_RDY))
#define IS_ADC_STARTED				(booleanValues == (booleanValues | START_ADC_CONVERSION))
#define IS_DATA_ACQUISITION_RDY		(booleanValues == (booleanValues | DATA_ACQUISITION_RDY))
#define IS_ALREADY_ON				(booleanValues == (booleanValues | ALREADY_ON_FLAG))

/************************************************************************/
/* ADC Light Channel configurations     UC3A0512.h                      */
/************************************************************************/

#define LIGHT_CHANNEL				2

// Fonctions preprocesseur
#define TOGGLE_LED(ledNumber)			(LED_Toggle(ledNumber))
#define GET_GPIOPORT_NUMBER(GPIONUMBER)	(GPIONUMBER / 32)

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
	.tcclks   = TC_CLOCK_SOURCE_TC5
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

U8 lightSensorSent = 0;

//Variables

U32 incomingSerialValue;
U8 compteur;

volatile U32 lightSensorValue = 0;
volatile U32 potValue = 0;	

volatile avr32_tc_t *tc0 = TIMER_COUNTER;

volatile U16 booleanValues = 0;
volatile int current =1;

__attribute__((__interrupt__))
static void irq_led(void)
{
	// La lecture du registre SR efface le fanion de l'interruption.
	tc_read_sr(TIMER_COUNTER, TC_CHANNEL_1);
	booleanValues |= INDICATION_LED;
}

__attribute__((__interrupt__))
static void irq_serial_communication(void)
{	
	
	if(AVR32_USART1.csr & (AVR32_USART_CSR_TXRDY_MASK))
	{
		booleanValues |= DEPASSEMENT_UART;
	}
	//Receiving
	if (AVR32_USART1.csr & (AVR32_USART_CSR_RXRDY_MASK))
	{
		incomingSerialValue = (AVR32_USART1.rhr & AVR32_USART_RHR_RXCHR_MASK);
	}
	else
	{
		//Transmitting
		if(IS_POTENTIOMETER_VAL_RDY)
		{
			AVR32_USART1.thr = potValue & AVR32_USART_THR_TXCHR_MASK;
			CLEAR_POTENTIOMETER_FLAG;
		}
		else if(IS_SENSOR_VAL_RDY)
		{
			
			AVR32_USART1.thr = 	lightSensorValue & AVR32_USART_THR_TXCHR_MASK;
			CLEAR_SENSOR_FLAG;
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
		booleanValues |= PUSHED_PB_0;
	}
	gpio_clear_pin_interrupt_flag(PUSH_BUTTON_0);
}

__attribute__((__interrupt__))
static void irq_adc_channel(void)
{
	// Check depassement du UART par le ADC
	if(IS_SENSOR_VAL_RDY && IS_POTENTIOMETER_VAL_RDY)
	{
		booleanValues |= DEPASSEMENT_ADC;
	}
	
	U32 statusRegister = AVR32_ADC.sr;
	//light sensor conversion done
	if(statusRegister & AVR32_ADC_IER_EOC2_MASK)
	{
		lightSensorValue = AVR32_ADC.cdr2;
		booleanValues |= SENSOR_VAL_RDY;
	}
	//potentiometer conversion done
	else if(statusRegister & AVR32_ADC_IER_EOC1_MASK)
	{
		potValue = AVR32_ADC.cdr1;
		booleanValues |= POTENTIOMETER_VAL_RDY;
	}
	CLEAR_ADC_CONVERSION_FLAG;
}

__attribute__((__interrupt__))
static void irq_adc_timer(void)
{
	tc_read_sr(TIMER_COUNTER, TC_CHANNEL_2);
	if(!IS_ADC_STARTED)
	{
		booleanValues |= START_ADC_CONVERSION;
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
	INTC_register_interrupt(&irq_adc_timer,TIMER_COUNTER_2_IRQ,P_LOWEST);
	INTC_register_interrupt(&irq_led, TIMER_COUNTER_1_IRQ, P_LOWEST);
	
	current = 2;
	
	tc_init_waveform(tc0, &WAVEFORM_OPT_1);
	//4Hz = 4 cycles / second
	tc_write_rc(tc0, TC_CHANNEL_1, FOURHERTZ_TC5);
	tc_configure_interrupts(tc0, TC_CHANNEL_1, &TC_INTERRUPT_0);
	tc_start(tc0, TC_CHANNEL_1);
	
	tc_init_waveform(tc0, &WAVEFORM_OPT_2);
	tc_write_rc(tc0, TC_CHANNEL_2, TWOKHERTZ_TC4);
	tc_configure_interrupts(tc0, TC_CHANNEL_2, &TC_INTERRUPT_0);
	tc_start(tc0, TC_CHANNEL_2);
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
	
	//LED_Toggle(LED1);
	LED_Off(LED4);
	print_dbg("Taper S ou X pour demarrer l'acquisition de donnees: \n");
}

int main (void)
{
	intialization();
	
	while(1)
	{
		if(IS_ADC_STARTED && IS_DATA_ACQUISITION_RDY)
		{
			AVR32_ADC.cr = AVR32_ADC_START_MASK;
		}
		
		if(IS_LED)
		{
			TOGGLE_LED(LED0);
			if(IS_DATA_ACQUISITION_RDY)
			{
				TOGGLE_LED(LED1);	
			}
			CLEAR_LED_FLAG;
		}
		//Les valeurs du UART
		
		if((incomingSerialValue == 'x' || incomingSerialValue == 'X') && IS_ALREADY_ON)
		{
			CLEAR_DATA_ACQUISITION_FLAG;
			CLEAR_ALREADY_ON;
			LED_Off(LED1);
		}
		
		if((incomingSerialValue == 's' || incomingSerialValue == 'S') && !IS_ALREADY_ON)
		{
			(LED_Test(LED0) == true) ? LED_On(LED1) : LED_Off(LED1);
			booleanValues |= ALREADY_ON_FLAG;
			booleanValues |= DATA_ACQUISITION_RDY;
		}
		
		//if(sensorValueReady || (potValueReady && lightSensorSent))
		if(IS_SENSOR_VAL_RDY || (POTENTIOMETER_VAL_RDY && lightSensorSent))
		{
			/*if(AVR32_USART1.csr & (AVR32_USART_CSR_TXRDY_MASK))
			{
				booleanValues |= DEPASSEMENT_UART;
			}*/
			AVR32_USART1.ier = AVR32_USART_IER_TXRDY_MASK;
		}
		
		//If we pressed the PB0, we change the hertz
		if(IS_PUSHED)
		{
			CLEAR_PUSH_BUTTON;
			switch(current)
			{
				case 1:
					tc_write_rc(tc0, TC_CHANNEL_2, TWOKHERTZ_TC4);
					current = 2;
					break;
				case 2:
					tc_write_rc(tc0, TC_CHANNEL_2, ONEKHERTZ_TC4);
					current = 1;
					break;
			}
		}
				
		// Dépassements
		if(IS_DEPASSEMENT_ADC)
		{
			LED_On(LED3);
		}
		if(IS_DEPASSEMENT_UART)
		{
			LED_On(LED4);
		}
	}
}
