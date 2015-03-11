/******************************************************************************************/
/* Demo2 de UCOS-II                                                                       */
/*    Programme qui lance 4 taches, chaque tache faisant clignoter un LED                 */
/*    Implique l'utilisation de semaphore, mailbox et de messageQueue.                    */
/*    Implique l'utilisation de USART1, mais sans interruption.                           */
/*                                                                                        */
/*  Attention - Les drivers PM et INTC du ASF entre en conflit avec BSP_Init() de UCOS-II */
/******************************************************************************************/

#include  "includes.h"   // Ceci inclu os_cfg.h, config des services noyaux UCOS-II...

/**** Reservation du STACK de chaque tache ************************************************/
#define OS_TASK_STK_SIZE 256
OS_STK  Task1_Stk[OS_TASK_STK_SIZE];
OS_STK  Task2_Stk[OS_TASK_STK_SIZE];
OS_STK  Task3_Stk[OS_TASK_STK_SIZE];
OS_STK  Task4_Stk[OS_TASK_STK_SIZE];

static  void  Task1(void *p_arg);
static  void  Task2(void *p_arg);
static  void  Task3(void *p_arg);
static  void  Task4(void *p_arg);

OS_EVENT	*Sema1;		// Semaphore
OS_EVENT	*Sema2;		// Semaphore
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
	OSTaskCreate(Task1, NULL, (OS_STK *)&Task1_Stk[OS_TASK_STK_SIZE-1], 1);
	OSTaskCreate(Task2, NULL, (OS_STK *)&Task2_Stk[OS_TASK_STK_SIZE-1], 2);
	OSTaskCreate(Task3, NULL, (OS_STK *)&Task3_Stk[OS_TASK_STK_SIZE-1], 3);
	OSTaskCreate(Task4, NULL, (OS_STK *)&Task4_Stk[OS_TASK_STK_SIZE-1], 4);

	Sema1   = OSSemCreate(0);		// Semaphore, initialisation
	Sema2   = OSSemCreate(0);		// Semaphore, initialisation
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
static  void  Task1 (void *p_arg)
{
	(void)p_arg;          // Pour eviter le warnings en GCC
	CPU_INT08U   err, MsgQDataRX;

	BSP_Init();           // Set le timer et demarre les ISR_TICKS.
	                      // Doit absolument etre appele par la premiere tache a operer.
	                      // Attention, desormais on roule a 48MHz et USART a 48/2MHz

	CPU_IntDis();		  // Desactive les IRQ pendant l'initialisation
	Init_IO_Usager();     // Initialisation des differents I/O par l'usager
	CPU_IntEn();		  // Reactive les IRQ

	LED_Toggle(5);
	/*---Fin de l'initialisation----------*/

	while (1) {           // Tache, une boucle infinie.
		MsgQDataRX=*((CPU_INT08U *)OSQPend(MsgQ1, 0, &err));  // Attend le MessageQueue
		BSP_USART_ByteWr(1, MsgQDataRX);
		LED_Toggle(1);
	}
}
/******************************************************************************************/
static  void  Task2 (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U   err, MboxDataRX, MsgQDataPost;
	MsgQDataPost='a';

	while (1) {
		MsgQDataPost++;
		if(MsgQDataPost>'z') MsgQDataPost='a';
		MboxDataRX=*((CPU_INT08U *)OSMboxPend(Mbox1, 0, &err));  // Attend le mailbox
		LED_Toggle(2);
		OSQPost(MsgQ1, &MsgQDataPost);
	}
}
/******************************************************************************************/
static  void  Task3 (void *p_arg)
{
	(void)p_arg;
	CPU_INT08U  err, MboxDataPost;
	MboxDataPost='3';

	while (1) {
		OSSemPend(Sema1, 0, &err);  // Attend le semaphore
		LED_Toggle(3);
		OSMboxPost(Mbox1, &MboxDataPost);
	}
}
/******************************************************************************************/
static  void  Task4 (void *p_arg)
{
	(void)p_arg;

	while (1) {
		OSSemPost(Sema1);
		LED_Toggle(4);
		OSTimeDly(100);        // Delai en TICKS (1 TICKS=1milisec)
	}
}
/******************************************************************************************/

void Init_IO_Usager(void)
{
	 BSP_USART_Init(1,57600) ; //Initialise USART1 at 57600bauds, voir BSP.c
	 BSP_USART_printf(1, "\nUCOS-II Demo2 LOG550\n");
}


