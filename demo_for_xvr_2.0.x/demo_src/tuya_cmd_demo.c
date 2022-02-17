#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include "tuya_ipc_common_demo.h"
#if defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)
#if 1
static pid_t    *childpid = NULL;
                        /* ptr to array allocated at run-time */
static int      maxfd;  /* from our open_max(), {Prog openmax} */

#define SHELL   "/bin/sh"

#ifdef  OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

/*
 * If OPEN_MAX is indeterminate, we're not
 * guaranteed that this is adequate.
 */
#define OPEN_MAX_GUESS 256

static long open_max(void)
{
    if (openmax == 0) {      /* first time through */
        errno = 0;

        if ((openmax = sysconf(_SC_OPEN_MAX)) < 0) {
            if (errno == 0) {
                openmax = OPEN_MAX_GUESS;    /* it's indeterminate */
            } else {
                printf("sysconf error for _SC_OPEN_MAX");
            }
        }
    }

    return openmax;
}

static int ty_close_all_fd()
{
    int i = 0;
    struct rlimit rl;

    if(getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        perror("getrlimit(RLIMIT_NOFILE, &rl)");
        return -1;
    }

    if(rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }

    for(i = 3; i < rl.rlim_max; i++) {
        close(i);
    }

    return 0;
}

static FILE* ty_popen(const char *cmdstring, const char *type)
{
    int     i, pfd[2];
    pid_t   pid;
    FILE    *fp;

    /* only allow "r" or "w" */
    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
        errno = EINVAL;     /* required by POSIX.2 */
        printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
        return(NULL);
    }

    if (childpid == NULL) {     /* first time through */
        /* allocate zeroed out array for child pids */
        maxfd = open_max();
        if((childpid = calloc(maxfd, sizeof(pid_t))) == NULL) {
            printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
            return NULL;
        }
    }

    if (pipe(pfd) < 0)
    {
        printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
        return(NULL);   /* errno set by pipe() */
    }

    if ( (pid = vfork()) < 0)
    {
        printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
        return(NULL);   /* errno set by fork() */
    }
    else if (pid == 0) {                            /* child */
        if (*type == 'r') {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        } else {
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO) {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }

        /* close all descriptors in childpid[] */
        for (i = 0; i < maxfd; i++)
            if (childpid[ i ] > 0)
                close(i);

        ty_close_all_fd();

        execl(SHELL, "sh", "-c", cmdstring, (char *) 0);
        _exit(127);
    }
                                /* parent */
    if (*type == 'r') {
        close(pfd[1]);
        if ( (fp = fdopen(pfd[0], type)) == NULL)
        {
            printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
            return(NULL);
        }
    } else {
        close(pfd[0]);
        if ( (fp = fdopen(pfd[1], type)) == NULL)
        {
            printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
            return(NULL);
        }
    }
    childpid[fileno(fp)] = pid; /* remember child pid for this fd */

    return(fp);
}

static int ty_pclose(FILE *fp)
{
    int     fd, stat;
    pid_t   pid;

    if (childpid == NULL)
        return(-1);     /* popen() has never been called */

    fd = fileno(fp);
    if ( (pid = childpid[fd]) == 0)
        return(-1);     /* fp wasn't opened by popen() */

    childpid[fd] = 0;
    if (fclose(fp) == EOF)
        return(-1);

    while (waitpid(pid, &stat, 0) < 0)
        if (errno != EINTR)
            return(-1); /* error other than EINTR from waitpid() */

    return(stat);   /* return child's termination status */
}
#endif

int ty_cmd_excute_shell(const char* cmd, char* buffer, int* len)
{
	FILE *fp = NULL;

    fp = ty_popen(cmd, "r");
    if(fp == NULL) {
        perror("");
        printf("%s() line:%d errno = %d\n", __FUNCTION__, __LINE__, errno);
        return -1;
    }

    if (NULL != buffer) {
        if(fgets(buffer, *len, fp) == NULL) {

        }

        //while(fgets(buffer, *len, fp) != NULL) {
            // should do something
            // printf("%s", buffer);
        //}

        int i = 0;

        for(i = 0; i < *len; i++) {
            if(buffer[i] == '\n' || buffer[i] == '\r') {
                buffer[i] = '\0';
                break;
            }
        }

        buffer[*len - 1] = '\0';
        *len = strlen(buffer);
    }

    ty_pclose(fp);
    fp = NULL;

    return 0;
}
#endif
