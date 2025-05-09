#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_pid 256
#define MAX_cmd 256

typedef struct PID_STATE
{
    pid_t pid;
    bool is_state;
} Pid_state;

Pid_state *pid_state;
int pid_count = 0;

void finish_handler(int sig)
{
    int status;
    pid_t wpid;
    while ((wpid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < pid_count; i++)
        {
            if (pid_state[i].pid == wpid)
            {
                if (pid_state[i].is_state)
                {
                    printf("done [%d]\n", wpid);
                    printf("%% ");
                    fflush(stdout);
                }
            }
        }
    }
}

void cor_arg(int size, char *cmd)
{
    // 　空白で区切り
    char *p = strtok(cmd, " ");

    for (int i = 0;; i++)
    {
        cmd[i] = p;
        p = strtok(NULL, " ");
        if (p == NULL)
        {
            size = i;
            break;
        }
    }
}

bool check_bg(char *cmd, int size_cmd){
    if (strncmp(cmd[size_cmd], "&", MAX_cmd) == 0)
    {
        cmd[size_cmd] = NULL;
        return true;
    }
    return false;
}

bool check_innercmd(char *inners[], char cmd){
    if (strncmp(cmd, inners[0], MAX_cmd) == 0)
    {
        //exit
        exit(0);
    } else if (strncmp(cmd, inners[1], MAX_cmd) == 0){
        //quit
        exit(0);
    } else if (strncmp(cmd, inners[2], MAX_cmd) == 0){
        //jobs
        
    } else if (strncmp(cmd, inners[3], MAX_cmd) == 0){
        //fg
    }
    return false;
}

int main(void)
{
    pid_state = malloc(sizeof(Pid_state) * MAX_pid);
    char *innercmds[] = {"exit", "quit", "jobs", "fg"};
    while (1)
    {
        // pid_t pid;
        // プロンプトが早く表示されるため調節
        usleep(10000);
        printf("%% ");
        char enter_cmd[MAX_cmd];
        fgets(enter_cmd, sizeof(enter_cmd), stdin);
        enter_cmd[strlen(enter_cmd) - 1] = '\0'; // 空白削除
        printf("enter_cmd: %s\n", enter_cmd);
        if(enter_cmd == NULL){
            continue;
        }

        int size_cmd = 0;
        char *cmd_execv[MAX_cmd] = {NULL};

        cor_arg(size_cmd, cmd_execv);
        printf("size: %d", size_cmd);

        bool is_bg = check_bg;

        check_innercmd(innercmds, cmd_execv[0]);

        pid_t pid = fork();
        signal(SIGCHLD, finish_handler);
        int status;
        int errno;

        if (pid == 0)
        {
            // 子プロセス
            execv(cmd_execv[0], cmd_execv);
            exit(0);
        }

        if (pid == -1)
        {
            printf("fork error");
        }

        if (!errno)
        {
            printf("errono: %d", errno);
            perror(strerror(errno));
        }

        // 親プロセス
        if (pid > 0)
        {
            pid_count++;
            printf("pid: %d\n", pid);
            pid_state[pid_count - 1].pid = pid;
            pid_state[pid_count - 1].is_state = is_bg;

            if (is_bg)
            {
                printf("==Run Background==\n");
            }
            else
            {
                printf("==Run FG==\n");
                pid = waitpid(-1, &status, 0);
            }
        }
        printf("\n");
    }
    return 0;
}
