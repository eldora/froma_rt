#include "FROMA_HEADER.h"

#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 255
#endif

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR			( 0x0000001F )

//void vSerialPutString(int,const signed char * cons,int);

// Interrupt Handler
typedef struct STRUCT_HANDLER_PARAMETER
{
	void (*vHandler)(void *);
    void *pvParameter;
} xInterruptHandlerDefinition;
xInterruptHandlerDefinition pxInterruptHandlers[ portNUM_PROCESSORS ][ portMAX_VECTORS ] = { { NULL, NULL } };

extern unsigned portBASE_TYPE * volatile pxCurrentTCB[ portNUM_PROCESSORS ];
static volatile spinlock_t portYieldLock = {0,};

// GIC Interrupt MAX Vector ID 
static unsigned long ulMaxVectorId = portMAX_VECTORS;

// Each Mode Stack Space
static portSTACK_TYPE puxFIQStack[ portNUM_PROCESSORS ][ portFIQ_STACK_SIZE ];
static portSTACK_TYPE puxIRQStack[ portNUM_PROCESSORS ][ portIRQ_STACK_SIZE ];
static portSTACK_TYPE puxAbortStack[ portNUM_PROCESSORS ][ portABORT_STACK_SIZE ];
static portSTACK_TYPE puxSVCStack[ portNUM_PROCESSORS ][ portSVC_STACK_SIZE ];

// Page Table
static unsigned long PageTable[4096] __attribute__((aligned (16384)));

// Tick Interrupt
static void prvSetupTimerInterrupt( void );

// Exception handlers
void vPortPendSVHandler( void *pvParameter ) __attribute__((naked));
void vPortSysTickHandler( void *pvParameter );
void vPortSVCHandler( void ) __attribute__ (( naked ));
void vPortInterruptContext( void ) __attribute__ (( naked ));
void vPortSMCHandler( void ) __attribute__ (( naked ));

// Start First Task
void vPortStartFirstTask( void ) __attribute__ (( naked ));

// Interrupt Handler code
void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets );

// Linker defined variables
extern unsigned long _etext;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _bss;
extern unsigned long _ebss;
extern unsigned long _stack_top;

char *itoa( int iIn, char *pcBuffer )
{
char *pcReturn = pcBuffer;
int iDivisor = 0;
int iResult = 0;
	for ( iDivisor = 100; iDivisor > 0; iDivisor /= 10 )
	{
		iResult = ( iIn / iDivisor ) % 10;
		*pcBuffer++ = (char)iResult + '0';
	}
	return pcReturn;
}

//See header file for description.
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
	portSTACK_TYPE *pxOriginalStack = pxTopOfStack;

	// Align top of stack
	pxTopOfStack = (portSTACK_TYPE *)((unsigned long)(pxTopOfStack)&~portBYTE_ALIGNMENT_MASK);
	/* Simulate the stack frame as it would be created by a context switch
	interrupt. */
	*pxTopOfStack = portINITIAL_XPSR;										/* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode;					/* PC */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0;								/* LR */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) pxOriginalStack;	/* SP */
	pxTopOfStack -= 12;																	/* R1~R12 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters;		/* R0 */

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void vPortSVCHandler( void )
{
	volatile unsigned int currentCORE_ID = portCORE_ID();
	__asm volatile(
			" ldr r9, pxCurrentTCBConst2	\n"		// Load pxCurrentTCB Pointer Address
			" ldr r8, [r9, %[portCORE], lsl #2]								\n"		// Load pxCurrentTCB Address 
			" ldr lr, [r8]								\n"		// lr = Task Stack Pointer(pxTopOfStack)
			" ldmia lr, {r0-lr}^				 	\n"		// Load Task's registers
			" add lr, lr, #60						 	\n"		// Re-adjust the stack for the Task Context
			" nop						 							\n"
			" rfeia lr								 		\n"		// Return from exception, PC and CPSR from Task Stack
																					// RFEIA: REF(return from exception), IA(after transfer, increse address: use descending stack
																					// ARM_Assembly_Description_v4.0_kor.pdf - 156p
			" nop						 							\n"
			"															\n"
			"	.align 2										\n"
			" pxCurrentTCBConst2: .word pxCurrentTCB 	\n"
			: : [portCORE] "r" (currentCORE_ID) : "memory"
			);
}
/*-----------------------------------------------------------*/

