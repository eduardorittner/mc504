#ifndef _SYNC_INCLUDE_H
#define _SYNC_INCLUDE_H

#include <semaphore.h>
#include <pthread.h>

/* Wrappers around pthread_mutex_* functions that prints to stderr and calls exit
on failure */

pthread_mutex_t* mutex_new(pthread_mutex_t* mutex);
void mutex_acquire(pthread_mutex_t* mutex);
void mutex_release(pthread_mutex_t *mutex);

/* Wrappers around sem_* functions that prints to stderr and calls exit on
failure */

sem_t* sem_new(sem_t* sem, int value);
void sem_acquire(sem_t *sem);
void sem_release(sem_t *sem);

#endif
