#ifndef APPS_H
#define APPS_H

typedef struct{
	int start;
	int end;
}xPRIME;

/*  */
void vPrimeTaskSMP(void *pvParameters);
void vPrimeTask(void *pvParameters);
void vUARTEchoTask(void *pvParameters);

/* Periodically checks to see whether the demo tasks are still running. */

#endif