void vPortInterruptContext( void )
{
	volatile unsigned int currentCORE_ID = portCORE_ID();
#if 0
	volatile tskTCB * pxTCBAddr;
	char cAddress[50];
	__asm volatile(
			" ldr r9, pxCurrentTCBConst2	\n"		/* Load the pxCurrentTCB pointer address. */
			" ldr r8, [r9, %[portCORE], lsl #2]								\n"		// Load pxCurrentTCB Address 
			" UART_SEND:	\n"
			" ldr r1, =0x48020014	\n"
			" ldr r2, [r1]	\n"
			" and	r2, r2, #0x20	\n"
			" cmp r2, #0x00	\n"
			" beq UART_SEND	\n"
			" ldr r1, =0x48020000	\n"
//			" mov r2, r8	\n"
			" mov r2, %[portCORE]	\n"
			" add r2, r2, #48 \n"
			" mov %[saveAddr], r8	\n"
			" str r2, [r1]	\n"
			: [saveAddr] "=r" (pxTCBAddr)  : [portCORE] "r" (currentCORE_ID) : "memory"
			);

	//sprintf(cAddress, "\r\n Core%d: %s\r\n", currentCORE_ID, ((tskTCB*)pxTCBAddr)->pcTaskName);
	//vSerialPutString( (xComPortHandle)2, (const signed char * const)cAddress, strlen(cAddress) );
#endif
#if 1
	__asm volatile(
			/*
			" UART_SEND:	\n"
			" ldr r1, =0x48020014	\n"
			" ldr r2, [r1]	\n"
			" and	r2, r2, #0x20	\n"
			" cmp r2, #0x00	\n"
			" beq UART_SEND	\n"
			" ldr r1, =0x48020000	\n"
			" mov r2, %[portCORE]	\n"
			" add r2, r2, #56 \n"
			" str r2, [r1]	\n"
*/

/*    ## Description ##
 *
 *    ARM, C, C++ 컴파일러에서는 항상 전체 내림차순 스택을 사용한다. PUSH, POP 명령어는 전체 내림차순 스택을 가정함.
 *    LDM 및 STM을 통한 스택 구현: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204hk/Cacbgchh.html
 *
 *    UserMode의 cpsr이 전환되는 Exception Mode에 spsr에 자동 저작이 되고 이어서,
 *    UserMode의 pc가 전환되는 Exception Mode의 lr에 자동 저장이 됨.
 *
			" sub lr, lr, #4				\n"		// lr-=4: ?
			" srsdb SP, #31					\n"		// srs(스택에 반환 상태(lr) 저장) db(전송 전에 주소를 증가시킨다)
																		// #31(System)Mode 스텍에 현재 lr레지스터 값을 저장한다
																		// SRS는 현재 모드의 lr과 SPSR을 modenum에 의해 지정된 모드의 sp에 들어 있는 주소와 그 다음 워드에 각각 저장하고, 
																		// 경우에 따라 sp의 주소를 업데이트함
																		// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204ik/Cihfdedi.html
			" stmdb SP, {SP}^			 	\n"		// stm(다중 레지스터 저장) db(전송 전에 주소를 감소시킨다) ^(현재 모드 대신 사용자 모드 레지스터 내부 또는 외부 데이터를 전송)
																		// User SP 값을 SP주소에 저장
			" sub SP, SP, #4			 	\n"		// sp-=4: ?
			" ldmia SP!, {lr}			 	\n"		// SP의 주소에 lr값을 로드 !(최종 주소가 SP에 다시 기록), ia(각 전송 후에 주소를 증가시킴)
																		// 현재 lr에는 UserMode의 PC값이 들어가 있다.
			" sub LR, LR, #8 				\n"		// lr-=8: 
			" stmdb LR, {r0-lr}^ 		\n"		// 
			" sub LR, LR, #60		 		\n"		// lr-=60: r0-r14(lr)의 15개의 레지스터 값만큼 주소변경을 해야 하니 15*4 = 60임
	
			" ldr r9, pxCurrentTCBConst2	\n"
//			" ldr r8, [r9]					\n"
			" ldr r8, [r9, %[portCORE], lsl #2]								\n"
			" str lr, [r8]	 				\n"
			" bl vPortGICInterruptHandler	\n"
//			" ldr r8, [r9]					\n"
			" ldr r8, [r9, %[portCORE], lsl #2]								\n"
			" ldr lr, [r8]					\n"	
			" ldmia lr, {r0-lr}^	 	\n"		// lr의 주소를 참조하여 그 위치로부터 pop하면서 UserMode의 r0-lr의 레지스터에 복구
			" add lr, lr, #60			 	\n"		
			" rfeia lr				 			\n"  // RFE(예외에서 복귀), IA(전송후에 주소를 증가시킴(내림차순 스택))
																	 // LR에 들어있는 주소와 그 다음 주소에서 PC와 CPSR를 로드
																	 // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204ik/Cihjacag.html
			" nop						 				\n"
*/
			" sub lr, lr, #4				\n"		/* Adjust the return address. */
			" srsdb SP, #31					\n"		/* Store the return address and SPSR to the Task's stack. */
			" stmdb SP, {SP}^			 	\n"		/* Store the SP_USR to the stack. */
			" sub SP, SP, #4			 	\n"		/* Decrement the Stack Pointer. */
			" ldmia SP!, {lr}			 	\n"		/* Load the SP_USR into LR. */
			" sub LR, LR, #8 				\n"		/* Make room for the previously stored LR and CPSR. */
			" stmdb LR, {r0-lr}^ 		\n"		/* Store the Task's registers. */
			" sub LR, LR, #60		 		\n"		/* Adjust the Task's stack pointer. */
			" ldr r9, pxCurrentTCBConst2	\n"		/* Load the pxCurrentTCB pointer address. */
//			" ldr r8, [r9]					\n"		/* Load the pxCurrentTCB address. */
			" ldr r8, [r9, %[portCORE], lsl #2]								\n"		// Load pxCurrentTCB Address 
			" str lr, [r8]	 				\n"		/* Store the Task stack pointer to the TCB. */
			" bl vPortGICInterruptHandler	\n"		/* Branch and link to find specific service handler. */
//			" ldr r8, [r9]					\n"		/* Load the pxCurrentTCB address. */
			" ldr r8, [r9, %[portCORE], lsl #2]								\n"		// Load pxCurrentTCB Address 
			" ldr lr, [r8]					\n"		/* Load the Task Stack Pointer into LR. */
			" ldmia lr, {r0-lr}^	 	\n"		/* Load the Task's registers. */
			" add lr, lr, #60			 	\n"		/* Re-adjust the stack for the Task Context */
			" rfeia lr				 			\n"		/* Return from exception by loading the PC and CPSR from Task Stack. */
			" nop						 				\n"
			: : [portCORE] "r" (currentCORE_ID) : "memory"
			);
#endif
}
/*-----------------------------------------------------------*/

