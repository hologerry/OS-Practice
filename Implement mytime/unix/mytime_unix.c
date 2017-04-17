//
//  mytime_unix.c
//  mytime
//
//  Created by Gerry on 14/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

static void pr_times(struct timeval start, struct timeval end);

int main(int argc, char *argv[])
{
    struct timeval start, end;
    pid_t       pid;

    if (argc <= 1) {
        printf("usage: mytime cmd [args opts]\n");
        exit(0);
    }

    if (gettimeofday(&start, NULL) < 0) {
        printf("Get start time error!\n");
    }

    if ((pid = fork()) < 0) {
        printf("Fork error!\n");
    } else if (pid == 0) {
        printf("command: %s\n", argv[1]);
        if (execvp(argv[1], argv+1) < 0) {
            printf("Execut cmd error.\n");
            exit(1);
        }
    } else {
        if (wait(NULL) < 0) {
            printf("Wait child cmd error.\n");
        }
    }
    
    if (gettimeofday(&end, NULL) < 0) {
        printf("Get end time error!\n");
    }

    pr_times(start, end);
    
    exit(0);
}

static void pr_times(struct timeval start, struct timeval end)
{
    int sec = (int)(end.tv_sec - start.tv_sec);
    int usec = (int)(end.tv_usec - start.tv_usec);

    int pr_usec = usec % 1000;
    int pr_msec = usec / 1000;
    int pr_sec = sec % 60;
    int pr_minute = sec / 60;
    int pr_hour = pr_minute / 60;

    printf("time: %dh%dm%ds%dms%dus\n", pr_hour, pr_minute, pr_sec, pr_msec, pr_usec);
}


