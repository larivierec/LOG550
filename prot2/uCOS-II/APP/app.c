/************************************************************************/
/*                                                                      */
/************************************************************************/

#include  "includes.h"   // Ceci inclu os_cfg.h, config des services noyaux UCOS-II...

/**** Reservation du STACK de chaque tache ************************************************/

#define ADC_START_FLAG	1 << 0
#define ALARM_FLAG		1 << 1
#define LED1_STATE		1 << 2

#define CLEAR_ADC_FLAG	(booleanFlags &= ~0x01)
#define CLEAR_ALARM_FLAG (booleanFlags &= ~0x02)
#define CLEAR_LED1_STATE (booleanFlags &= ~0x04)

#define IS_ADC_STARTED	(booleanFlags == (booleanFlags | ADC_START_FLAG))
#define IS_ALARM_ON		(booleanFlags == (booleanFlags | ALARM_FLAG))
#define IS_LED1_ON		(booleanFlags == (booleanFlags | LED1_STATE))

#define LIGHT_CHANNEL				2			/**< Le numero du channel de la lumiere */
#define POT_CHANNEL					1			/**< Le numero du channel du potentiometre */

volatile CPU_INT32U booleanFlags = 0;

#define OS_TASK_STK_SIZE 256

OS_STK  LED_Stack[OS_TASK_STK_SIZE];
OS_STK  UART_RX_Stack[OS_TASK_STK_SIZE];
OS_STK  UART_TX_Stack[OS_TASK_STK_SIZE];
OS_STK  ADC_Stack[OS_TASK_STK_SIZE];
OS_STK  Alarm_Stack[OS_TASK_STK_SIZE];


static  void  LED_flash(void *p_arg);
static  void  UART_Cmd_RX(void *p_arg);
static  void  UART_SendSample(void *p_arg);
static  void  ADC_Cmd(void *p_arg);
static	void  Alarm_msgQ(void *p_arg);

/** Semaphores */

OS_EVENT	*Start_Sem;		// Conversion has started
OS_EVENT	*UART_TX_Sem;		// Semaphore
OS_EVENT	*Alarm_Sem;		// Alarm has been triggered or not

OS_EVENT	*Mbox1;		// Semaphore
OS_EVENT	*ADCMsg;		// Mailbox

void  *ADCMsgFIFO[3];		// Ma MessageQueue (FIFO)
void Init_IO_Usager(void);          // Fonction definie localement

/**** Fonction principale main()***********************************************************/
int  main (void)
{
	// Initialize the needed elements
	BSP_Init();         // Set le timer et demarre les ISR_TICKS.
	// Doit absolument etre appele par la premiere tache a operer.
	// Attention, desormais on roule a 48MHz et USART a 48/2MHz
	CPU_IntDis();       /* Desactive toute les interrupts pendant l'initialisation  */
	OSInit();           /* Initialise "uC/OS-II, The Real-Time Kernel"              */
	Init_IO_Usager();   // Initialisation des differents I/O par l'usager

	/* Creation de toute les taches...a des priorites differentes (1<prio<25) */
	OSTaskCreate(LED_flash, NULL, (OS_STK *)&LED_Stack[OS_TASK_STK_SIZE-1], 1);
	OSTaskCreate(UART_Cmd_RX, NULL, (OS_STK *)&UART_RX_Stack[OS_TASK_STK_SIZE-1], 2);
	OSTaskCreate(UART_SendSample, NULL, (OS_STK *)&UART_TX_Stack[OS_TASK_STK_SIZE-1], 3);
	OSTaskCreate(ADC_Cmd, NULL, (OS_STK *)&ADC_Stack[OS_TASK_STK_SIZE-1], 4);
	OSTaskCreate(Alarm_msgQ, NULL,(OS_STK *)&Alarm_Stack[OS_TASK_STK_SIZE-1], 5);

	Start_Sem   = OSSemCreate(0);	// Conversion has started
	UART_TX_Sem = OSSemCreate(0);	// Transfer Ready
	Alarm_Sem = OSSemCreate(0);		// Alarm has been triggered or not
	
	Mbox1   = OSMboxCreate(NULL);   // MailBox,   initialisation
	ADCMsg   = OSQCreate(ADCMsgFIFO, 3); // Message Queue of the ADC Initialized
	
	

	OSStart();          /* Demarre le multitasking (Donne le controle au noyau uC/OS-II)  */
	// Le code ici ne sera jamais execute
	return (0);         /* Pour eviter le warnings en GCC, prototype (int main (void))    */
}