void vPortStartFirstTask( void )
{
	volatile unsigned int currentCORE_ID = portCORE_ID();
	__asm volatile(
					" mov SP, %[svcsp]			\n" /* Set-up the supervisor stack. */
					" svc 0 					\n" /* Use the supervisor call to be in an exception. */
					" nop						\n"
					: : [pxTCB] "r" (pxCurrentTCB[ currentCORE_ID ]), [svcsp] "r" (&(puxSVCStack[ currentCORE_ID ][ portFIQ_STACK_SIZE - 1 ] )) :
				);
}
/*-----------------------------------------------------------*/

void vPortSMCHandler( void )
{
	/* Nothing to do. */
}
/*-----------------------------------------------------------*/

void vPortYieldFromISR( void )
{
	//__spin_lock(&portYieldLock);
	vTaskSwitchContext();
	portSGI_CLEAR_YIELD( portGIC_DISTRIBUTOR_BASE, portCORE_ID() );
	//__spin_lock(&portYieldLock);
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
portBASE_TYPE xPortStartScheduler( void )
{
	/* Start the timer that generates the tick ISR.  Interrupts are disabled here already. */
#if 1
	if(portCORE_ID()==SECONDARY_CPU_ID)
		prvSetupTimerInterrupt();
#endif
	/* Install the interrupt handler. */
	vPortInstallInterruptHandler( (void (*)(void *))vPortYieldFromISR, NULL, portSGI_YIELD_VECTOR_ID, pdTRUE, 
			/* configMAX_SYSCALL_INTERRUPT_PRIORITY */ configKERNEL_INTERRUPT_PRIORITY, 1<<portCORE_ID() );

	/* Finally, allow the GIC to pass interrupts to the processor. */
	portGIC_WRITE( portGIC_ICDDCR(portGIC_DISTRIBUTOR_BASE), 0x01UL );
	portGIC_WRITE( portGIC_ICCBPR(portGIC_PRIVATE_BASE), 0x00UL );
	portGIC_WRITE( portGIC_ICCPMR(portGIC_PRIVATE_BASE), configLOWEST_INTERRUPT_PRIORITY );
	portGIC_WRITE( portGIC_ICCICR(portGIC_PRIVATE_BASE), 0x01UL );

	/* Start the first task. */
	vPortStartFirstTask();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/


void vPortEndScheduler( void )
{
	/* It is unlikely that the CM3 port will require this function as there
	is nothing to return to.  */
}
/*-----------------------------------------------------------*/

void vPortSysTickHandler( void *pvParameter )
{
	/* Clear the Interrupt. */
	*(portSYSTICK_INTERRUPT_STATUS) = 0x01UL;

	//vTaskIncrementTick(portCORE_ID());
	vTaskIncrementTick(0);

#if 0
#if configUSE_PREEMPTION == 1
	/* If using preemption, also force a context switch. */
	vTaskSwitchContext();
	portEND_SWITCHING_ISR(pdTRUE);
#endif
#endif
}
/*-----------------------------------------------------------*/

void vPortUART3Handler( void *pvParameter )
{
	/* Clear the Interrupt. */
	vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"UART Interrupt\r\n", strlen("UART Interrupt\r\n"));

#if configUSE_PREEMPTION == 1
	/* If using preemption, also force a context switch. */
	vTaskSwitchContext();
	portEND_SWITCHING_ISR(pdTRUE);
#endif
}
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
void prvSetupTimerInterrupt( void )
{
	/* Install the interrupt handler. */
	vPortInstallInterruptHandler( vPortSysTickHandler, NULL, portSYSTICK_VECTOR_ID, pdTRUE, 
			/* configMAX_SYSCALL_INTERRUPT_PRIORITY */ configKERNEL_INTERRUPT_PRIORITY, 1<<portCORE_ID() );

	/* Configure SysTick to interrupt at the requested rate. */
	*(portSYSTICK_LOAD) = (configCPU_PERIPH_HZ / (portSYSTICK_PRESCALE+1) / configTICK_RATE_HZ ) - 1UL;
	*(portSYSTICK_CONTROL) = ( portSYSTICK_PRESCALE << 8 ) | portSYSTICK_CTRL_ENABLE_PERIODIC_INTERRUPTS;
}
/*-----------------------------------------------------------*/

/* UART Install Function */
void prvSetupUARTInterrupt( void ){
	vPortInstallInterruptHandler( vPortUART3Handler, NULL, portUART3_VECTOR_ID, pdTRUE, /* configMAX_SYSCALL_INTERRUPT_PRIORITY */ configKERNEL_INTERRUPT_PRIORITY, 1<<portCORE_ID() );
}

void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets )
{
unsigned long ulBank32 = 4 * ( ulVector / 32 );
unsigned long ulOffset32 = ulVector % 32;
unsigned long ulBank4 = 4 * ( ulVector / 4 );
unsigned long ulOffset4 = ulVector % 4;
unsigned long ulBank16 = 4 * ( ulVector / 16 );
unsigned long ulOffset16 = ulVector % 16;
/* unsigned long puxGICAddress = 0; */
unsigned long puxGICDistributorAddress = 0;

	/* Select which GIC to use. */
	if ( ulVector < portMAX_VECTORS )
	{
		/* puxGICAddress = portGIC_PRIVATE_BASE; */
		puxGICDistributorAddress = portGIC_DISTRIBUTOR_BASE;
	}
	else
	{
		/* Shouldn't get here. */
	}

	/* Record the Handler. */
	if (ulVector < ulMaxVectorId )
	{
		pxInterruptHandlers[ portCORE_ID() ][ ulVector ].vHandler = vHandler;
		pxInterruptHandlers[ portCORE_ID() ][ ulVector ].pvParameter = pvParameter;

		/* Now calculate all of the offsets for the specific GIC. */
		/*
		 * Cortex-A9 GIC
		 * BankX: 하나의 인터럽트는 (32/X)-bit만큼 공간을 사용한다. 4를 곱해준 이유는 주소체계가 4Byte단위이기 때문에(Register에선 31-0까지의 비트주소를 사용)
		 * OffsetX: Bank로 나눈 단위에서 OffsetX는 해당 인터럽트의 위치를 가리키고, BankX와 OffsetX를 세트로 사용
		 */
		ulBank32 = 4 * ( ulVector / 32 );
		ulOffset32 = ulVector % 32;
		ulBank4 = 4 * ( ulVector / 4 );
		ulOffset4 = ulVector % 4;
		ulBank16 = 4 * ( ulVector / 16 );
		ulOffset16 = ulVector % 16;

		/* First make the Interrupt a Secure one. */
		//	*(portGIC_ICDISR_BASE(puxGICDistributorAddress) + ulBank32) &= ~( 1 << ulOffset32 );

		/* Is it Edge Triggered?. */
		if ( 0 != ucEdgeTriggered )
		{
			portGIC_SET( (portGIC_ICDICR_BASE(puxGICDistributorAddress + ulBank16) ), 
					( portGIC_READ(puxGICDistributorAddress + ulBank16) | ( 0x02 << ( ulOffset16 * 2 ) ) ) );
		}

		/* Set the Priority. */
		portGIC_WRITE( portGIC_ICDIPR_BASE(puxGICDistributorAddress) + ulBank4, ( ( (unsigned long)ucPriority ) << ( ulOffset4 * 8 ) ) );

		/* Set the targeted Processors. */
		portGIC_WRITE( portGIC_ICDIPTR_BASE(puxGICDistributorAddress + ulBank4), ( ( (unsigned long)ucProcessorTargets ) << ( ulOffset4 * 8 ) ) );

		/* Enable the Interrupt. */
		if ( NULL != vHandler ){
			portGIC_SET( portGIC_ICDICPR_BASE(puxGICDistributorAddress + ulBank32), 
					( portGIC_READ( portGIC_ICDICPR_BASE(puxGICDistributorAddress + ulBank32) ) | ( 1 << ulOffset32 ) ) );
			portGIC_SET( portGIC_ICDISER_BASE(puxGICDistributorAddress + ulBank32), 
					( portGIC_READ( portGIC_ICDISER_BASE(puxGICDistributorAddress + ulBank32) ) | ( 1 << ulOffset32 ) ) );
		}
		else{
			/* Or disable when passed a NULL handler. */
			portGIC_CLEAR( portGIC_ICDICPR_BASE(puxGICDistributorAddress + ulBank32), 
					( portGIC_READ( portGIC_ICDICPR_BASE(puxGICDistributorAddress + ulBank32) ) | ( 1 << ulOffset32 ) ) );
			portGIC_CLEAR( portGIC_ICDISER_BASE(puxGICDistributorAddress + ulBank32), 
					( portGIC_READ( portGIC_ICDISER_BASE(puxGICDistributorAddress + ulBank32) ) | ( 1 << ulOffset32 ) ) );
		}
	}
}
/*----------------------------------------------------------------------------*/

