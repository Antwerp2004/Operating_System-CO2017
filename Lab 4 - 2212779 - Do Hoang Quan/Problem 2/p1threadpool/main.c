#include <stdio.h>
#include "bktpool.h"
#include <unistd.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#undef STRESS_TEST

#define SHARED_MEMORY
#undef DEBUG

int func(void * arg) {
  int id = * ((int * ) arg);

  printf("Task func - Hello from %d\n", id);
  wrk_ptr->arg=NULL;
  wrk_ptr->bktaskid=-1;
  wrk_ptr->func=NULL;
  
  fflush(stdout);

  return 0;
}

int main(int argc, char * argv[]) {
  int tid[15];
  int wid[15];
  int id[15];
  int ret;

  taskid_seed = 0;
  wrkid_cur = 0;
  bktask_sz = 0;
  
  id[0] = 1;
  id[1] = 2;
  id[2] = 5;
  
#ifdef SHARED_MEMORY
  create_shared_mem("Parent");
  attach_shared_mem("Parent");
#endif

  ret = bktpool_init();
  if (ret != 0)
    return -1;

  bktask_init( & tid[0], & func, (void * ) & id[0]);
  bktask_init( & tid[1], & func, (void * ) & id[1]);
  bktask_init( & tid[2], & func, (void * ) & id[2]);

  wid[1] = bkwrk_get_worker();
  ret = bktask_assign_worker(tid[0], wid[1]);
  if (ret != 0)
    printf("assign_task_failed tid=%d wid=%d\n", tid[0], wid[1]);
    
#ifdef SHARED_MEMORY
  fill_shared_mem_data("Parent",wid[1]);
#endif

#ifdef DEBUG
  printf("Parent -> Start dispatching with pid: %d\n",wrkid_tid[wid[1]]);
#endif

  /* Wakeup the worker */
  bkwrk_dispatch_worker(wid[1]);
  
  /* Wait for the worker done its work */
  while(wrk_ptr->func!=NULL);
  wrkid_busy[wid[1]]=0;

  wid[0] = bkwrk_get_worker();
  ret = bktask_assign_worker(tid[1], wid[0]);
  if (ret != 0)
    printf("assign_task_failed tid=%d wid=%d\n", tid[1], wid[0]);

  wid[2] = bkwrk_get_worker();
  ret = bktask_assign_worker(tid[2], wid[2]);
  if (ret != 0)
    printf("assign_task_failed tid=%d wid=%d\n", tid[2], wid[2]);
    
#ifdef SHARED_MEMORY
  fill_shared_mem_data("Parent",wid[0]);
  fill_shared_mem_data("Parent",wid[2]);
#endif

#ifdef DEBUG
  printf("Parent -> Start dispatching with pid: %d\n",wrkid_tid[wid[0]]);
  printf("Parent -> Start dispatching with pid: %d\n",wrkid_tid[wid[2]]);
#endif

  bkwrk_dispatch_worker(wid[0]);
  bkwrk_dispatch_worker(wid[2]);
  
  /* Wait for the worker done its work */
  while(wrk_ptr->func!=NULL);
  wrkid_busy[wid[0]]=0;
  wrkid_busy[wid[2]]=0;

#ifdef DEBUG
  printf("Parent -> Done work, worker id %d is free\n",wid[0]);
  printf("Parent -> Done work, worker id %d is free\n",wid[2]);
#endif

  fflush(stdout);
  sleep(1);
  
#ifdef SHARED_MEMORY
  detach_shared_mem("Parent",wrk_ptr);
#endif

#ifdef STRESS_TEST
  int i = 0;
  for (i = 0; i < 15; i++) {
    id[i] = i;
    bktask_init( & tid[i], & func, (void * ) & id[i]);

    wid[i] = bkwrk_get_worker();
    ret = bktask_assign_worker(tid[i], wid[i]);

    if (ret != 0)
      printf("assign_task_failed tid=%d wid=%d\n", tid[i], wid[i]);

    bkwrk_dispatch_worker(wid[i]);
  }

  sleep(3);
#endif

  return 0;
}
