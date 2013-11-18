#include "FROMA_HEADER.h"

#define ENTER_TERM		10

extern xTaskHandle xPrimeTaskHandle;
extern xTaskHandle xShellTaskHandle;
extern xTaskHandle xIDLE1TaskHandle;
extern void vTaskState(const char *pcParameterBuffer);

void vPrimeTaskSMP( void *pvParameters ){
	portINT i, j, isPrime, start, end, core, enter;
	portCHAR primeBuf[30];
	portTickType xLastExecutionTime;
	xPRIME xEachPrime;

#if 1
	xEachPrime = *((xPRIME*)pvParameters);
	start = xEachPrime.start;
	end = xEachPrime.end;
#endif

	for(;;){
		core = portCORE_ID();
		xLastExecutionTime = xTaskGetTickCount();
		enter=0;

		for(i=start; i<=end; i++){
			if(i%2==0 && i!=2)
				continue;
			isPrime = 1;
			for(j=start; j*j<=i; j++){
				if(i%j==0){
					isPrime=0;
					break;
				}
			}
			if(isPrime){
#if 0
				if(!(enter++%ENTER_TERM)) vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)"\r\n", 2 );
				sprintf(primeBuf, "%5d\t", i);
				vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );
#endif
			}
		}
		//sprintf(primeBuf, "\r\nCore: %d, Time: %4d\r\n", core, (int)(xTaskGetTickCount() - xLastExecutionTime)/1000);
		sprintf(primeBuf, "\r\nCore: %d, Search Complete!!\r\n", core);
		vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );

		vTaskState(NULL);

		if(core==PRIMARY_CPU_ID)
			vTaskResume(xShellTaskHandle, core);

		if(core==SECONDARY_CPU_ID)
			vTaskResume(xIDLE1TaskHandle, core);
	}
}

void vPrimeTask( void *pvParameters ){
	portINT i, j, isPrime, start, end, core, enter;
	portCHAR primeBuf[30];
	portTickType xLastExecutionTime;
	xPRIME xEachPrime;

#if 1
	xEachPrime = *((xPRIME*)pvParameters);
	start = xEachPrime.start;
	end = xEachPrime.end;
#endif

	for(;;){
		core = portCORE_ID();
		xLastExecutionTime = xTaskGetTickCount();
		enter=0;

		for(i=start; i<=end; i++){
			if(i%2==0 && i!=2)
				continue;
			isPrime = 1;
			for(j=start; j*j<=i; j++){
				if(i%j==0){
					isPrime=0;
					break;
				}
			}
			if(isPrime){
#if 0
				if(!(enter++%ENTER_TERM)) vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)"\r\n", 2 );
				sprintf(primeBuf, "%5d\t", i);
				vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );
#endif
			}
		}
		//sprintf(primeBuf, "\r\nCore: %d, Time: %4d\r\n", core, (int)(xTaskGetTickCount() - xLastExecutionTime)/1000);
		sprintf(primeBuf, "\r\nCore: %d, Search Complete!!\r\n", core);
		vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );

		vTaskState(NULL);

		vTaskResume(xShellTaskHandle, core);
	}
}


void vUARTEchoTask( void *pvParameters ){
	signed portCHAR cChar;
	for(;;){
#if 0
		if ( pdTRUE == xSerialGetChar( (xComPortHandle)mainPRINT_PORT, &cChar, portMAX_DELAY ) )
			(void)xSerialPutChar( (xComPortHandle)mainPRINT_PORT, cChar, portMAX_DELAY );
#endif
	}
}