void vPortGICInterruptHandler( void )
{
unsigned long ulVector = 0UL;
unsigned long ulGICBaseAddress = portGIC_PRIVATE_BASE;

	/* Query the private address first. */
	ulVector = portGIC_READ( portGIC_ICCIAR(portGIC_PRIVATE_BASE) );

	if ( ( ( ulVector & portGIC_VECTOR_MASK ) < ulMaxVectorId ) && ( NULL != pxInterruptHandlers[ portCORE_ID() ][ ( ulVector & portGIC_VECTOR_MASK ) ].vHandler ) )
	{
		/* Call the associated handler. */
		pxInterruptHandlers[ portCORE_ID() ][ ( ulVector & portGIC_VECTOR_MASK ) ].vHandler( pxInterruptHandlers[ portCORE_ID() ][ ( ulVector & portGIC_VECTOR_MASK ) ].pvParameter );
		/* And acknowledge the interrupt. */
		portGIC_WRITE( portGIC_ICCEOIR(ulGICBaseAddress), ulVector );
	}
	else
	{
		/* This is a spurious interrupt, do nothing. */
	}
}
/*----------------------------------------------------------------------------*/

portBASE_TYPE xPortSetInterruptMask( void )
{
portBASE_TYPE xPriorityMask = portGIC_READ( portGIC_ICCPMR(portGIC_PRIVATE_BASE) );
	portGIC_WRITE( portGIC_ICCPMR(portGIC_PRIVATE_BASE), configMAX_SYSCALL_INTERRUPT_PRIORITY );
	return xPriorityMask;
}
/*----------------------------------------------------------------------------*/

