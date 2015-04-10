/**
	Fichier: app.c
	Auteur:	Christopher Lariviere, Samy Lemcelli
	Date:	4/9/2015
*/

/** @mainpage
<center><b>Laboratoire de systemes informatiques en temps reel</b></center>

@author <ul> <li> Christopher Lariviere <li>Samy Lemcelli </ul>

@section MainSection1 Description

Le but du systeme sur le microcontroleur est de convertir le signal analogique
en donnees numeriques avec l'ADC et de les transferer vers le PC par lien en serie soit en:

Le but du deuxieme laboratoire de LOG550 est identique a la premiere.\n
Nous n'utilisons pas directement l'ASF (Atmel Software Framework), nous utilisons plutot
un noyau uCOS-II qui interface avec le EVK1100

-
Vitesse de transfert utilisée
<ul>
<li> 115200 bauds (Bonus - USART0)</li>
<li> 57600 bauds (ADC - USART1)</li>
</ul>
- <b>Logiciel necessaire</b> : AVR-Studio 6.2, AVR-GCC, Oscilloscope JAVA, TeraTerminal PRO pour Bonus
- <b>Materiel necessaire</b> : ATmega32 on EVK1100 board
- <b>Librairies/Modules necessaire</b> :
<ul>
<li> uCOS-II de Microcium qui est inclut dans le fichier zip.
</ul>

-

Specifications
<ul>
<li> 2 canaux operant 500 echantillons/sec
<li> Tout acces a une ressource partagee requiert un semaphore
<li> L'utilisation d'un FIFO fournis par le noyau uCOS-II
<li> 5 taches dont: LED_Flash, UART_Cmd_RX, UART_SendSample, ADC_Cmd(), Alarm_MsgQ
<li> 1 tache (boni): Bonus_Thread() qui s'occupe d'afficher en terminal de l'utilisation CPU et Le nombre de context switch
</ul>

@file app.c Contient le programme de LOG550 developpe par Christopher Lariviere et Samy Lemcelli
*/
#include  "includes.h"   /**<Ceci inclu os_cfg.h, config des services noyaux UCOS-II...*/

#define LIGHT_CHANNEL				2			/**< Le numero du channel de la lumiere */
#define POT_CHANNEL					1			/**< Le numero du channel du potentiometre */

/** Variables Globales */

/**
	@brief defini le stack de chacun des taches
*/
#define OS_TASK_STK_SIZE 512

OS_STK  LED_Stack[OS_TASK_STK_SIZE];					/**< Declaration du stack LED_Stack */
OS_STK  UART_RX_Stack[OS_TASK_STK_SIZE];			/**< Declaration du stack UART_RX_Stack*/
OS_STK  UART_TX_Stack[OS_TASK_STK_SIZE];			/**< Declaration du stack UART_TX_Stack*/
OS_STK  ADC_Stack[OS_TASK_STK_SIZE];					/**< Declaration du stack ADC_Stack*/
OS_STK  Alarm_Stack[OS_TASK_STK_SIZE];				/**< Declaration du stack Alarm_Stack*/
OS_STK  Bonus_Stack[OS_TASK_STK_SIZE];				/**< Declaration du stack Bonus_Stack*/

/**
	@brief	La tache LED_flash qui toggle a chaque 100 ms -> 200 ms pour un toggle complet
*/
static void LED_flash(void *p_arg);

/**
	@brief gere la reception des donnes par l'interface de communication USART
*/
static void UART_Cmd_RX(void *p_arg);

/**
	@brief Envoi le un sample du MessageQueue (ADCMsg) sur le USART1 a 57600 bauds
*/
static void UART_SendSample(void *p_arg);

/**
	@brief Converti un canal du adc lumiere OU potentiometre a chaque milliseconde
*/
static void ADC_Cmd(void *p_arg);

