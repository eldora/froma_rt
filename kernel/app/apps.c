#include "FROMA_HEADER.h"

xTaskHandle xPrimeTaskHandle;
extern xTaskHandle xShellTaskHandle;
extern void vTaskState(const char *pcParameterBuffer);

void vPrimeTask( void *pvParameters ){
	int i, j, isPrime, end;
	char primeBuf[30];
	portTickType xLastExecutionTime;

	end = *((int*)pvParameters);

	for(;;){
		//if(!xPrimeTaskStart)
		//	continue;

		xLastExecutionTime = xTaskGetTickCount();

		for(i=2; i<=end; i++){
			if(i%2==0 && i!=2)
				continue;
			isPrime = 1;
			for(j=2; j*j<=i; j++){
				if(i%j==0){
					isPrime=0;
					break;
				}
			}
			if(isPrime){
				sprintf(primeBuf, "%4d\t", i);
				vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );
			}
		}
		sprintf(primeBuf, "\r\nTime: %4d\r\n", (int)(xTaskGetTickCount() - xLastExecutionTime)/1000);
		vSerialPutString( (xComPortHandle)mainPRINT_PORT, (const signed char * const)primeBuf, strlen(primeBuf) );

		//xPrimeTaskStart = 0;
		vTaskResume(xShellTaskHandle);
	}
}

void vUARTEchoTask( void *pvParameters ){
	signed char cChar;
	for(;;){
		if ( pdTRUE == xSerialGetChar( (xComPortHandle)mainPRINT_PORT, &cChar, portMAX_DELAY ) )
			(void)xSerialPutChar( (xComPortHandle)mainPRINT_PORT, cChar, portMAX_DELAY );
	}
}
