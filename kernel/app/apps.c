#include "FROMA_HEADER.h"

extern xTaskHandle xPrimeTaskHandle;
extern xTaskHandle xShellTaskHandle;
extern xTaskHandle xIDLE1TaskHandle;
extern void vTaskState(const char *pcParameterBuffer);

void vPrimeTask( void *pvParameters ){
	int i, j, isPrime, start, end, core;
	char primeBuf[30];
	portTickType xLastExecutionTime;
	xPRIME xEachPrime;

#if 1
	xEachPrime = *((xPRIME*)pvParameters);
	start = xEachPrime.start;
	end = xEachPrime.end;
	core = xEachPrime.core;
#endif

	for(;;){
		//if(!xPrimeTaskStart)
		//	continue;

		xLastExecutionTime = xTaskGetTickCount();

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
				//sprintf(primeBuf, "%4d\t", i);
				//vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );
			}
		}
		sprintf(primeBuf, "\r\nCore: %d, Time: %4d\r\n", core, (int)(xTaskGetTickCount() - xLastExecutionTime)/1000);
		vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );

		vTaskState(NULL);

		//xPrimeTaskStart = 0;
		if(core==PRIMARY_CPU_ID)
			vTaskResume(xShellTaskHandle, core);

		if(core==SECONDARY_CPU_ID)
			vTaskResume(xIDLE1TaskHandle, core);
			//vTaskSuspend(NULL);
	}
}

void vUARTEchoTask( void *pvParameters ){
	signed char cChar;
	for(;;){
#if 0
		if ( pdTRUE == xSerialGetChar( (xComPortHandle)mainPRINT_PORT, &cChar, portMAX_DELAY ) )
			(void)xSerialPutChar( (xComPortHandle)mainPRINT_PORT, cChar, portMAX_DELAY );
#endif
	}
}
