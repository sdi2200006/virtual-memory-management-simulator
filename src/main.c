#include <sys/wait.h>        //waitpid
#include "../include/MM.h"


int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "%s k totalFrames q or %s k totalFrames q Max\n", argv[0],  argv[0]);
        return 1;
    }

    unsigned long long int k = strtoull(argv[1], NULL, 10);
    unsigned long long int frames = strtoull(argv[2], NULL, 10);
    unsigned long long int q = strtoull(argv[3], NULL, 10);
    unsigned long long int maxR = 0;
    char *max = "0"; 
    if (argc == 5) {
        maxR = strtoull(argv[4], NULL, 10);
        if (maxR == 0) {
            perror("[main] Max must be bigger of 0.\n");
            return 1;  
        }
        max = argv[4];   
    }

    // Initialize the MM and creates shared memories + semaphores
    MemoryManager mm;
    mm_init(&mm, frames, k, q, maxR);

    // Fork child PM1 
    pid_t c1 = fork();
    if (c1 == 0) {
        execl("./pm1", "pm1", max, NULL);
        perror("[main] execl pm1");
        return 1;
    }
    // Fork child PM2    
    pid_t c2 = fork();
    if (c2 == 0) {
        execl("./pm2", "pm2" , max, NULL);
        perror("[main] execl pm2");
        return 2;
    }

    //Runs MM
    mm_run(&mm);
    
    //Waits PM1 and PM2 to finish
    int status;
    waitpid(c1, &status, 0);
    waitpid(c2, &status, 0);

    //Destroy shared memories, semaphores, HashPageTable and print statistics
    mm_destroy(&mm);
    return 0;
}
