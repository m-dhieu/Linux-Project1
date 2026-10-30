#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

typedef enum {
    NORMAL,
    MAINTENANCE
} ServiceState;

static volatile sig_atomic_t status_flag = 0;
static volatile sig_atomic_t toggle_flag = 0;
static volatile sig_atomic_t shutdown_flag = 0;
static volatile sig_atomic_t terminate_flag = 0;
static volatile sig_atomic_t monitor_flag = 0;

static void handle_signal(int sig)
{
    switch (sig) {
        case SIGUSR1:
            status_flag = 1;
            break;
        case SIGUSR2:
            toggle_flag = 1;
            break;
        case SIGINT:
            shutdown_flag = 1;
            break;
        case SIGTERM:
            terminate_flag = 1;
            break;
        case SIGALRM:
            monitor_flag = 1;
            break;
    }
}

int main(void)
{
    struct sigaction sa = {0};
    sigset_t block_mask, wait_mask, old_mask;
    ServiceState state = NORMAL;

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1 ||
        sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigaddset(&block_mask, SIGUSR2);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTERM);
    sigaddset(&block_mask, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    wait_mask = old_mask;
    sigdelset(&wait_mask, SIGUSR1);
    sigdelset(&wait_mask, SIGUSR2);
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGTERM);
    sigdelset(&wait_mask, SIGALRM);

    printf("[Server Controller] Started successfully. PID: %d\n",
	getpid());
    printf("[Server Controller] Service running normally...\n");
    fflush(stdout);

    alarm(5);
    while (1) {
        if (terminate_flag) {
            printf("\n[Server Controller] Emergency termination requested (SIGTERM).\n");
            break;
        }
        if (shutdown_flag) {
            printf("\n[Server Controller] Graceful shutdown initiated (SIGINT).\n");
            printf("[Server Controller] Service stopped cleanly.\n");
            break;
        }
        if (toggle_flag) {
            toggle_flag = 0;
            state = (state == NORMAL) ? MAINTENANCE : NORMAL;
            printf("[Server Controller] Mode changed to: %s\n",
                   state == NORMAL ? "NORMAL" : "MAINTENANCE");
            fflush(stdout);
        }
        if (status_flag) {
            status_flag = 0;
            printf("[Status Report] PID: %d | State: %s\n",
                   getpid(),
                   state == NORMAL ? "NORMAL" : "MAINTENANCE");
            fflush(stdout);
        }
        if (monitor_flag) {
            monitor_flag = 0;
            printf("[Server Controller] Service running in %s mode...\n",
                   state == NORMAL ? "NORMAL" : "MAINTENANCE");
            fflush(stdout);
            alarm(5);
        }
        sigsuspend(&wait_mask);
    }

    alarm(0);
    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    return terminate_flag ? EXIT_FAILURE : EXIT_SUCCESS;
}

