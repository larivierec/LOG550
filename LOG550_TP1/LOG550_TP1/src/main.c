/*************************************************************************
*	 Fichier:  main.c
*	 Auteur:   Christopher Lariviere, Samy Lemcelli
*	 Date:	   2/26/2015
*/

/** @mainpage
<center><b>Laboratoire de systemes informatiques en temps reel</b></center>

@author <ul> <li> Christopher Lariviere <li>Samy Lemcelli </ul>

@section MainSection1 Description

Le but du systeme sur le microcontroleur est de convertir le signal analogique
en donnees numeriques avec l'ADC et de les transferer vers le PC par lien en serie soit en:

Le but du premier laboratoire de LOG550 est d'utilise les nombreuses drivers du cadriciel ASF (Atmel Software Framework).
Ce cadriciel nous permettra d'explorer davantage et comprendre ce qui se passe sur un microcontrolleur de ce type.
-
<ul>
<li> 57600 bauds </li>
<li> 38400 bauds </li>
</ul>


- <b>Logiciel necessaire</b> : AVR-Studio 6.2, AVR-GCC, Oscilloscope JAVA
- <b>Materiel necessaire</b> : ATmega32 on EVK1100 board
- <b>Librairies/Modules necessaire</b> :
<ul>
<li> ADC (Analog to Digital Converter - Driver)
<li> PM (Power Manager - Driver)
<li> GPIO (General Purpose Input/Output - Driver)
<li> INTC (Interrupt Controller - Driver)
<li> TC	  (Timer/Counter - Driver)
<li> USART	  (Universal Synchronous/Asynchronous Receiver/Transmitter - Driver + Debug Strings Service)
<li> Generic Board Support	  (Driver)
</ul>

@file main.c Contient le programme de LOG550 developpe par Christopher Lariviere et Samy Lemcelli
*/

#include <asf.h>
#include "compiler.h"

#define TC_CHANNEL_1				0
#define TC_CHANNEL_2				1
#define TIMER_COUNTER				(&AVR32_TC)
#define TIMER_COUNTER_1_IRQ			AVR32_TC_IRQ0
#define TIMER_COUNTER_2_IRQ			AVR32_TC_IRQ1
#define P_HIGH						AVR32_INTC_INT3 /**< La priorite la plus haute, utilise pour l'initialisation des IRQ */
#define P_MED						AVR32_INTC_INT2 /**< La priorite haute, utilise pour l'initialisation des IRQ */
#define P_LOW						AVR32_INTC_INT1 /**< La priorite moyenne, utilise pour l'initialisation des IRQ */
#define P_LOWEST					AVR32_INTC_INT0 /**< La priorite plus basse, utilise pour l'initialisation des IRQ */
#define FPBA						(FOSC0 >> 4) /**< Correspond a 12MHz / 32 */
#define FALSE						0			 /**< Fake booleen */

#define LIGHT_CHANNEL				2			/**< Le numero du channel de la lumiere */
#define POT_CHANNEL					1			/**< Le numero du channel du potentiometre */

#define FOURHERTZ_TC5				(FOSC0 >> 7) / 8 /**< A chaque seconde, toggle deux fois avec l'horloage TC5 \n Le calcul = 12MHz bit shift 7 = (12MHz / 128) / 8 */
#define TWOKHERTZ_TC4				FPBA / 4000		 /**< A chaque seconde, recupere 2000 echantillons par canal avec l'horloge TC4 */
#define ONEKHERTZ_TC4				FPBA / 2000		 /**< A chaque seconde, recupere 1000 echantillons par canal avec l'horloge TC4 */
#define FOURKHERTZ_TC4				FPBA / 9000		 /**< A chaque seconde, recupere 2000 echantillons par canal avec l'horloge TC4 */



/************************************************************************/
/* Le numero du pin du push button										*/
/************************************************************************/

#define PUSH_BUTTON_0				88	/**< Le numero du GPIO PB_0 */

/************************************************************************/
/* Usable boolean bits													*/
/************************************************************************/

#define PUSHED_PB_0					1 << 0 /**< correspond au bit 0 de la variable booleanValues */
#define DEPASSEMENT_UART			1 << 1 /**< correspond au bit 1 de la variable booleanValues */
#define DEPASSEMENT_ADC				1 << 2 /**< correspond au bit 2 de la variable booleanValues */
#define INDICATION_LED				1 << 3 /**< correspond au bit 3 de la variable booleanValues */
#define SENSOR_VAL_RDY				1 << 4 /**< correspond au bit 4 de la variable booleanValues */
#define POTENTIOMETER_VAL_RDY		1 << 5 /**< correspond au bit 5 de la variable booleanValues */
#define START_ADC_CONVERSION		1 << 6 /**< correspond au bit 6 de la variable booleanValues */
#define DATA_ACQUISITION_RDY		1 << 7 /**< correspond au bit 7 de la variable booleanValues */
#define ALREADY_ON_FLAG				1 << 8 /**< correspond au bit 8 de la variable booleanValues */
#define USART_RDY					1 << 9 /**< correspond au bit 9 de la variable booleanValues */

