#include "FROMA_HEADER.h"

#define COMMAND_BUFFER_SIZE		100

xTaskHandle xShellTaskHandle;
xTaskHandle xPrimeTaskHandle;
xTaskHandle xIDLE1TaskHandle;
extern PRIVILEGED_DATA tskTCB * volatile pxCurrentTCB[portNUM_PROCESSORS];

struct shellCommandEntry gsCommandTable[] = {
	{"help", "\t- View available commands and their description.", vHelp},
	{"version", "\t- Show the version information of the OS.", vVersion},
	{"clear", "\t- Clear Screen.", vClear},
	{"state", "\t- View Tasks State.", vTaskState},
	{"prime", "\t- Search Prime Number.", vPrime},
	{"primeSMP", "- Search Prime Number using Multi-Core", vPrimeSMP},
};

void vShellTask( void *pvParameters ){
	portCHAR vcCommandBuffer[COMMAND_BUFFER_SIZE];
	portBASE_TYPE xCommandBufferIndex = 0;
	signed portCHAR cInputKey;

	vTaskState(NULL);
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"FROMA$ ", 7);
	while(1){
		if(pdTRUE == xSerialGetChar((xComPortHandle)mainPRINT_PORT, &cInputKey, portMAX_DELAY)){

			switch(cInputKey){
				case KEY_BACKSPACE:
					if(xCommandBufferIndex > 0){
						xSerialPutChar((xComPortHandle)mainPRINT_PORT, cInputKey, portMAX_DELAY);
						xSerialPutChar((xComPortHandle)mainPRINT_PORT, ' ', portMAX_DELAY);
						xSerialPutChar((xComPortHandle)mainPRINT_PORT, cInputKey, portMAX_DELAY);
						xCommandBufferIndex--;
					}
					break;
				case KEY_ENTER:
					if(xCommandBufferIndex > 0){
						vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"\r\n", strlen("\r\n"));
						vcCommandBuffer[xCommandBufferIndex] = '\0';
						vExecuteCommand(vcCommandBuffer);
					}
					memset(vcCommandBuffer, 0, COMMAND_BUFFER_SIZE);
					xCommandBufferIndex = 0;
					break;

				default:
					if(xCommandBufferIndex < COMMAND_BUFFER_SIZE - 1){			// -1 meaning: SECURE ENTER BUFFER SPACE 
						vcCommandBuffer[xCommandBufferIndex++] = cInputKey;
						xSerialPutChar((xComPortHandle)mainPRINT_PORT, cInputKey, portMAX_DELAY);
					}
					else
						vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"Command Buffer is Full\r\n", strlen("Command Buffer is Full\r\n"));
					break;
			}
		}
	}
}

void vExecuteCommand(const char *pcCommandBuffer){
	int i, iCommandTypeNumber;
	char *pcCommand;
	char cTempBuffer[COMMAND_BUFFER_SIZE+20];

	iCommandTypeNumber = sizeof(gsCommandTable)/sizeof(struct shellCommandEntry);

	pcCommand = strtok((char *)pcCommandBuffer, " ");
	for(i=0; i<iCommandTypeNumber; i++){
		if(!strcmp(pcCommand, gsCommandTable[i].pcCommand)){
			gsCommandTable[i].pvFunction(strtok(NULL, ""));
			break;
		}
	}
	if(i>=iCommandTypeNumber){
		sprintf(cTempBuffer, "%s: command not found\r\n", pcCommandBuffer);
		vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));
	}
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"FROMA$ ", 7);
}

void vHelp(const char *pcParameterBuffer){
	int i, iCommandTypeNumber;

	iCommandTypeNumber = sizeof(gsCommandTable)/sizeof(struct shellCommandEntry);
	
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"=== Shell of FROMA Help ===\r\n", strlen("=== Shell of FROMA Help ===\r\n"));
	for(i=0; i<iCommandTypeNumber; i++){
		vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)gsCommandTable[i].pcCommand, strlen(gsCommandTable[i].pcCommand));
		vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)gsCommandTable[i].pcHelp, strlen(gsCommandTable[i].pcHelp));
		vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"\r\n", strlen("\r\n"));
	}
}

void vClear(const char *pcParameterBuffer){
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"\033[2J", strlen("\033[2J"));
}

void vVersion(const char *pcParameterBuffer){
	portCHAR cTempBuffer[50];
	portCHAR *pcVersion = "2.17";

	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"Distributor ID:\tFROMA\r\n", strlen("Distributor ID:\tFROMA\r\n"));
	sprintf(cTempBuffer, "Description:\tFROMA %s SMP Supported\r\n", pcVersion);
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));
	sprintf(cTempBuffer, "Release:\t%s\r\n", pcVersion);
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"Codename:\trudder\r\n", strlen("Codename:\trudder\r\n"));
}

void vPrimeSMP(const char *pcParameterBuffer){
#if 0
	portCHAR cTempBuffer[30];
	portCHAR ch, type;
	portINT index;
	type = (portCHAR)index = 0;
	while(ch=pcParameterBuffer[index++]){	
		switch(ch){
			case '2': type = 1; break;
			default: break;
		}
	}
#endif
	// if type == 1 then execute SMP prime task, else then execute UP prime task
	if(pdTRUE){
		vTaskSuspend(xPrimeTaskHandle, PRIMARY_CPU_ID);
		vTaskSuspend(xIDLE1TaskHandle, SECONDARY_CPU_ID);
		vTaskSuspend(xShellTaskHandle, PRIMARY_CPU_ID);
		vTaskResume(xPrimeTaskHandle, PRIMARY_CPU_ID);
	}
#if 0
	else{
		vTaskSuspend(xShellTaskHandle, PRIMARY_CPU_ID);
	}
#endif
}

void vPrime(const char *pcParameterBuffer){
	vTaskSuspend(xShellTaskHandle, PRIMARY_CPU_ID);
}

void vTaskState(const char *pcParameterBuffer){
	portBASE_TYPE xNumberOfTasks;
	portCHAR cTempBuffer[300];

	xNumberOfTasks = uxTaskGetNumberOfTasks();

	sprintf(cTempBuffer, "CPU[0] pxTCB: %s\t CPU[1] pxTCB: %s\r\n", pxCurrentTCB[0]->pcTaskName, pxCurrentTCB[1]->pcTaskName);
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));

	sprintf(cTempBuffer, "Number Of Tasks: %u\r\n", (unsigned int)xNumberOfTasks);
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));

	sprintf(cTempBuffer, "Task Name\tStatus\tPrority\tStack\tNumber");
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));

	vTaskList((signed char *)cTempBuffer);
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)cTempBuffer, strlen(cTempBuffer));
}
