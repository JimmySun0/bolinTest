#include <iostream>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <math.h>
#include <ecrt.h>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sched.h> /* sched_setscheduler() */

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "ecat.h"

using namespace std;

static int latency_target_fd;
int32_t latency_target_value = 0;
int run = 1;
int32_t destinePosition = 0;

void signal_stop(int sig)
{
    run = 0;
}

static void set_latency_target(void)
{
        struct stat s;
        int err;

        err = stat("/dev/cpu_dma_latency", &s);
        if (err == -1) {
                perror("WARN: stat /dev/cpu_dma_latency failed: ");
                return;
        }


        latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
        if (latency_target_fd == -1) {
                perror("WARN: open /dev/cpu_dma_latency: ");
                return;
        }
        err = write(latency_target_fd, &latency_target_value, 4);
        if (err < 1) {
                printf("# error setting cpu_dma_latency to %d!", latency_target_value);
                close(latency_target_fd);
                return;
        }
        printf("# /dev/cpu_dma_latency set to %dus\n", latency_target_value);
}

int main()
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;
    signal(SIGINT, signal_stop);
    signal(SIGTERM, signal_stop);


    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
    {
        perror("mlockall failed");
        return -1;
    }

    set_latency_target();

    ret = pthread_attr_init(&attr);
    if(ret)
    {
        perror("init pthread attributes failed\n");
        exit(EXIT_FAILURE);
    }

    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
            printf("pthread setschedpolicy failed\n");
            goto out;
    }

    param.sched_priority = 98;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
            printf("pthread setschedparam failed\n");
            goto out;
    }

    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        goto out;
    }

    ret = pthread_create(&thread, &attr, ecat_task, NULL);
    if (ret) {
        printf("create pthread failed\n");
        goto out;
    }

    int num;
    while(cin >> num)
    {
        if(0 == num)
            break;

        destinePosition = num;
    }

    while(run)
    {
        usleep(100000);
    }

    out:
        close(latency_target_fd);
        munlockall();

    ret = pthread_join(thread, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

    cout << "Hello World!" << endl;
    return 0;
}