/**
	@brief Nous averti s'il y a eu un depassement au niveau du ADC (ne devrait jamais se reveiller)
*/
static void Alarm_msgQ(void *p_arg);

/**
	@brief Nous affiche sur le USART0 a 115200 bauds l'usage CPU et le nombre de context switch \n
	a chaque 500 ms
*/
static void Bonus_Thread(void *p_arg);

/**<Semaphores */

OS_EVENT	*Start_Sem;		/**<Semaphore de debut de conversion */
OS_EVENT	*UART_TX_Sem;		/**<Semaphore de transmission UART1*/
OS_EVENT	*Alarm_Sem;		/**<Semaphore d'alarm (qui ne devrait pas actionner)*/
OS_EVENT	*LED1_Sem;		/**<Semphore des LEDs*/
OS_EVENT	*SEM_ADC_Alternate	/**<Semaphore d'alternance pour le ADC (canaux)*/;

OS_EVENT	*ADCMsg;		/**<Semaphore pour MessageQueue pour ADC*/

void *ADCMsgFIFO[4];		/**<MessageQueue pour l'ADC (FIFO) */
void Init_IO_Usager(void);

/**
	@brief	Point d'entree pour le programme
*/

int main (void)
{
	BSP_Init();         /*Set le timer et demarre les ISR_TICKS.*/
	CPU_IntDis();       /*Desactive toute les interrupts pendant l'initialisation  */
	OSInit();           /*Initialise "uC/OS-II, The Real-Time Kernel"              */
	Init_IO_Usager();   /*Initialisation des differents I/O par l'usager					*/

	/*Creation de toute les taches, a des priorites differentes (1<prio<25) */
	OSTaskCreate(LED_flash, NULL, (OS_STK *)&LED_Stack[OS_TASK_STK_SIZE-1], 1);
	OSTaskCreate(UART_Cmd_RX, NULL, (OS_STK *)&UART_RX_Stack[OS_TASK_STK_SIZE-1], 2);
	OSTaskCreate(UART_SendSample, NULL, (OS_STK *)&UART_TX_Stack[OS_TASK_STK_SIZE-1], 3);
	OSTaskCreate(ADC_Cmd, NULL, (OS_STK *)&ADC_Stack[OS_TASK_STK_SIZE-1], 4);
	OSTaskCreate(Alarm_msgQ, NULL,(OS_STK *)&Alarm_Stack[OS_TASK_STK_SIZE-1], 5);
	OSTaskCreate(Bonus_Thread, NULL,(OS_STK *)&Bonus_Stack[OS_TASK_STK_SIZE-1], 6);


	Start_Sem   = OSSemCreate(0);				/*Creation du semaphore Start_Sem*/
	UART_TX_Sem = OSSemCreate(0);				/*Creation du semaphore UART_TX_Sem*/
	Alarm_Sem = OSSemCreate(0);					/*Creation du semaphore Alarm_Sem*/
	LED1_Sem = OSSemCreate(0);					/*Creation du semaphore LED1_Sem*/
	SEM_ADC_Alternate = OSSemCreate(0);	/*Creation du semaphore SEM_ADC_Alternate*/
	ADCMsg = OSQCreate(ADCMsgFIFO, 4);	/*Creation du message queue de taille 4 ADCMsg */

	OSStart();
	return (0);
}


static void LED_flash (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U   err, MsgQDataRX;

	OSStatInit();

	while (1) {

		LED_Toggle(1);
		if(LED1_Sem->OSEventCnt == 1) // Have to track LED1 state. It was done by LED.h in old lab.
		{
			OSSemSet(LED1_Sem,0,NULL);
		}
		else
		{
			OSSemSet(LED1_Sem,1,NULL);
		}

		if(Start_Sem->OSEventCnt == 1)
		{
			LED_Toggle(2);
		}
		OSTimeDly(100);// 200ms == 5 fois secondes
	}
}


