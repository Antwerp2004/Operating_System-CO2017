/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))
#define TLB_SIZE mp->maxsz/TLB_ENTRY_SIZE

pthread_mutex_t cache_lock;

// need 1 bit for detect the entry is used or not: 0 = Not used, 1 = Used
// 32 bit (unsigned int) = 4 bytes for pid
// need 14 bit for pgnum -> 2 bytes
// frame number has 13 bit = 2 byte
// -/----/--/--  => 1 entry hết 9 byte
// pid/pgnum/frame number

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, int* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      associated mapping etc.
    */
   pthread_mutex_lock(&cache_lock);
   /* Iterate over all tlb_entry in mp->storage, cause we are using fully associative */
   for(int i = 0; i < TLB_SIZE * TLB_ENTRY_SIZE; i += TLB_ENTRY_SIZE)
   {
      // Extracting pid, pgnum, data from current tlb_entry
      int used = mp->storage[i];
      if ((used & 1) == 0)
         continue;
      int entry_pid = 0;
      for(int j = 3; j >= 0; j--)
      {
         entry_pid |= (mp->storage[i + 4 - j] << (j * 8));
      }
      int entry_pgnum = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
      int entry_data = (mp->storage[i + 7] << 8) | mp->storage[i + 8]; // Frame number
         
      /* If pid and pgnum of current tlb_entry match with the original pid and pgnum */
      if(entry_pid == pid && entry_pgnum == pgnum)
      {
         // Set value to current data of tlb_entry and return 0
         *value = entry_data;
         pthread_mutex_unlock(&cache_lock);
         return 0;
      }
   }
   pthread_mutex_unlock(&cache_lock);
   // If no precise tlb_entry matched, return -1
   return -1;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, int *value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      associated mapping etc.
    */
   pthread_mutex_lock(&cache_lock);
   int free_entry = -1;
   /* Iterate over all tlb_entry in mp->storage, cause we are using fully associative */
   for(int i = 0; i < TLB_SIZE * TLB_ENTRY_SIZE; i += TLB_ENTRY_SIZE)
   {
      int flag = 0;
      for(int j = 0; j <= 8; j++)
      {
         if(mp->storage[i + j] != 0)
         {
            flag = 1;
            break;
         }
      }
      if(flag == 0)
      {
         free_entry = i / TLB_ENTRY_SIZE;
         break;
      }
   }
   if(free_entry == -1)
   { // Full TLB
      free_entry = rand() % TLB_SIZE;
   }
   int base_entry_addr = free_entry * TLB_ENTRY_SIZE;
   // Write pid to cache
   for(int i = 0; i <= 3; i++)
   {
      TLBMEMPHY_write(mp, base_entry_addr + 4 - i, (pid >> (i * 8)) & 0xFF);
   }
   // Write pgnum to cache
   TLBMEMPHY_write(mp, base_entry_addr + 6, pgnum & 0xFF);
   TLBMEMPHY_write(mp, base_entry_addr + 5, (pgnum >> 8) & 0xFF);
   // Write value(frame number) to cache
   TLBMEMPHY_write(mp, base_entry_addr + 8, *value & 0xFF);
   TLBMEMPHY_write(mp, base_entry_addr + 7, (*value >> 8) & 0xFF);
   
   // Mark used entry
   TLBMEMPHY_write(mp, base_entry_addr, 1);
   
   pthread_mutex_unlock(&cache_lock);
   return 0;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   printf("TLBMEMPHY_dump\n");
   printf("TLB Cache Start\n");
   // Check whether the physical memory exists or not
   if(mp == NULL || mp->storage == NULL)
   {
      printf("Physical memory doesn't exist.");
      return -1;
   }
   
   pthread_mutex_lock(&cache_lock);
   /* Iterate over all tlb_entry in mp->storage */
   for(int i = 0; i < TLB_SIZE * TLB_ENTRY_SIZE; i += TLB_ENTRY_SIZE)
   {
      // Extracting pid, pgnum, data from current tlb_entry
      int used = mp->storage[i];
      if ((used & 1) == 0)
         continue;
      int entry_pid;
      for(int j = 0; j <= 3; j++)
      {
         entry_pid |= (mp->storage[i + 4 - j] << (j * 8));
      }
      int entry_pgnum = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
      int entry_data = (mp->storage[i + 7] << 8) | mp->storage[i + 8]; // Frame number
      printf("Entry %d:\tUsed: %d\tEntry pid: %d\tEntry pagenum: %d\tEntry framenum: %d\n", i/TLB_ENTRY_SIZE, used, entry_pid, entry_pgnum, entry_data);
   }
   
   pthread_mutex_unlock(&cache_lock);
   printf("TLB Cache End\n");
   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;

   pthread_mutex_init(&cache_lock, NULL);
   return 0;
}

//#endif
