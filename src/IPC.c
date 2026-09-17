#include "../include/IPC.h"

//Reads one line of file, checks the character after the address and returns addr and true/false for W,w/R,r
int parse_line(FILE *f, unsigned long long int *address, bool *is_write) {
    char op;
    if (fscanf(f, "%llx %c", address, &op) != 2) return 0;

    if (op == 'R' || op == 'r') {
        *is_write = false;
    } else if (op == 'W' || op == 'w') {
        *is_write = true;
    } else {
        return 0;
    }
    return 1;
}

int ipc_producer_open(const char *shm, const char *name_empty, const char *name_full, const char *name_mutex, Info **out) {
    // Open an existing shared memory 
    int fd = shm_open(shm, O_RDWR, 0600);     
    if (fd == -1) { 
        perror("shm_open of ipc_producer_open"); 
        return -1; 
    }

    // Map the shared memory to address
    void *mem = mmap(NULL, sizeof(Buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);     
    if (mem == MAP_FAILED) { 
        perror("mmap of ipc_producer_open"); 
        close(fd); 
        return -1; 
    }
    close(fd);

    // Has shared memory and semaphores
    Info *h = malloc(sizeof(Info));
    if (!h) { 
        munmap((Buffer*)mem, sizeof(Buffer));  
        return -1; 
    }
    h->shm = (Buffer*)mem;

    // Open existing semaphores
    h->sem_empty = sem_open(name_empty, O_RDWR);
    h->sem_full  = sem_open(name_full,  O_RDWR);
    h->sem_mutex = sem_open(name_mutex, O_RDWR);
    if (h->sem_empty==SEM_FAILED || h->sem_full==SEM_FAILED || h->sem_mutex==SEM_FAILED) {
        perror("ipc_producer_open");
        munmap((Buffer*)mem, sizeof(Buffer)); 
        free(h);                  
        return -1;
    }
    *out = h;   // Return the Info
    return 0;
}

int ipc_consumer_create(const char *shm, const char *name_empty, const char *name_full, const char *name_mutex, Info **out) {
    // Create a new shared memory 
    int fd = shm_open(shm, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) { 
        perror("shm_open of ipc_consumer_create");
        return -1; 
    }

    // Set the size of the shared memory
    if (ftruncate(fd, sizeof(Buffer)) == -1) {
        perror("ftruncate of ipc_consumer_create");
        close(fd);
        shm_unlink(shm);   
        return -1;
    }

    // Map the shared memory to address 
    void *mem = mmap(NULL, sizeof(Buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap of ipc_consumer_create");
        close(fd);
        shm_unlink(shm);
        return -1;
    }
    close(fd);
    // Initialize buffer -> 0
    memset((Buffer*)mem, 0, sizeof(Buffer));   

    // Has shared memory and semaphores
    Info *h = malloc(sizeof(Info));
    if (!h) {
        perror("malloc of ipc_consumer_create");
        munmap((Buffer*)mem, sizeof(Buffer));
        shm_unlink(shm);
        return -1;
    }
    h->shm = (Buffer*)mem;

    // Creates new semaphores 
    h->sem_empty = sem_open(name_empty, O_CREAT | O_EXCL, SEM_PERMS, Buffer_size); //all slots empty
    h->sem_full  = sem_open(name_full,  O_CREAT | O_EXCL, SEM_PERMS, 0);           //no slots full 
    h->sem_mutex = sem_open(name_mutex, O_CREAT | O_EXCL, SEM_PERMS, 1);          // 1 for mutual exclusion
    if (h->sem_empty==SEM_FAILED || h->sem_full==SEM_FAILED || h->sem_mutex==SEM_FAILED) {
        perror("sem_open of ipc_consumer_create");
        if (h->sem_empty != SEM_FAILED) sem_unlink(name_empty);
        if (h->sem_full  != SEM_FAILED) sem_unlink(name_full);
        if (h->sem_mutex != SEM_FAILED) sem_unlink(name_mutex);
        munmap((Buffer*)mem, sizeof(Buffer));
        shm_unlink(shm);
        free(h);
        return -1;
    }

    *out = h;   // Return the Info
    return 0;
}


void ipc_consumer_pop(Info *h, Request *out){
    // Waits until there is one+ request
    sem_wait(h->sem_full);
    // Mutual  access 
    sem_wait(h->sem_mutex);
    
    // Read the request and update tail
    *out = h->shm->buf[h->shm->tail];
    h->shm->tail = (h->shm->tail + 1) % Buffer_size;
    
    // No mutual  access 
    sem_post(h->sem_mutex);
    // Signal one empty position
    sem_post(h->sem_empty);
}

void ipc_producer_push(Info *h, const Request *in){
    // Waits until there is one+ empty positions
    sem_wait(h->sem_empty);
    // Mutual  access 
    sem_wait(h->sem_mutex);

    // Writes the request and update head
    h->shm->buf[h->shm->head] = *in;
    h->shm->head = (h->shm->head + 1) % Buffer_size;
    
    // No mutual  access 
    sem_post(h->sem_mutex);
    // Signal one full position
    sem_post(h->sem_full);
}

void ipc_consumer_close(const char *shm,const char *name_empty, const char *name_full,const char *name_mutex, Info *h){
    // Closes the semaphores
    sem_close(h->sem_empty);
    sem_close(h->sem_full);
    sem_close(h->sem_mutex);

    // Deletes the semaphores
    sem_unlink(name_empty);
    sem_unlink(name_full);
    sem_unlink(name_mutex);

    //Unmap the shared memory
    munmap(h->shm, sizeof(Buffer));
    //Deletes the shared memory and free info
    shm_unlink(shm);
    free(h);
}

void ipc_producer_close(Info *h){
    // Closes the semaphores at producer but continue to consumer
    sem_close(h->sem_empty);
    sem_close(h->sem_full);
    sem_close(h->sem_mutex);
    
    //Free info and shared memory  but continue to consumer
    munmap(h->shm, sizeof(Buffer));
    free(h);
}