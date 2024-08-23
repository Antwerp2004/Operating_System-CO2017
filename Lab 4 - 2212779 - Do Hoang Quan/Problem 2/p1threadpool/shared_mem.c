#include "bktpool.h"

void create_shared_mem(char *proc){
   /* Apply shared memory for each process to upsate value */
   shmID=shmget(IPC_PRIVATE,sizeof(wrk_ptr)*MAX_WORKER,IPC_CREAT|0664);
   if(shmID<0){
      perror("shmget");
      exit(1);
   }
}

void attach_shared_mem(char *proc){
   wrk_ptr=(struct bkworker_t *)shmat(shmID,NULL,0);
   if(wrk_ptr==(struct bkworker_t *)-1){
      perror("shmat");
      exit(1);
   }
}

void fill_shared_mem_data(char *proc, int wrk_id){
   wrk_ptr[wrk_id].bktaskid=worker[wrk_id].bktaskid;
   wrk_ptr[wrk_id].func=worker[wrk_id].func;
   wrk_ptr[wrk_id].arg=worker[wrk_id].arg;
}

void detach_shared_mem(char *proc, struct bkworker_t *shm_ptr){
   if(shmdt(shm_ptr)==-1){
      perror("shmdt");
   }
}