/******************************************************************************************/
/* Definition des taches sous UCOS-II                                                     */
/*  - Toutes les taches ont une priorite differente                                       */
/*  - Le scheduler de UCOS-II execute TOUJOURS la tache la plus prioritaire qui est READY.*/
/******************************************************************************************/

/******************************************************************************************/
static  void  LED_flash (void *p_arg)
{
	(void)p_arg;          // Pour eviter le warnings en GCC
	CPU_INT08U   err, MsgQDataRX;

	

	/*---Fin de l'initialisation----------*/

	while (1) {           // Tache, une boucle infinie.
		
		
		LED_Toggle(1);
		if(IS_LED1_ON) // Have to track LED1 state. It was done by LED.h in old lab.
		{
			CLEAR_LED1_STATE;
		}
		else
		{
			booleanFlags |= LED1_STATE;
		}
		
		if(IS_ADC_STARTED){
			LED_Toggle(2);
		}
		
		OSTimeDly(200);// 200ms == 5 fois secondes
	}
}
/******************************************************************************************/
static  void  UART_Cmd_RX (void *p_arg)
{
	(void)p_arg;
	char MboxDataRX;

	while (1) {
		MboxDataRX=BSP_USART_ByteRd(1); // Read from USART1
		
		if(MboxDataRX == 's'){
			OSSemSet(Start_Sem,1,NULL);
			booleanFlags |= 0x01;
			(IS_LED1_ON) ? LED_On(2) : LED_Off(2);
		}
		if(MboxDataRX =='x'){
			OSSemSet(Start_Sem,0,NULL);
			CLEAR_ADC_FLAG;
			LED_Off(2);
		}
		
		OSTimeDly(200); // Shouldn't spam check so delay
	}
}
/******************************************************************************************/
static  void  UART_SendSample (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U MsgQDataRX;

	while (1) {
		CPU_INT08U MsgQDataRX = (CPU_INT08U)OSQPend(ADCMsg, 0, NULL);// Attend le MessageQueue
		BSP_USART_ByteWr(1, MsgQDataRX);
	}
}
/******************************************************************************************/
static  void  ADC_Cmd (void *p_arg)
{
	(void)p_arg;
	
	while (1) {
		if(ADCMsg->OSEventCnt >= 3)
		{
			OSSemPost(Alarm_Sem);
		}
		
		if(IS_ADC_STARTED)
		{
			LED_Toggle(4); // This is just to get a visual queue. Must delete.
			unsigned int statusRegister = AVR32_ADC.sr;
			if(statusRegister & AVR32_ADC_IER_EOC1_MASK)
			{
				OSQPost(ADCMsg, (AVR32_ADC.cdr1 | 0x01));
			}
			
			if(statusRegister & AVR32_ADC_IER_EOC2_MASK)
			{
				OSQPost(ADCMsg, (AVR32_ADC.cdr2 & ~0x01));
			}
		}
		
		OSTimeDly(100);        // Delai en TICKS (1 TICKS=1milisec)
	}
}
/******************************************************************************************/

static	void Alarm_msgQ(void *p_arg){
	(void)p_arg; //cry more noob
	OSSemPend(Alarm_Sem,0,NULL);
	LED_Toggle(3);
}

void Init_IO_Usager(void)
{
	BSP_USART_Init(1,57600) ; //Initialise USART1 at 57600bauds, voir BSP.c
	BSP_USART_printf(1, "\nPrototype 2 : LOG550\n Samy Lemcelli, Christopher Lariviere");
}