#define CLEAR_PUSH_BUTTON			(booleanValues &= ~0x01)	/**<Reset le bit 1 de notre variable de flag */
#define CLEAR_DEPASSEMENT_UART_FLAG	(booleanValues &= ~0x02)	/**<Reset le bit 2 de notre variable de flag */
#define CLEAR_DEPASSEMENT_ADC_FLAG	(booleanValues &= ~0x04)	/**<Reset le bit 3 de notre variable de flag */
#define CLEAR_LED_FLAG				(booleanValues &= ~0x08)	/**<Reset le bit 4 de notre variable de flag */
#define CLEAR_SENSOR_FLAG			(booleanValues &= ~0x10)	/**<Reset le bit 5 de notre variable de flag */
#define CLEAR_POTENTIOMETER_FLAG	(booleanValues &= ~0x20)	/**<Reset le bit 6 de notre variable de flag */
#define CLEAR_ADC_CONVERSION_FLAG	(booleanValues &= ~0x40)	/**<Reset le bit 7 de notre variable de flag */
#define CLEAR_DATA_ACQUISITION_FLAG	(booleanValues &= ~0x80)	/**<Reset le bit 8 de notre variable de flag */
#define CLEAR_ALREADY_ON			(booleanValues &= ~0x100)	/**<Reset le bit 9 de notre variable de flag */
#define CLEAR_USART_RDY				(booleanValues &= ~0x200)	/**<Reset le bit 10 de notre variable de flag */

#define IS_PUSHED					(booleanValues == (booleanValues | PUSHED_PB_0))			/**< Verifie si le bit 1 est active */
#define IS_DEPASSEMENT_UART			(booleanValues == (booleanValues | DEPASSEMENT_UART))		/**< Verifie si le bit 2 est active */
#define IS_DEPASSEMENT_ADC			(booleanValues == (booleanValues | DEPASSEMENT_ADC))		/**< Verifie si le bit 3 est active */
#define IS_LED						(booleanValues == (booleanValues | INDICATION_LED))			/**< Verifie si le bit 4 est active */
#define IS_SENSOR_VAL_RDY			(booleanValues == (booleanValues | SENSOR_VAL_RDY))			/**< Verifie si le bit 5 est active */
#define IS_POTENTIOMETER_VAL_RDY	(booleanValues == (booleanValues | POTENTIOMETER_VAL_RDY))	/**< Verifie si le bit 6 est active */
#define IS_ADC_STARTED				(booleanValues == (booleanValues | START_ADC_CONVERSION))	/**< Verifie si le bit 7 est active */
#define IS_DATA_ACQUISITION_RDY		(booleanValues == (booleanValues | DATA_ACQUISITION_RDY))	/**< Verifie si le bit 8 est active */
#define IS_ALREADY_ON				(booleanValues == (booleanValues | ALREADY_ON_FLAG))		/**< Verifie si le bit 9 est active */
#define IS_USART_RDY				(booleanValues == (booleanValues | USART_RDY))				/**< Verifie si le bit 10 est active */

#define TOGGLE_LED(ledNumber)			(LED_Toggle(ledNumber))	 /**< Toggle le GPIO LEDNUMBER */
#define GET_GPIOPORT_NUMBER(GPIONUMBER)	(GPIONUMBER / 32)		 /**< Retourne le PORT du numero GPIO */

/** Variables globales */
/** @brief Configuration du peripherique TC et ces variables globales */
static const tc_waveform_opt_t WAVEFORM_OPT_1 =
{
	.channel  = TC_CHANNEL_1,

	.bswtrg	  = TC_EVT_EFFECT_NOOP,
	.beevt	  = TC_EVT_EFFECT_NOOP,
	.bcpc	  = TC_EVT_EFFECT_NOOP,
	.bcpb	  = TC_EVT_EFFECT_NOOP,

	.aswtrg	  = TC_EVT_EFFECT_NOOP,
	.aeevt	  = TC_EVT_EFFECT_NOOP,
	.acpc	  = TC_EVT_EFFECT_NOOP,
	.acpa	  = TC_EVT_EFFECT_NOOP,
	.wavsel	  = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
	.enetrg	  = FALSE,
	.eevt	  = 0,
	.eevtedg  = TC_SEL_NO_EDGE,
	.cpcdis	  = FALSE,
	.cpcstop  = FALSE,
	.burst	  = FALSE,
	.clki	  = FALSE,
	.tcclks	  = TC_CLOCK_SOURCE_TC5
};

