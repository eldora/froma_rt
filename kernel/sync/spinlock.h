#ifndef SYNC_H
#define SYNC_H

typedef struct{
	//volatile unsigned char *pxBitBand;
	volatile unsigned int flag;
}spinlock_t;

void __spin_lock(volatile spinlock_t *lock);
void __spin_unlock(volatile spinlock_t *lock);

#endif
