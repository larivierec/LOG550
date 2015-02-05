/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
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
#define TIMER_COUNTER_1             (&AVR32_TC)
#define TIMER_COUNTER_2             0
#define TIMER_COUNTER_1_IRQ_HIGH    AVR32_TC_IRQ2
#define TIMER_COUNTER_1_IRQ_MED     AVR32_TC_IRQ1
#define TIMER_COUNTER_1_IRQ_LOW     AVR32_TC_IRQ0
#define FPBA                        FOSC0
#define FALSE						0

volatile U8 i;
U32 incomingSerialValue;
U8 compteur;

volatile avr32_tc_t *tc = TIMER_COUNTER_1;

__attribute__((__interrupt__))
static void tc_irq(void)
{
	// La lecture du registre SR efface le fanion de l'interruption.
	tc_read_sr(TIMER_COUNTER_1, TC_CHANNEL_1);
	i = 1;
}

__attribute__((__interrupt__))
static void irq_serial(void)
{	
	if (AVR32_USART1.csr & (AVR32_USART_CSR_RXRDY_MASK))
	{
		//Lire le char recu dans registre RHR, et le stocker dans un 32bit
		incomingSerialValue = (AVR32_USART1.rhr & AVR32_USART_RHR_RXCHR_MASK);
		//Eliminer la source de l'IRQ, bit RXRDY (automatiquement mis a zero a la lecture de RHR)
	}
}

void intialization(void)
{
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

	static const tc_interrupt_t TC_INTERRUPT =
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
	
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
	gpio_enable_module(USART_GPIO_MAP,sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	
	init_dbg_rs232(FOSC0);
	Disable_global_interrupt();
	INTC_init_interrupts();	
	INTC_register_interrupt(&irq_serial, AVR32_USART1_IRQ, AVR32_INTC_INT0);
	AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
	
	INTC_register_interrupt(&tc_irq, TIMER_COUNTER_1_IRQ_LOW, AVR32_INTC_INT0);
	
	Enable_global_interrupt();
	print_dbg("Taper S ou X pour demarrer l'acquisition de donnees: \n");
	
	tc_init_waveform(tc, &WAVEFORM_OPT_1);
	tc_write_rc(tc, TC_CHANNEL_1, (FPBA / 32) / 10);
	tc_configure_interrupts(tc, TC_CHANNEL_1, &TC_INTERRUPT);
	tc_start(tc, TC_CHANNEL_1); 
	i = 0;
}

int main (void)
{
	

	intialization();
	

	while(1) // Boucle inifinie a vide !
	{
		if(i == 1)
		{
			gpio_tgl_gpio_pin(LED0_GPIO);
			i=0;
		}
		if(incomingSerialValue == 'x' || incomingSerialValue == 'X')
		{
			gpio_tgl_gpio_pin(LED1_GPIO);
			incomingSerialValue = 'a';
		}
		if(incomingSerialValue == 's' || incomingSerialValue == 'S')
		{
			gpio_tgl_gpio_pin(LED2_GPIO);
			incomingSerialValue = 'a';
		}
	}
	
	
	// Insert system clock initialization code here (sysclk_init()).

	//board_init();

	// Insert application code here, after the board has been initialized.
}