static const tc_waveform_opt_t WAVEFORM_OPT_2 =
{
	.channel  = TC_CHANNEL_2,

	.bswtrg	  = TC_EVT_EFFECT_NOOP,
	.beevt	  = TC_EVT_EFFECT_NOOP,
	.bcpc	  = TC_EVT_EFFECT_NOOP,
	.bcpb	  = TC_EVT_EFFECT_NOOP,

	.aswtrg	  = TC_EVT_EFFECT_NOOP,
	.aeevt	  = TC_EVT_EFFECT_NOOP,
	.acpc	  = TC_EVT_EFFECT_NOOP,
	.acpa	  = TC_EVT_EFFECT_NOOP,
	.wavsel	  = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
	.enetrg	  = FALSE,
	.eevt	  = 0,
	.eevtedg  = TC_SEL_NO_EDGE,
	.cpcdis	  = FALSE,
	.cpcstop  = FALSE,
	.burst	  = FALSE,
	.clki	  = FALSE,
	.tcclks	  = TC_CLOCK_SOURCE_TC4
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


U32 incomingSerialValue;
volatile U32 lightSensorValue = 0;	/**< Contient la valeur de la capteur de lumiere */
volatile U32 potValue = 0;			/**< Contient la valeur du potentiometre */
volatile avr32_tc_t *tc0 = TIMER_COUNTER;
volatile U32 booleanValues = 0; /**< Variable 32 bit non-signe contenant tout nos flags pour notre programme */
volatile int current =1;


__attribute__((__interrupt__)) 
static void irq_led(void)
{
	// La lecture du registre SR efface le fanion de l'interruption.
	tc_read_sr(TIMER_COUNTER, TC_CHANNEL_1);
	booleanValues |= INDICATION_LED;
}


/*************************************************************************
 Fonction : irq_serial_communication(void)  */
/** @brief Interrupt Request provenant d'un lecteur un char ou de l'envoi d'une valeur pret par le ADC

*************************************************************************/

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

/*************************************************************************
 Fonction : irq_push_button(void)  */
/** @brief Interrupt Request lorsque nous appuyons sur le PB0

*************************************************************************/

__attribute__((__interrupt__)) 
static void irq_push_button(void)
{
	if(booleanValues | PUSHED_PB_0)
	{
		booleanValues |= PUSHED_PB_0;
	}
	gpio_clear_pin_interrupt_flag(PUSH_BUTTON_0);
}

/*************************************************************************
 Fonction : irq_adc_channel(void)  */
/** @brief Interrupt Request provenant d'un EOC (End of Conversion) soit pour la lumiere ou le potentiometre

*************************************************************************/

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
		if(IS_POTENTIOMETER_VAL_RDY	 && !(AVR32_USART1.csr & (AVR32_USART_CSR_TXRDY_MASK)))
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

/*************************************************************************
 Fonction : irq_adc_timer(void)  */
/** @brief Interrupt Request provenant du Timer du ADC.

*************************************************************************/

__attribute__((__interrupt__)) 
static void irq_adc_timer(void)
{
	tc_read_sr(TIMER_COUNTER, TC_CHANNEL_2);
	if(!IS_ADC_STARTED)
	{
		booleanValues |= START_ADC_CONVERSION;
	}
}

/*************************************************************************
 Fonction : adc_init(void)  */
/** @brief Initialise le ADC pour recuperer les 8 bits de chaque canaux configurer.\n
		   Light channel = 2 et Pot channel = 1 avec une interruption de priorite 3

*************************************************************************/

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

/*************************************************************************
 Fonction : timercounter_init(void)  */
/** @brief Initialise les deux timer counter a priorite basse.

*************************************************************************/

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

/*************************************************************************
 Fonction : usart_init(void)  */
/** @brief Initialise le USART

*************************************************************************/


void usart_init(void)
{
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
	gpio_enable_module(USART_GPIO_MAP,sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	init_dbg_rs232_ex(57600,FOSC0);
	
	INTC_register_interrupt(&irq_serial_communication, AVR32_USART1_IRQ, P_LOWEST);
	
	AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
}

/*************************************************************************
 Fonction : push_button_init(void)  */
/** @brief Initialise le bouton du GPIO88 => PB0

*************************************************************************/

void push_button_init(void)
{
	INTC_register_interrupt(&irq_push_button,(AVR32_GPIO_IRQ_0+PUSH_BUTTON_0/8),AVR32_INTC_INT0);
	gpio_enable_gpio_pin(PUSH_BUTTON_0);
	gpio_enable_pin_interrupt(PUSH_BUTTON_0,GPIO_FALLING_EDGE);
}

/*************************************************************************
 Fonction : initialization(void)  */
/** @brief Initialise les 4 IRQ et deux timer clock sources

*************************************************************************/

void initialization(void)
{
	Disable_global_interrupt();
	
	INTC_init_interrupts();
	
	adc_init();
	push_button_init();
	usart_init();
	timercounter_init();
	
	Enable_global_interrupt();
	
	LED_Off(LED4);
	print_dbg("\nTaper S ou X pour demarrer l'acquisition de donnees: \n");
}

/*************************************************************************
 Fonction : main(void)  */
/** @brief Lance l'initialisation et debute l'arriere plan pour le microcontrolleur

*************************************************************************/


int main(void)
{
	initialization();
	
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
		
		
		if(IS_USART_RDY)
		{
			if(IS_SENSOR_VAL_RDY && IS_USART_RDY)
			{
				
				AVR32_USART1.thr =	lightSensorValue | 0x01;
				CLEAR_SENSOR_FLAG;
				CLEAR_USART_RDY;
				
				AVR32_USART1.ier = AVR32_USART_IER_TXRDY_MASK;
			}
			if(IS_POTENTIOMETER_VAL_RDY	 && IS_USART_RDY)
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