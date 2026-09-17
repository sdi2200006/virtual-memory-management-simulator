#include "../include/MM.h"

//Calculate the position in HashPageTable
unsigned long long int hashCalculate (MemoryManager* mm, unsigned long long int  address){
    return address % mm->totalFrames;
}

//Prints statistics for PM1,PM2 and total
void printStats(MemoryManager* mm){
    
    printf("\nPM1\n");
    printf("Entries: %llu\n", mm->stats1.entries);
    printf("Page Faults: %llu\n", mm->stats1.pageFault);
    printf("Reads From Disc: %llu\n", mm->stats1.readDisk);
    printf("Writes to disc: %llu\n", mm->stats1.writeDisk);
    printf("Used Frames: %llu\n", mm->stats1.frames);

    printf("\nPM2\n");
    printf("Entries: %llu\n", mm->stats2.entries);
    printf("Page Faults: %llu\n", mm->stats2.pageFault);
    printf("Reads From Disc: %llu\n", mm->stats2.readDisk);
    printf("Writes to disc: %llu\n", mm->stats2.writeDisk);
    printf("Used Frames: %llu\n\n", mm->stats2.frames);

    printf("\nTotal\n");
    printf("Entries: %llu\n", mm->stats1.entries + mm->stats2.entries);
    printf("Page Faults: %llu\n", mm->stats1.pageFault + mm->stats2.pageFault);
    printf("Reads From Disc: %llu\n", mm->stats1.readDisk + mm->stats2.readDisk);
    printf("Writes to disc: %llu\n", mm->stats1.writeDisk + mm->stats2.writeDisk);
    printf("Used Frames: %llu\n\n", mm->stats1.frames + mm->stats2.frames );

}

//Search the address in the HashPageTable and returns a pointer or NULL
PageFrame* findPage(MemoryManager *mm, int pid, unsigned long long int  address) {
    // finds position
    unsigned long long i = hashCalculate(mm, address);
    //takes the first pageframe
    PageFrame *cur = mm->HashPageTable[i];
    //while it has pageframes continue
    while (cur != NULL) {
        //if it is from the same pm and it is the same address 
        if (cur->pid == pid && cur->pageNumber == address) {
            //returns pointer
            return cur;
        }
        //go to the next
        cur = cur->next;
    }
    //if it wasn't found then return NULL
    return NULL;
}

//Insert the address in HashPageTable and updates the data
void insert(MemoryManager *mm, int pid, unsigned long long int  address, bool dirty) {
    //find position
    unsigned long long i = hashCalculate(mm, address);

    //create pageframe
    PageFrame *p = (PageFrame*)malloc(sizeof(PageFrame));
    if (!p) exit(1);

    //update data
    p->pid = pid;
    p->pageNumber = address;
    p->dirty = dirty;
    p->next = mm->HashPageTable[i];

    // update hashpageframe
    mm->HashPageTable[i] = p;

    //update statistics
    if (pid == 1) {
        mm->stats1.readDisk++;
    }
    else{ 
        mm->stats2.readDisk++;
    }
}

// FWF algorithm of one specific PM (=pid)
void FWF(MemoryManager *mm, int pid) {
    for (unsigned long long i = 0; i < mm->totalFrames; i++) {
        PageFrame *cur = mm->HashPageTable[i];
        PageFrame *prev = NULL;
        PageFrame *v ;
        while (cur) {
            // if is one of the specific PM
            if (cur->pid == pid) {
                v = cur;
                //updates statistic
                if (v->dirty) {
                    if (pid == 1) {
                        mm->stats1.writeDisk++;
                    }
                    else{
                        mm->stats2.writeDisk++;
                    }
                }
                //if its in the middle of list, change the pointer of previous pageframe
                if (prev) {
                    prev->next = cur->next;
                }
                //if it is the first pageframe of list, change the pointer of Hashpagetable in position i
                else {
                    mm->HashPageTable[i] = cur->next;
                }
                // go to the next and free the v 
                cur = cur->next;
                free(v);
            } 
            // if is NOT one of the specific PM, go to the next
            else {
                prev = cur;
                cur  = cur->next;
            }
        }
    };
}

//Initialize the MM's data and creates Shared Memory and Semaphores
void mm_init(MemoryManager* mm, unsigned long long int totalFrames, unsigned long long int k, unsigned long long int q, unsigned long long int max){
    //CREATE SM AND SEMAPHORES FOR PM1-MM AND PM2-MM
    if (ipc_consumer_create(SHM_PM1, SEM_EMPTY_PM1, SEM_FULL_PM1, SEM_MUTEX_PM1, &mm->sm1) == -1) {
        perror("[MM] ipc_consumer_create PM1");
        exit(1);
    }
    if (ipc_consumer_create(SHM_PM2, SEM_EMPTY_PM2, SEM_FULL_PM2, SEM_MUTEX_PM2, &mm->sm2) == -1) {
        perror("[MM] ipc_consumer_create PM2");
        exit(1);
    }
    //CREATE HASH PAGE TABLE
    mm->HashPageTable = (PageFrame**) calloc(totalFrames, sizeof(PageFrame *));
    if (!mm->HashPageTable) { perror("calloc frames"); exit(1); }

    //INITIALIZE FRAMES
    mm->totalFrames  = totalFrames;
    mm->framesP1 = totalFrames / 2;    
    mm->framesP2 = totalFrames - mm->framesP1;

    //INITIALIZE k,q,max WITH WHAT USER GAVE
    mm->k = k;                  
    mm->q = (q > 0) ? q : 1;    
    mm->max = max;

    //INITIALIZE STATISTICS
    mm->stats1.readDisk = 0;
    mm->stats1.writeDisk = 0;
    mm->stats1.pageFault = 0;
    mm->stats1.entries = 0;
    mm->stats1.frames = 0;
    
    mm->stats2.readDisk = 0;
    mm->stats2.writeDisk = 0;
    mm->stats2.pageFault = 0;
    mm->stats2.entries = 0;
    mm->stats2.frames = 0;
    
    //INITIALIZE PF IN BLOCK FOR EACH PM
    mm->PF1_block = 0;                           
    mm->PF2_block = 0;                           

    printf("[MM] Initialized: total=%llu (frames P1=%llu, frames P2=%llu), k=%llu, q=%llu, max=%llu\n",totalFrames, mm->framesP1, mm->framesP2, k, q, max);
}

