#ifndef MM_H
#define MM_H
#include "../include/IPC.h"


typedef struct PageFrame {
    int pid;
    unsigned long long int  pageNumber;
    bool dirty;
    struct PageFrame *next;  
} PageFrame;

typedef struct {
    unsigned long long int readDisk;     // READS FROM DISC
    unsigned long long int writeDisk;    // WRITE TO DISC
    unsigned long long int pageFault;    // PAGE FAULTS
    unsigned long long int entries;      // TOTAL ANAFORES
    unsigned long long int frames;       // USED FRAMES AT THE END 
} Stats;
 
typedef struct {
    PageFrame** HashPageTable;              
    unsigned long long int totalFrames;  // # frames
    unsigned long long int framesP1;     // frames for PM1
    unsigned long long int framesP2;     // frames for PM2

    unsigned long long int k;            //lim of PF     
    unsigned long long int q;            //#entries of blocks       
    unsigned long long int max;          //max entries from each file

    Stats stats1;                        //Statistics of PM1
    Stats stats2;                        //Statistics of PM2
    
    unsigned long long int PF1_block;    //current number of PF of PM1
    unsigned long long int PF2_block;    //current number of PF of PM2

    Info *sm1;                           // SHM/sem του PM1
    Info *sm2;                           // SHM/sem του PM2

} MemoryManager;

/* Functions of MM */
void mm_init(MemoryManager* mm,unsigned long long int totalFrames, unsigned long long int k, unsigned long long int q, unsigned long long int max);
void mm_run(MemoryManager* mm);     
void mm_destroy(MemoryManager* mm);  
#endif 
