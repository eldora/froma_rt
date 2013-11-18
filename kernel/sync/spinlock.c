#include "FROMA_HEADER.h"

void __spin_lock(volatile spinlock_t *lock){
	portDISABLE_INTERRUPTS();
	__asm volatile ( "dmb" ::: "memory" );
	while(lock->flag)
		__asm volatile ( "" ::: "memory" );
	lock->flag = 1;
	__asm volatile ( "dmb" ::: "memory" );
}

void __spin_unlock(volatile spinlock_t *lock) {
	__asm volatile ( "dmb" ::: "memory" );
	lock->flag = 0;
	__asm volatile ( "dsb" ::: "memory" );
	__asm volatile ( "dmb" ::: "memory" );
	portENABLE_INTERRUPTS();
}