//Runs process
void mm_run(MemoryManager* mm) {
    if (!mm || !mm->sm1 || !mm->sm2) {
        perror("[MM] Error mm_run\n"); 
        return;
    }

    unsigned long long used1 = 0, used2 = 0; 
    bool ended1 = false, ended2 = false;
    Request req;

    while (ended1 == false || ended2 == false) {
        if (ended1 == false) {
            for (unsigned long long i = 0; i < mm->q; i++) {
                //read one request
                ipc_consumer_pop(mm->sm1, &req);
                //if it is the end , stop and put ended1 = true for the next repetitions
                if (req.end) { 
                    ended1 = true; 
                    break; 
                }
               
                //update entries1 
                mm->stats1.entries++;
                //take only the page number
                const unsigned long long int  pageNumber = req.address >> 12;

                printf("[MM  1] address=%#llx pageNumber=%llu %c\n", req.address, pageNumber, req.is_write?'W':'R');

                //find if pageNumber already exist in HashPageTable
                PageFrame *p = findPage(mm, 1, pageNumber);
                //if exists and is W/w update data of pageframe
                if (p) {
                    if (req.is_write) p->dirty = true;
                } 
                //if it doesn't exist in hashpagetable
                else {
                    //increase PFs and see if it has to do FWF
                    mm->stats1.pageFault++;
                    mm->PF1_block++;
                    if (mm->PF1_block > mm->k || used1 == mm->framesP1) {
                        //FWF of PM1 and update the data
                        FWF(mm, 1);
                        mm->PF1_block = 1; 
                        used1 = 0;
                    }
                    //insert it 
                    insert(mm, 1, pageNumber, req.is_write);
                    used1++;
                }
            }
            // changes block 
            mm->PF1_block = 0; 
        }
        if (ended2 == false) {
            for (unsigned long long i = 0; i < mm->q; i++) {
                //read one request
                ipc_consumer_pop(mm->sm2, &req);
                //if it is the end , stop and put ended2 = true for the next repetitions
                if (req.end) { 
                    ended2 = true; 
                    break; 
                }

                //update entries2
                mm->stats2.entries++;
                //take only the page number
                const unsigned long long int  pageNumber = req.address >> 12;

                printf("[MM  2] address=%#llx pageNumber=%llu %c\n", req.address,pageNumber,req.is_write?'W':'R');

                //find if pageNumber already exist in HashPageTable
                PageFrame *p = findPage(mm, 2, pageNumber);
                //if exists and is W/w update data of pageframe
                if (p) {
                    if (req.is_write) p->dirty = true;
                } 
                //if it doesn't exist in hashpagetable
                else {
                    //increase PFs and see if it has to do FWF
                    mm->stats2.pageFault++;
                    mm->PF2_block++;
                    if (mm->PF2_block > mm->k || used2 == mm->framesP2) {
                        //FWF of PM2 and update the data
                        FWF(mm, 2);
                        mm->PF2_block = 1;
                        used2 = 0;
                    }
                    //insert it 
                    insert(mm, 2, pageNumber, req.is_write);
                    used2++;
                }
            }            
            // changes block 
            mm->PF2_block = 0; 
        }
    }
    //finish and update statistics
    mm->stats1.frames = used1;
    mm->stats2.frames = used2;
}

//Delete/destroy what was created in mm_init
void mm_destroy(MemoryManager* mm) {
    if (!mm) {
        perror("[MM] Error mm_destroy\n"); 
        return;
    }
    
    //prints statistic
    printStats(mm);

    // delete shared memories and semaphores
    if (mm->sm1)   ipc_consumer_close(SHM_PM1, SEM_EMPTY_PM1, SEM_FULL_PM1, SEM_MUTEX_PM1, mm->sm1); 
    if (mm->sm2)   ipc_consumer_close(SHM_PM2, SEM_EMPTY_PM2, SEM_FULL_PM2, SEM_MUTEX_PM2, mm->sm2);

    //free the HashPageTable and the Pageframes that exists 
    if (mm->HashPageTable) {
        for (unsigned long long i = 0; i < mm->totalFrames; i++) {
            PageFrame *cur = mm->HashPageTable[i];
            while (cur) {
                PageFrame *temp = cur;
                cur = cur->next;
                free(temp);
            }
        }
        free(mm->HashPageTable);
    }
}