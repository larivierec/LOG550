TODAY

Youuploaded an item
21:01
C
main.c
EARLIER THIS WEEK

Samy Lemcelliedited an item
Mon 21:27
Google Docs
Cours 6

Samy Lemcellicreated an item in
Mon 18:04
Google Drive Folder
LOG550
Created items:
Google Docs
Cours 6

Youedited an item
Sun 16:14
Google Docs
Revision Intra

Youedited an item
Sun 15:50
Google Docs
Cours 5

Youmoved an item to
Sun 15:26
Google Drive Folder
LOG550
Moved items:
Google Docs
Revision Intra

Youedited an item
Sun 15:24
Google Docs
Revision Intra

Youcreated an item in
Sun 14:05
Google Drive Folder
Chapitre 4
Created items:
Google Docs
Revision Intra
LAST WEEK

Youedited an item
17 Feb
Google Docs
Plan scolaire

Samy Lemcelliedited an item
16 Feb
Google Docs
Cours 5

Samy Lemcellicreated an item in
16 Feb
Google Drive Folder
LOG550
Created items:
Google Docs
Cours 5
EARLIER THIS MONTH
S
Simon Richard-Girouxmoved 2 items to the bin
12 Feb
Google Docs
GTI350_ModeleRapportLab4
Google Docs
GTI350_ModeleRapportLab3

Youadded an item to
11 Feb
Google Drive Folder
LOG550
Added items:
Google Drive Folder
LOG550

Youcreated an item in
Computer • 6 Feb
Google Drive Folder
Winter 2015
Created items:
PDF
Proto1_DescriptionSeance_1_H2015.pdf
LAST MONTH

Youcreated an item in
Computer • 31 Jan
Google Drive Folder
Personal
Created items:
PDF
DepotDirecteChristopherLariviere.pdf

Youmoved an item to the bin
29 Jan
Google Sheets
Untitled spreadsheet

Youcreated an item in
29 Jan
Google Drive Folder
School
Created items:
Google Sheets
Untitled spreadsheet

Youmoved an item to the bin
Mobile • 25 Jan
Unknown File
American.Sniper.2014.DVDSCR.XviD.AC3-EVO.torrent

Youcreated 8 items in
Computer • 25 Jan
Google Drive Folder
My Scans
Created items:
PDF
Triotech.pdf
PDF
passport.pdf
PDF
proof of residency.pdf
Image
opus.bmp
PDF
DepotDirecteChristopherLariviere.pdf
PDF
CMC_Electronique.pdf
Show all...

