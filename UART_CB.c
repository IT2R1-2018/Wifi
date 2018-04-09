// Utilisation Event UART en emission-reception

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "LPC17xx.h"                    // Device header
#include "cmsis_os.h"                   // CMSIS RTOS header file
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

void Thread_T (void const *argument);                             // thread function Transmit
osThreadId tid_Thread_T;                                          // thread id
osThreadDef (Thread_T, osPriorityNormal, 1, 0);                   // thread object

void Thread_R (void const *argument);                             // thread function Receive
osThreadId tid_Thread_R;                                          // thread id
osThreadDef (Thread_R, osPriorityNormal, 1, 0);                   // thread object

// prototypes fonctions
void Init_UART(void);
void sendCommand(char * command, int tempo_ms);
void Init_WiFi(void);

extern ARM_DRIVER_USART Driver_USART1;

//fonction de CB lancee si Event T ou R
void event_UART(uint32_t event)
{
	switch (event) {
		
		case ARM_USART_EVENT_RECEIVE_COMPLETE : 	osSignalSet(tid_Thread_R, 0x01);
																							break;
		
		case ARM_USART_EVENT_SEND_COMPLETE  : 	osSignalSet(tid_Thread_T, 0x02);
																							break;
		
		default : break;
	}
}

int main (void){

	osKernelInitialize ();                    // initialize CMSIS-RTOS
	
	// initialize peripherals here
	Init_UART();
	GLCD_Initialize();
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_6x8);
	NVIC_SetPriority(UART1_IRQn,2);
	
	//Creation des 2 taches
	tid_Thread_T = osThreadCreate(osThread(Thread_T),NULL);
	tid_Thread_R = osThreadCreate(osThread(Thread_R),NULL);
	osKernelStart ();                         // start thread execution 
	
	osDelay(osWaitForever);
	
	return 0;
}

void Thread_T (void const *argument) {
	
	char Cmd[30];
	char ReqHTTP[90];
	Init_WiFi();
	while(1);
	
	
}

// Tache de réception et d'affichage

void Thread_R (void const *argument) {

	char RxChar;
	int ligne;
	int i=0;	// i pour position colonne caractère
	char RxBuf[200];
	
  while (1) {
		Driver_USART1.Receive(&RxChar,1);		// A mettre ds boucle pour recevoir 
		osSignalWait(0x01, osWaitForever);	// sommeil attente reception
		
		RxBuf[i]=RxChar;
		i++;
		//Suivant le caractère récupéré
		switch(RxChar)
		{
			case 0x0D: 		//Un retour chariot? On ne le conserve pas...
				i--;
				break;
			case 0x0A:										//Un saut de ligne?
				RxBuf[i-1]=0;											//=> Fin de ligne, donc, on "cloture" la chaine de caractères
				GLCD_DrawString(1,ligne,RxBuf);	//On l'affiche (peut etre trop long, donc perte des caractères suivants??)
				ligne+=10;										//On "saute" une ligne de l'afficheur LCD
			  if(ligne>240)
				{
					ligne=1;
					GLCD_ClearScreen();
					osDelay(2000);
				}
				i=0;													//On se remet au début du buffer de réception pour la prochaine ligne à recevoir
				break;
		}
  }
}

void Init_UART(void){
	Driver_USART1.Initialize(event_UART);
	Driver_USART1.PowerControl(ARM_POWER_FULL);
	Driver_USART1.Control(	ARM_USART_MODE_ASYNCHRONOUS |
							ARM_USART_FLOW_CONTROL_NONE   |
							ARM_USART_DATA_BITS_8		|
							ARM_USART_STOP_BITS_1		|
							ARM_USART_PARITY_NONE		,							
							115200);
	Driver_USART1.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART1.Control(ARM_USART_CONTROL_RX,1);
}

void sendCommand(char * command, int tempo_ms)
{
	int len;
	len = strlen (command);
	Driver_USART1.Send(command,len); // send the read character to the esp8266
	osSignalWait(0x02, osWaitForever);		// sommeil fin emission
	osDelay(tempo_ms);		// attente traitement retour
}

void Init_WiFi(void)
{
	// reset module
	sendCommand("AT+RST\r\n",7000); 
	
	// disconnect from any Access Point
	sendCommand("AT+CWQAP\r\n",2000); 
	
	sendCommand("AT+CWMODE=3\r\n",2000);
	
  // configure as Station 
	sendCommand("AT+CWJAP=\"it2r1\",\"testit2r\"\r\n",7000);
	
	sendCommand("AT+CIFSR\r\n",2000);
	
	//Connect to YOUR Access Point
	sendCommand("AT+CIPSTART=\"TCP\",\"192.168.1.1\",2000\n\r",7000); 
	
	
	
}



