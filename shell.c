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
    char pid_name[MAX_cmd];
} Pid_state;

Pid_state *pid_state;
int pid_count = 0;
pid_t fg_pid;

void finish_pid(pid_t pid)
{
    for (int i = 0; i < pid_count; i++)
    {
        if (pid == pid_state[i].pid)
        {
            for (int j = i; j < pid_count; j++)
            {
                pid_state[j].pid = pid_state[j + 1].pid;
                pid_state[j].is_state = pid_state[j + 1].is_state;
                strncmp(pid_state[j].pid_name, pid_state[j + 1].pid_name, strlen(pid_state[j + 1].pid_name));
            }
        }
    }
    pid_count--;
}

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
                finish_pid(wpid);
            }
        }
    }
}

void kill_handler(int sig)
{
    kill(fg_pid, sig);
}

void cor_arg(int *size, char *cmd[], char enter_cmd[])
{
    char *p = strtok(enter_cmd, " ");
    int i = 0;
    for (i = 0; p; i++)
    {
        cmd[i] = p;
        p = strtok(NULL, " ");
    }
    *size = i;
}

void cm_job()
{
    for (int i = 0; i < pid_count; i++)
    {
        if (pid_state[i].is_state)
        {
            printf("pid: %d    %s\n", pid_state[i].pid, pid_state[i].pid_name);
        }
    }
}

void cm_fg(char *cmd[])
{
    int pid = (int)strtol(cmd[strlen(*cmd) - 1], NULL, 10);
    int status;
    for (int i = 0; i < pid_count; i++)
    {
        if (pid_state[i].is_state && pid_state[i].pid == pid)
        {
            pid_state[i].is_state = false;
        }
    }
    waitpid(pid, &status, 0);
}

bool check_bg(char *cmd[], int size_cmd)
{
    if (strncmp(cmd[size_cmd - 1], "&", 1) == 0)
    {
        cmd[size_cmd - 1] = NULL;
        return true;
    }
    return false;
}

bool check_innercmd(char *inners[], char *cmd[])
{
    if (strncmp(cmd[0], inners[0], strlen(inners[1])) == 0)
    {
        // exit
        exit(0);
    }
    else if (strncmp(cmd[0], inners[1], strlen(inners[1])) == 0)
    {
        // quit
        exit(0);
    }
    else if (strncmp(cmd[0], inners[2], strlen(inners[2])) == 0)
    {
        // jobs
        cm_job();
        return true;
    }
    else if (strncmp(cmd[0], inners[3], strlen(inners[3])) == 0)
    {
        // fg
        cm_fg(cmd);
        return true;
    }
    return false;
}

int main(void)
{
    pid_state = malloc(sizeof(Pid_state) * MAX_pid);
    char *innercmds[] = {"exit", "quit", "jobs", "fg"};
    char *env_list;

    while (1)
    {
        signal(SIGCHLD, finish_handler);
        signal(SIGINT, kill_handler);

        // プロンプトが早く表示されるため調節
        usleep(10000);
        printf("%% ");
        char enter_cmd[MAX_cmd] = {0};
        char tmp_cmd[MAX_cmd] = {0};
        fgets(enter_cmd, MAX_cmd, stdin);
        if (strncmp(enter_cmd, "\n", 2) == 0)
        {
            continue;
        }
        enter_cmd[strlen(enter_cmd) - 1] = '\0'; // 空白削除

        strncpy(tmp_cmd, enter_cmd, strlen(enter_cmd));

        int size_cmd = 0;
        char *cmd_execv[MAX_cmd] = {0};

        cor_arg(&size_cmd, cmd_execv, enter_cmd);

        bool is_bg = check_bg(cmd_execv, size_cmd);

        if (check_innercmd(innercmds, cmd_execv))
        {
            continue;
        }

        pid_t pid = fork();
        int status;
        int errno;

        if (pid == 0)
        {
            // 子プロセス
            etpgid(0, 0);
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
            char tmp[MAX_cmd];

            pid_count++;
            printf("pid: %d    procss_name: %s\n", pid, tmp_cmd);
            pid_state[pid_count - 1].pid = pid;
            pid_state[pid_count - 1].is_state = is_bg;
            strncpy(pid_state[pid_count - 1].pid_name, tmp_cmd, strlen(tmp_cmd));

            if (is_bg)
            {
                printf("==Run Background==\n");
            }
            else
            {
                printf("==Run FG==\n");
                fg_pid = pid;
                pid = waitpid(-1, &status, 0);
            }
        }
        printf("\n");
    }
    return 0;
}
