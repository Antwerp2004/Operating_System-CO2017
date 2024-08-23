#ifndef BKTPOOL_H
#define BKTPOOL_H

#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_WORKER 10
#define WRK_THREAD 1
#define STACK_SIZE 4096
#define SIG_DISPATCH SIGUSR1

int shmID;

/* Task ID is unique non-decreasing integer */
int taskid_seed;

int wrkid_tid[MAX_WORKER];
int wrkid_busy[MAX_WORKER];
int wrkid_cur;

struct bktask_t {
  void( * func)(void * arg);
  void * arg;
  unsigned int bktaskid;
  struct bktask_t * tnext;
}* bktask;

int bktask_sz;

struct bkworker_t {
  void( * func)(void * arg);
  void * arg;
  unsigned int wrkid;
  unsigned int bktaskid;
};

struct bkworker_t worker[MAX_WORKER];
struct bkworker_t *wrk_ptr;

/* Prototype API */

/* bktpool module */
int bktpool_init();

/* bktask module */
struct bktask_t * bktask_get_byid(unsigned int bktaskid);
int bktask_init(unsigned int * bktaskid, void * func, void * arg);
int bktask_assign_worker(unsigned int bktaskid, unsigned int wrkid);

/* bkwrk module */
void * bkwrk_worker(void * arg);
int bkwrk_create_worker();
int bkwrk_dispatch_worker(unsigned int wrkid);
int bkwrk_get_worker();

/* shared memory module */
void create_shared_mem(char *proc);
void attach_shared_mem(char *proc);
void fill_shared_mem_data(char *proc, int wrk_id);
void detach_shared_mem(char *proc, struct bkworker_t *shm_ptr);

#endif
