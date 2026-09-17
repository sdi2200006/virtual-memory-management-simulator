#ifndef IPC_H
#define IPC_H
#include <stdio.h>
#include <stdlib.h>          //free, malloc,strtoull,exit
#include <stdbool.h>         // bool
#include <string.h>          //memset
#include <fcntl.h>           // Flags για open
#include <unistd.h>          // ftruncate ,fork, execl
#include <sys/types.h>       // pid_t
#include <sys/stat.h>        // S_IRUSR, S_IWUSR
#include <sys/mman.h>        // Shared Memory
#include <semaphore.h>       // Semaphores


// Size of buffer
#define Buffer_size 1024

// Shared memory 
#define SHM_PM1  "/shm1"
#define SHM_PM2  "/shm2"

// Semaphores PM1
#define SEM_EMPTY_PM1 "/empty1"  
#define SEM_FULL_PM1  "/full1"  
#define SEM_MUTEX_PM1  "/mutex1"    
// Semaphores PM2
#define SEM_EMPTY_PM2 "/empty2"
#define SEM_FULL_PM2  "/full2"
#define SEM_MUTEX_PM2  "/mutex2"

// Files
#define TRACE1 "traces/bzip.trace"
#define TRACE2 "traces/gcc.trace"

//Permisions for sem_open of consumer
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)


// struct of the info producer -> consumer
typedef struct {
    int  pid;                               // 1 (PM1) OR 0 (PM2)
    unsigned long long int  address;        //Page Number | offset
    bool  is_write;                         // 0 = Read, 1 = Write
    bool  end;                              // 1 => Last Request
} Request;

// shared memory 
typedef struct {
    unsigned int head;         // pointer for producer
    unsigned int tail;         // pointer for consumer
    Request buf[Buffer_size];
} Buffer; 

// shared memory + Semaphores for each PM
typedef struct {
    Buffer *shm;      
    sem_t  *sem_empty;
    sem_t  *sem_full;
    sem_t  *sem_mutex;
} Info; 


// Functions of producer (PM1/PM2)
int parse_line(FILE *f, unsigned long long int  *address, bool *is_write) ;
int ipc_producer_open(const char *shm, const char *name_empty, const char *name_full, const char *name_mutex, Info **out);
void ipc_producer_push(Info *h, const Request *req);
void ipc_producer_close(Info *h);

// Functions of consumer (MM)
int ipc_consumer_create(const char *shm, const char *name_empty,const char *name_full, const char *name_mutex, Info **out) ;
void ipc_consumer_pop(Info *h, Request *out);
void ipc_consumer_close(const char *shm,const char *name_empty, const char *name_full,const char *name_mutex, Info *h);

#endif 
