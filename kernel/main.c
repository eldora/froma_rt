#include "FROMA_HEADER.h"

#define tskIDLE_STACK_SIZE	configMINIMAL_STACK_SIZE

static void prvSetupHardware();
static portBASE_TYPE xCoreStart_0 = pdFALSE;
static portBASE_TYPE xCoreStart_1 = pdFALSE;
extern xTaskHandle xShellTaskHandle;
extern xTaskHandle xPrimeTaskHandle;
extern xTaskHandle xIDLE1TaskHandle;

int secondary_main( void )
{
	xTaskHandle xPrimeTaskHandle = NULL;
	int num = 100000;
	char cAddress[20];
	extern void prvIdleTask( void *pvParameters );
	xPRIME xPrimeM1 = {2, 50000};

	/* waiting init Core0 */
	while(xCoreStart_0 == pdFALSE)
		__asm volatile( "" ::: "memory" );

	prvSetupHardware();

#if 0
	__asm volatile( "dmb" ::: "memory" );
	sprintf( cAddress, "1 shared:%d\t", sharedValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	while(sharedValue == pdFALSE){
		__asm volatile( "" ::: "memory" );
	}

	sprintf( cAddress, "Core: %ld\r\n", 1 );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	sprintf( cAddress, "DEBUG_%d: %d\r\n", 1, 1 );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );
#endif

	xTaskCreate( vPrimeTaskSMP, (const signed char *)"PrimeM1", configMINIMAL_STACK_SIZE, &xPrimeM1, mainCHECK_TASK_PRIORITY-1, NULL );
	xTaskCreate( prvIdleTask, ( signed char * ) "IDLE1", tskIDLE_STACK_SIZE, ( void * ) NULL, mainCHECK_TASK_PRIORITY+1, &xIDLE1TaskHandle );
	//xTaskCreate( vUARTEchoTask, (const signed char *)"UART", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, &xIDLE1TaskHandle );
	//vTaskSuspend(xPrimeTaskHandle);

	/* init & create Task Core1 END */
	__asm volatile( "dsb" ::: "memory" );
	xCoreStart_1 = pdTRUE;

	xPortStartScheduler();
	//vPortStartFirstTask();
	//while(pdTRUE);

	/* Should never reach here. */
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"Should never reach here!\r\n", 26 );

	return 0;
}

int main( void )
{
	int num = 100000;
	char cAddress[50];
	xPRIME xPrimeU0 = {2, 200000};
	xPRIME xPrimeM0 = {2, 100000};

#if 0
	sprintf( cAddress, "0 shared:%d\t", sharedValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	sprintf( cAddress, "Core: %ld\r\n", 0 );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );
#endif

	/*
	sprintf( cAddress, "\r\n\r\n0 shared: %d\r\n", sharedValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );
*/
	// memory barrier using
	/* Initialise the Hardware. */
	prvSetupHardware();

	/* Start the tasks defined within the file. */
	//xTaskCreate( vCheckTask, (const signed char *)"Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );
	xTaskCreate( vPrimeTaskSMP, (const signed char *)"PrimeM0", configMINIMAL_STACK_SIZE, &xPrimeM0, mainCHECK_TASK_PRIORITY-2, NULL );
	xTaskCreate( vPrimeTask, (const signed char *)"PrimeU0", configMINIMAL_STACK_SIZE, &xPrimeU0, mainCHECK_TASK_PRIORITY, &xPrimeTaskHandle );
	//xTaskCreate( vUARTEchoTask, (const signed char *)"EchoTask", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );
	xTaskCreate( vShellTask, (const signed char *)"Shell", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY+2, &xShellTaskHandle);

#if 0
	sprintf(cAddress, "\r\nmain Shell: %s, %s\r\n", ((tskTCB*)xShellTaskHandle)->pcTaskName, ((tskTCB*)xPrimeTaskHandle)->pcTaskName);
	vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)cAddress, strlen(cAddress) );
#endif
	/* init Core0 END */
	xCoreStart_0 = pdTRUE;
	__asm volatile( "dmb" ::: "memory" );

	/* waiting init Core1 */
	while(xCoreStart_1 == pdFALSE)
		__asm volatile( "" ::: "memory" );

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Should never reach here. */
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"Should never reach here!\r\n", 26 );

	/* Will only get here if there was not enough heap space to create the idle task. */
	return 0;
}
/*----------------------------------------------------------------------------*/

void vApplicationTickHook( void )
{
	vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)"Tick\r\n", 6 );
}
/*----------------------------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	signed char cChar;
	if ( pdTRUE == xSerialGetChar( (xComPortHandle)mainPRINT_PORT, &cChar, 0UL ) )
	{
		(void)xSerialPutChar( (xComPortHandle)mainPRINT_PORT, cChar, 0UL );
	}
}
/*----------------------------------------------------------------------------*/

static void prvSetupHardware( void )
{
	unsigned long ulVector = 0UL;

	portDISABLE_INTERRUPTS();

	/* Install the Spurious Interrupt Handler to help catch interrupts. */
	extern void vPortUnknownInterruptHandler( void *pvParameter );
	extern void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, 
			unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets );
	for ( ulVector = 0; ulVector < portMAX_VECTORS; ulVector++ )
		vPortInstallInterruptHandler( vPortUnknownInterruptHandler, (void *)ulVector, ulVector, pdTRUE, configMAX_SYSCALL_INTERRUPT_PRIORITY, portCORE_ID() );

#if 0
	extern void vUARTInitialise(unsigned long ulUARTPeripheral, unsigned long ulBaud, unsigned long ulQueueSize );
	if(portCORE_ID()==PRIMARY_CPU_ID)
		vUARTInitialise( mainPRINT_PORT, mainPRINT_BAUDRATE, 64 );
#endif
	/* Perform any other peripheral configuration. */
}
/*----------------------------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	__asm volatile (" smc #0 ");
}
/*----------------------------------------------------------------------------*/

extern void vAssertCalled( char *file, int line )
{
	printf("Assertion failed at %s, line %d\n\r",file,line);
	taskDISABLE_INTERRUPTS();
	for( ;; );
}