static void UART_Cmd_RX (void *p_arg)
{
	(void)p_arg;
	char MboxDataRX;

	while (1) {
		MboxDataRX=BSP_USART_ByteRd(1); // Read from USART1

		if(MboxDataRX == 's'){
			OSSemSet(Start_Sem,1,NULL);
			(LED1_Sem->OSEventCnt == 1) ? LED_On(2) : LED_Off(2);
		}
		if(MboxDataRX =='x'){
			OSSemSet(Start_Sem,0,NULL);
			LED_Off(2);
		}

		OSTimeDly(200); // Shouldn't spam check so delay
	}
}

static void Bonus_Thread(void *p_arg)
{
	(void)p_arg;
	char buffer[8];
	while(1)
	{
		OSTimeDly(500);
		BSP_USART_printf(0, "\033[2J");
		BSP_USART_printf(0, "\033[2H");

		BSP_USART_printf(0, "\033[2H");
		snprintf(buffer, 8, "%d", OSCPUUsage);
		BSP_USART_printf(0, "CPU Usage Percentage : ");
		BSP_USART_printf(0, buffer);
		BSP_USART_printf(0, "\033[2H\n");
		BSP_USART_printf(0, "Number of Context Switches: ");
		snprintf(buffer, 8, "%d", OSCtxSwCtr);
		OSCtxSwCtr = 0;
		BSP_USART_printf(0, buffer);
		BSP_USART_printf(0, "\n");
	}
}

static  void  UART_SendSample (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U MsgQDataRX;

	while (1) {
		CPU_INT08U MsgQDataRX = (CPU_INT08U)OSQPend(ADCMsg, 0, NULL);// Attend le MessageQueue
		BSP_USART_ByteWr(1, MsgQDataRX);
	}
}

static void ADC_Cmd (void *p_arg)
{
	(void)p_arg;

	while (1) {
		if(ADCMsg->OSEventCnt >= 4)
		{
			OSSemPost(Alarm_Sem);
		}

		if(Start_Sem->OSEventCnt == 1)
		{
			AVR32_ADC.cr = AVR32_ADC_START_MASK; // starts the ADC Conversion
			 // This is just to get a visual queue. Must delete.
			CPU_INT32U statusRegister = AVR32_ADC.sr;
			if((statusRegister & AVR32_ADC_IER_EOC1_MASK) && SEM_ADC_Alternate->OSEventCnt == 0)
			{
				OSSemSet(SEM_ADC_Alternate,1,NULL);
				OSQPost(ADCMsg, (AVR32_ADC.cdr1 | 0x01));
			}

			if((statusRegister & AVR32_ADC_IER_EOC2_MASK) && SEM_ADC_Alternate->OSEventCnt == 1)
			{
				OSSemSet(SEM_ADC_Alternate,0,NULL);
				OSQPost(ADCMsg, (AVR32_ADC.cdr2 & ~0x01));
			}
		}
		OSTimeDly(1);        // Delai en TICKS (1 TICKS=1milisec)
	}
}

static void Alarm_msgQ(void *p_arg){
	(void)p_arg;
	OSSemPend(Alarm_Sem,0,NULL);
	LED_Toggle(3);
}

/**
	@brief Initialise les GPIO pour l'usager
*/

void Init_IO_Usager(void)
{
	BSP_USART_Init(1,57600) ; //Initialise USART1 at 57600bauds
	BSP_USART_Init(0,115200) ; //Initialise USART0 at 115200 bauds pour le bonus
	BSP_USART_printf(1, "\nPrototype 2 : LOG550\n Samy Lemcelli, Christopher Lariviere");

	AVR32_ADC.mr |= 1 << AVR32_ADC_LOWRES_OFFSET;	//initialize the ADC to convert using only 8-bits
	AVR32_ADC.cher = 1 << LIGHT_CHANNEL; //Enable the Light Sensor
	AVR32_ADC.cher = 1 << POT_CHANNEL;	//Enable the potentiometer
}
