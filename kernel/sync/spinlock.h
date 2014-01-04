#ifndef SYNC_H
#define SYNC_H

typedef struct{
	//volatile unsigned char *pxBitBand;
	volatile unsigned int sLock;
}spinlock_t;

static inline void __spin_lock(spinlock_t *lock){
	unsigned long tmp;

	portDISABLE_INTERRUPTS();
	__asm volatile (
			"1:	ldrex	%0, [%1]\n"
			"		teq		%0, #0\n"
//			"		wfe\n"
			"		strexeq %0, %2, [%1]\n"
			"		teqeq		%0, #0\n"
			"		bne			1b\n"
			"		dmb\n"
			: "=&r" (tmp)
			: "r" (&lock->sLock), "r" (1)
			: "cc"
			);
}

static inline void __spin_unlock(spinlock_t *lock) {
	__asm volatile (
			"dmb\n"
			"str	%1, [%0]\n"
			:
			: "r" (&lock->sLock), "r" (0)
			: "cc"
			);
	portENABLE_INTERRUPTS();
}

#endif