void vPortClearInterruptMask( portBASE_TYPE xPriorityMask )
{
	portGIC_WRITE( portGIC_ICCPMR(portGIC_PRIVATE_BASE), xPriorityMask );
}
/*----------------------------------------------------------------------------*/

void vPortUnknownInterruptHandler( void *pvParameter )
{
	/* This is an unhandled interrupt, do nothing. */
	char cAddress[30];
	sprintf( cAddress, "Core-%d: call UnknownInterruptHandler\r\n", portCORE_ID() );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );
	(void)pvParameter;
}
/*----------------------------------------------------------------------------*/


static unsigned long ReadSCTLR()
{
	unsigned long SCTLR;
	__asm volatile("mrc p15, 0, %[sctlr], c1, c0, 0"
	:[sctlr] "=r" (SCTLR)::);
	return SCTLR;
}

static void WriteSCTLR(unsigned long SCTLR)
{
	__asm volatile("mcr p15, 0, %[sctlr], c1, c0, 0"
	::[sctlr] "r" (SCTLR):);
}

void _init(void)
{
extern int main( void );
extern int secondary_main( void );
int i;
unsigned long *pulSrc, *pulDest;
volatile unsigned int currentCORE_ID;
volatile unsigned long ulSCTLR = 0UL;
//unsigned long ulValue = 0UL;
extern unsigned long __isr_vector_start;
extern unsigned long __isr_vector_end;
extern unsigned long _bss;
extern unsigned long _ebss;

	currentCORE_ID = portCORE_ID();

	// Fill Zero, bss segment(Uninitialized data segment): Non-init Global Variable
	for(pulDest = &_bss; pulDest < &_ebss; )
		*pulDest++ = 0;

	// Config Stack Pointer for the Processor Modes
	__asm volatile (
			" cps #17							\n"
			" nop 								\n"
			" mov SP, %[fiqsp]					\n"
			" nop 								\n"
			" cps #18							\n"
			" nop 								\n"
			" mov SP, %[irqsp]					\n"
			" nop 								\n"
			" cps #23							\n"
			" nop 								\n"
			" mov SP, %[abtsp]					\n"
			" nop 								\n"
			" cps #27							\n"
			" nop 								\n"
			" mov SP, %[abtsp]					\n"
			" nop 								\n"
			" cps #19							\n"
			" nop 								\n"
			" mov SP, %[svcsp]					\n"
			" nop 								\n"
			: : [fiqsp] "r" (&(puxFIQStack[ currentCORE_ID ][ portFIQ_STACK_SIZE - 1 ] )),
				[irqsp] "r" (&(puxIRQStack[ currentCORE_ID ][ portFIQ_STACK_SIZE - 1 ] )),
				[abtsp] "r" (&(puxAbortStack[ currentCORE_ID ][ portFIQ_STACK_SIZE - 1 ] )),
				[svcsp] "r" (&(puxSVCStack[ currentCORE_ID ][ portFIQ_STACK_SIZE - 1 ] ))
				:  );

	// Copy the exception vector table over the boot loader
	pulSrc = (unsigned long *)&__isr_vector_start;
	pulDest = (unsigned long *)portEXCEPTION_VECTORS_BASE;						// Difference Each core
	for ( pulSrc = &__isr_vector_start; pulSrc < &__isr_vector_end; )
		*pulDest++ = *pulSrc++;

	// VBAR is modified to point to the new Vector Table
	pulDest = (unsigned long *)portEXCEPTION_VECTORS_BASE;
	__asm volatile(
			" mcr p15, 0, %[vbar], c12, c0, 0 			\n"
			: : [vbar] "r" (pulDest) :
			);

	// Modify the SCTLR to change the Vector Table Addres
	ulSCTLR=ReadSCTLR();
	ulSCTLR&=~(1<<13);
	WriteSCTLR(ulSCTLR);

	// Enable branch prediction
	// 512 entries per core + global history buffer (GHB) of 4096 2-bit predictors + 8-entry return stack
	// OMAP4460_Maunal.pdf - 1099p
	// http://www.slideshare.net/seedyoon/cpu-13-14
	WriteSCTLR(ReadSCTLR()|(1<<11));
  
	// Set up page table
	// 0x80000000 ~ 1GB RAM cached, the rest non-cachned
	for(i=0;i<2048;i++) PageTable[i]=(i<<20)|0x0de2;
	for(i=2048;i<3072;i++) PageTable[i]=(i<<20)|0x05de6;
	for(i=3072;i<4096;i++) PageTable[i]=(i<<20)|0x0de2;

	// Initialize MMU

	// Enable TTB0 only
	// Set translation table base address
	// Set all domains to client
	__asm volatile (
			" mcr	p15, 0, %[ttbcr], c2, c0, 2		\n" 	// Write Translation Table Base Control Register  LDR   r0, ttb_address
			" mcr	p15, 0, %[ttbr0], c2, c0, 0	\n" 		// Write Translation Table Base Register 0
			" mcr	p15, 0, %[dacr], c3, c0, 0 	\n" 		// Write Domain Access Control Register
			: : [ttbcr] "r" (0),
				[ttbr0] "r" (PageTable),
				[dacr] "r" (0x55555555)
				:  );

	/*
	 *  DACR 레지스터는 2비트씩 16개로 나누어 별도로 속성관리를 할 수 있음
	 *  0x55555555는 AP(2bit)의 값이 0x01로 Access Permisson이 Privileged는 Read/Write를 허용하며 User는 No Access임
	 */

	// Enable MMU
	WriteSCTLR(ReadSCTLR()|(1<<0));

	// Enable L1 I & D caches
	// SCU controls by hardware the coherency of the two Cortex-A9 MPCores L1 data caches
	//WriteSCTLR(ReadSCTLR()|(1<<2)|(1<<12));

#if 0
	// Debug UART Code
	while((((*(volatile unsigned int *)(0x48020014))&0x20)>>5)==0);
	(*(volatile unsigned int *)(0x48020000)) = portCORE_ID()+48;
#endif

	// Primary Core jump main, Secondary Core jump secondary_main
	(portCORE_ID()==0) ? main() : secondary_main();
}
/*----------------------------------------------------------------------------*/

// SMC: ARM Assembly SMC is swi
void Undefined_Handler_Panic( void )
{
vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"undefined_panic\r\n", strlen("undefined_panic\r\n"));
	__asm volatile ( " smc #0 " );
	for (;;);
}
/*----------------------------------------------------------------------------*/

void Prefetch_Handler_Panic( void )
{
vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"prefetch_panic\r\n", strlen("prefetch_panic\r\n"));
	__asm volatile ( " smc #0 " );
	for (;;);
}
/*----------------------------------------------------------------------------*/

void Abort_Handler_Panic( void )
{
	char cAddress[30];
	sprintf( cAddress, "Core-%d: abort_panic\r\n", portCORE_ID() );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );
	//vSerialPutString((xComPortHandle)mainPRINT_PORT, (const signed char * const)"abort_panic\r\n", strlen("abort_panic\r\n"));

//	__asm volatile ( " smc #8 " );
	for (;;);
}
/*----------------------------------------------------------------------------*/
