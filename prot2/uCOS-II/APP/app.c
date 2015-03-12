/************************************************************************/
/*                                                                      */
/************************************************************************/

#include  "includes.h"   // Ceci inclu os_cfg.h, config des services noyaux UCOS-II...

/**** Reservation du STACK de chaque tache ************************************************/

#define ADC_START_FLAG	1 << 0
#define ALARM_FLAG		1 << 1

#define CLEAR_ADC_FLAG	(booleanFlags &= ~0x01)
#define CLEAR_ALARM_FLAG (booleanFlags &= ~0x02)

#define IS_ADC_STARTED	(booleanFlags == (booleanFlags | ADC_START_FLAG))
#define IS_ALARM_ON		(booleanFlags == (booleanFlags | ALARM_FLAG))

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

OS_EVENT	*LED_Sem;		// Semaphore
OS_EVENT	*UART_RX_Sem;		// Semaphore
OS_EVENT	*UART_TX_Sem;		// Semaphore
OS_EVENT	*ADC_Sem;		// Semaphore
OS_EVENT	*Alarm_Sem;		// Semaphore

OS_EVENT	*Mbox1;		// Semaphore
OS_EVENT	*MsgQ1;		// Mailbox

void  *MyMsgQ[3];		// Ma MessageQueue (FIFO)
void Init_IO_Usager(void);          // Fonction definie localement

/**** Fonction principale main()***********************************************************/
int  main (void)
{
	CPU_IntDis();       /* Descative toute les interrupts pendant l'initialisation  */
	OSInit();           /* Initialise "uC/OS-II, The Real-Time Kernel"              */

	/* Creation de toute les taches...a des priorites differentes (1<prio<25) */
	OSTaskCreate(LED_flash, NULL, (OS_STK *)&LED_Stack[OS_TASK_STK_SIZE-1], 1);
	OSTaskCreate(UART_Cmd_RX, NULL, (OS_STK *)&UART_RX_Stack[OS_TASK_STK_SIZE-1], 2);
	OSTaskCreate(UART_SendSample, NULL, (OS_STK *)&UART_TX_Stack[OS_TASK_STK_SIZE-1], 3);
	OSTaskCreate(ADC_Cmd, NULL, (OS_STK *)&ADC_Stack[OS_TASK_STK_SIZE-1], 4);
	OSTaskCreate(Alarm_msgQ, NULL,(OS_STK *)&Alarm_Stack[OS_TASK_STK_SIZE-1], 5);

	LED_Sem   = OSSemCreate(0);		// Semaphore, initialisation
	UART_RX_Sem   = OSSemCreate(0);		// Semaphore, initialisation
	UART_TX_Sem = OSSemCreate(0);
	ADC_Sem = OSSemCreate(0);
	Alarm_Sem = OSSemCreate(0);
	
	Mbox1   = OSMboxCreate(NULL);   // MailBox,   initialisation
	MsgQ1   = OSQCreate(MyMsgQ, 3); // MailBox,   initialisation

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

	BSP_Init();           // Set le timer et demarre les ISR_TICKS.
	                      // Doit absolument etre appele par la premiere tache a operer.
	                      // Attention, desormais on roule a 48MHz et USART a 48/2MHz

	CPU_IntDis();		  // Desactive les IRQ pendant l'initialisation
	Init_IO_Usager();     // Initialisation des differents I/O par l'usager
	CPU_IntEn();		  // Reactive les IRQ

	/*---Fin de l'initialisation----------*/

	while (1) {           // Tache, une boucle infinie.
		MsgQDataRX=*((CPU_INT08U *)OSQPend(MsgQ1, 0, &err));  // Attend le MessageQueue
		BSP_USART_ByteWr(1, MsgQDataRX);
		LED_Toggle(1);
		if(IS_ADC_STARTED){
			LED_Toggle(2);
		}		
		if(IS_ALARM_ON){
			LED_On(3);
		}
	}
}
/******************************************************************************************/
static  void  UART_Cmd_RX (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U   err, MboxDataRX, MsgQDataPost;
	MsgQDataPost = 'a';

	while (1) {
		MboxDataRX=*((CPU_INT08U *)OSMboxPend(Mbox1, 0, &err));  // Attend le mailbox
		
		if(MboxDataRX == 's'){
			booleanFlags |= ADC_START_FLAG;
		}
		if(MboxDataRX =='x'){
			CLEAR_ADC_FLAG;
		}
		
		OSQPost(MsgQ1, &MsgQDataPost);
	}
}
/******************************************************************************************/
static  void  UART_SendSample (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U  err, MboxDataPost;
	MboxDataPost='3';

	while (1) {
		OSSemPend(LED_Sem, 0, &err);  // Attend le semaphore
		OSMboxPost(Mbox1, &MboxDataPost);
	}
}
/******************************************************************************************/
static  void  ADC_Cmd (void *p_arg)
{
	(void)p_arg;

	while (1) {
		OSSemPost(LED_Sem);
		OSTimeDly(100);        // Delai en TICKS (1 TICKS=1milisec)
	}
}
/******************************************************************************************/

static	void Alarm_msgQ(void *p_arg){
	(void)p_arg; //cry more noob
	while(1){
	
		
	}
}

void Init_IO_Usager(void)
{
	 BSP_USART_Init(1,57600) ; //Initialise USART1 at 57600bauds, voir BSP.c
	 BSP_USART_printf(1, "\nPrototype 2 : LOG550\n Samy Lemcelli, Christopher Lariviere");
}


