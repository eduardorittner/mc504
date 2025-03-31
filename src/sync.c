#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t *mutex_new(pthread_mutex_t *mutex) {
  if (pthread_mutex_init(mutex, NULL) < 0) {
    perror("Couldn't initialize mutex");
    exit(-1);
  }
  return mutex;
}

void mutex_acquire(pthread_mutex_t *mutex) {
  if (pthread_mutex_lock(mutex) < 0) {
    perror("Failed to lock mutex");
    exit(-1);
  }
}

void mutex_release(pthread_mutex_t *mutex) {
  if (pthread_mutex_unlock(mutex) < 0) {
    perror("Failed to unlock mutex");
    exit(-1);
  }
}

sem_t *sem_new(sem_t *sem, unsigned int value) {
  if (sem_init(sem, 0, value) < 0) {
    perror("Couldn't initialize semaphore");
    exit(-1);
  }
  return sem;
}

void sem_acquire(sem_t *sem) {
  if (sem_wait(sem) < 0) {
    perror("Failed to lock semaphore");
    exit(-1);
  }
}

void sem_release(sem_t *sem) {
  if (sem_post(sem) < 0) {
    perror("Failed to unlock semaphore");
    exit(-1);
  }
}