Youcreated an item in
Computer • 25 Jan
Google Drive Folder
PC Stuff
Created items:
Google Drive Folder
My Scans
Show more activity
2 GB used (6%)
Buy more storage
main.cOpen
/**
* Bonjour :)
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

#define LIGHT_CHANNEL				2
#define POT_CHANNEL					1

#define FOURHERTZ_TC5				(FOSC0 >> 7) / 8 //Every second, 4 clicks toggle twice
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
#define USART_RDY					1 << 9

#define CLEAR_PUSH_BUTTON			(booleanValues &= ~0x01)
#define CLEAR_DEPASSEMENT_UART_FLAG	(booleanValues &= ~0x02)
#define CLEAR_DEPASSEMENT_ADC_FLAG	(booleanValues &= ~0x04)
#define CLEAR_LED_FLAG				(booleanValues &= ~0x08)
#define CLEAR_SENSOR_FLAG			(booleanValues &= ~0x10)
#define CLEAR_POTENTIOMETER_FLAG	(booleanValues &= ~0x20)
#define CLEAR_ADC_CONVERSION_FLAG	(booleanValues &= ~0x40)
#define CLEAR_DATA_ACQUISITION_FLAG	(booleanValues &= ~0x80)
#define CLEAR_ALREADY_ON			(booleanValues &= ~0x100)
#define CLEAR_USART_RDY				(booleanValues &= ~0x200)

#define IS_PUSHED					(booleanValues == (booleanValues | PUSHED_PB_0))
#define IS_DEPASSEMENT_UART			(booleanValues == (booleanValues | DEPASSEMENT_UART))
#define IS_DEPASSEMENT_ADC			(booleanValues == (booleanValues | DEPASSEMENT_ADC))
#define IS_LED						(booleanValues == (booleanValues | INDICATION_LED))
#define IS_SENSOR_VAL_RDY			(booleanValues == (booleanValues | SENSOR_VAL_RDY))
#define IS_POTENTIOMETER_VAL_RDY	(booleanValues == (booleanValues | POTENTIOMETER_VAL_RDY))
#define IS_ADC_STARTED				(booleanValues == (booleanValues | START_ADC_CONVERSION))
#define IS_DATA_ACQUISITION_RDY		(booleanValues == (booleanValues | DATA_ACQUISITION_RDY))
#define IS_ALREADY_ON				(booleanValues == (booleanValues | ALREADY_ON_FLAG))
#define IS_USART_RDY				(booleanValues == (booleanValues | USART_RDY))

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

//Variables

U32 incomingSerialValue;

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
	//Receiving
	if (AVR32_USART1.csr & (AVR32_USART_CSR_RXRDY_MASK))
	{
		incomingSerialValue = (AVR32_USART1.rhr & AVR32_USART_RHR_RXCHR_MASK);
	}
	//transmitting
	else
	{
		AVR32_USART1.idr = AVR32_USART_IDR_TXRDY_MASK;
	}
	booleanValues |= USART_RDY;
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
	
	//potentiometer conversion done
	if(statusRegister & AVR32_ADC_IER_EOC1_MASK)
	{
		if(IS_POTENTIOMETER_VAL_RDY  && !(AVR32_USART1.csr & (AVR32_USART_CSR_TXRDY_MASK)))
		{
			booleanValues |= DEPASSEMENT_UART;
		}
		potValue = AVR32_ADC.cdr1;
		booleanValues |= POTENTIOMETER_VAL_RDY;
	}
	//light sensor conversion done
	if(statusRegister & AVR32_ADC_IER_EOC2_MASK)
	{
		if(IS_SENSOR_VAL_RDY && !(AVR32_USART1.csr & (AVR32_USART_CSR_TXRDY_MASK)))
		{
			booleanValues |= DEPASSEMENT_UART;
		}
		lightSensorValue = AVR32_ADC.cdr2;
		booleanValues |= SENSOR_VAL_RDY;
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
	//Configure the ADC to use 8 bit values instead of 10
	AVR32_ADC.mr |= 1 << AVR32_ADC_LOWRES_OFFSET;
	
	//Enable the Light Sensor
	AVR32_ADC.cher = 1 << LIGHT_CHANNEL;
	//Enable the potentiometer
	AVR32_ADC.cher = 1 << POT_CHANNEL;
	
	
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
	init_dbg_rs232_ex(57600,FOSC0);
	
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
	print_dbg("\nTaper S ou X pour demarrer l'acquisition de donnees: \n");
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
		//Transmitting
		if(IS_USART_RDY)
		{
			if(IS_SENSOR_VAL_RDY && IS_USART_RDY)
			{
				
				AVR32_USART1.thr = 	lightSensorValue | 0x01;
				CLEAR_SENSOR_FLAG;
				CLEAR_USART_RDY;
				
				AVR32_USART1.ier = AVR32_USART_IER_TXRDY_MASK;
			}
			if(IS_POTENTIOMETER_VAL_RDY  && IS_USART_RDY)
			{
				AVR32_USART1.thr = potValue & ~0x01;
				CLEAR_POTENTIOMETER_FLAG;
				CLEAR_USART_RDY;
				
				AVR32_USART1.ier = AVR32_USART_IER_TXRDY_MASK;
			}
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