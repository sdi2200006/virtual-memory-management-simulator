#include "../include/IPC.h"

int main(int argc, char *argv[]) {

    unsigned long long int max = (argc > 1) ? strtoull(argv[1], NULL, 10) : 0;
    unsigned long long int count = 0 ;   
    bool num = (max != 0);
   
    Info *i = NULL;
    // Opens shared memory and semaphores for PM1
    if (ipc_producer_open(SHM_PM1, SEM_EMPTY_PM1, SEM_FULL_PM1, SEM_MUTEX_PM1, &i) == -1) {
        perror("[PM1] FAILED ipc_producer_open\n"); 
        return 1;
    }
    // Opens file TRACE1
    FILE *f = fopen(TRACE1, "r");
    if (!f) { 
        perror("[PM1] FAILED FOPEN\n"); 
        return 2;
    }

    unsigned long long int  addr; 
    bool is_write;

    Request req; 
    req.pid = 1; 
    req.end = 0;

    // Reads line-line and send requests to SM until the end of file
    while (parse_line(f, &addr, &is_write)) {
        req.is_write = is_write;
        req.address = addr;
        printf("[PM1] address=0x%llx pageNumber=%llu %c\n", req.address, req.address >> 12,is_write?'W':'R');
        ipc_producer_push(i, &req);
        // If there is a max, stop when it will reach it 
        if ( num ){
            count ++;
            if (count == max ) break;
        }
    }
    // Send the final request
    req.end = 1; 
    ipc_producer_push(i, &req);
    // Close shared memory and file for PM1
    ipc_producer_close(i);
    fclose(f);
    return 0;
}
