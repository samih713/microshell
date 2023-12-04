#include <fcntl.h>  // open, close,
#include <signal.h> // signal
#include <stdbool.h>
#include <stdlib.h> // malloc, free,
#include <string.h> // strcmp, strncmp
#include <sys/types.h>
#include <sys/wait.h> // waitpid
#include <unistd.h>   // fork, write, exit, chdir, execve, dup, dup2, pipe
// testing
#include <stdio.h>
// testing

//---------------------------------------------------------------------------
//** error handling       ---------------------------------------------------
//---------------------------------------------------------------------------

#define FAIL 1
#define SUCC 0

#define CD_ARGS "error: cd: bad arguments"
#define SYS_ERR "error: fatal"

#define error(error_msg)                                                       \
    do                                                                         \
    {                                                                          \
        write(STDERR_FILENO, error_msg, ft_strlen(error_msg));                 \
        write(STDERR_FILENO, "\n", 1);                                         \
        return -1;                                                             \
    } while (false)
#define error_exit(path)                                                       \
    do                                                                         \
    {                                                                          \
        write(STDERR_FILENO, "error: cannot execute ", 22);                    \
        write(STDERR_FILENO, path, ft_strlen(path));                           \
        write(STDERR_FILENO, "\n", 1);                                         \
        exit(WEXITSTATUS(exit_status));                                        \
        return -1;                                                             \
    } while (false)

//---------------------------------------------------------------------------
//** command             ----------------------------------------------------
//---------------------------------------------------------------------------

int cmd[3] = { 1, 1, 1 };

void get_cmd(char **argv);

#define START 0
#define END   1
#define LEN   2

#define LAST 0
#define PIPE '|'
#define SEQ  ';'

typedef struct _sc
{
        int         start;
        int         end;
        int         len;
        int         type;
        char      **args;
        struct _sc *next;
} simple_cmd;

// pipe fds
#define IN  0
#define OUT 1

// exit_status
int exit_status = 0;

// built-ins
int cd(const char *path);
// utils
size_t ft_strlen(const char *s);
// close pipe
#define close_pipe(fd)                                                         \
    do                                                                         \
    {                                                                          \
        close((fd)[OUT]);                                                      \
        close((fd)[IN]);                                                       \
    } while (false)

int main(int argc, char **argv, char **envp)
{
    (void)argc;
    // parse
    simple_cmd *prev = NULL;
    simple_cmd *head = NULL;

    while (argv[cmd[END]])
    {
        get_cmd(argv);
        simple_cmd *cur = malloc(sizeof(simple_cmd));
        cur->start = cmd[START];
        cur->end = cmd[END];
        cur->len = cmd[END] - cmd[START] + 1;
        cur->type = (argv[cmd[END]] ? argv[cmd[END]][0] : LAST);
        cur->next = NULL;
        if (prev)
            prev->next = cur;
        else
            head = cur;
        prev = cur;
    }

    // execute
    for (simple_cmd *temp = head; temp; temp = temp->next)
    {
        // create argv to pass to exec
        temp->args = malloc(sizeof(char *) * temp->len + 1);
        int i = 0;
        for (int i = temp->start; i < temp->end; i++)
            temp->args[i - temp->start] = argv[i];
        temp->args[i - temp->start] = NULL;

        // execve
        int pipe_fd[2];
        int old_fd[2] = { dup(STDIN_FILENO),
                          dup(STDOUT_FILENO) }; // error check
#if 1
        switch (temp->type)
        {
            case PIPE:
            {
                if (pipe(pipe_fd) < 0)
                    error(SYS_ERR);
                if (temp->args[0] && !fork())
                {
                    if (dup2(pipe_fd[OUT], STDOUT_FILENO) < 0)
                        error(SYS_ERR);
                    close_pipe(pipe_fd);
                    if (execve(temp->args[0], temp->args, envp))
                        error_exit(temp->args[0]);
                }
                else
                {
                    wait(&exit_status);
                    if (dup2(pipe_fd[IN], STDIN_FILENO) < 0)
                        error(SYS_ERR);
                    close_pipe(pipe_fd);
                }
                break;
            }
            case SEQ:
            {
                // reset fds
                if (dup2(STDOUT_FILENO, old_fd[OUT]) < 0)
                    error(SYS_ERR);
                if (dup2(STDIN_FILENO, old_fd[IN]) < 0)
                    error(SYS_ERR);
                if (temp->args[0] && !fork())
                {
                    if (execve(temp->args[0], temp->args, envp))
                        error_exit(temp->args[0]);
                }
                else
                    wait(&exit_status);
                break;
            }
            default:
            {
                if (temp->args[0] && !fork())
                {
                    if (execve(temp->args[0], temp->args, envp))
                        error_exit(temp->args[0]);
                }
                else
                    wait(&exit_status);
            }
        }
#endif
    }
    return SUCC;
}

// helper functions
size_t ft_strlen(const char *s)
{
    size_t len = 0;
    while (s && s[len])
        len++;
    return len;
}

int cd(const char *path)
{
    if (chdir(path) < 0)
    {
        write(STDERR_FILENO, "error: cd: cannot change directory to ", 38);
        write(STDERR_FILENO, path, ft_strlen(path));
        write(STDERR_FILENO, "\n", 1);
        return (FAIL);
    }
    return (SUCC);
}

// {1, 1, 1}
void get_cmd(char **argv)
{

    if ((argv[cmd[END]] != 0 && cmd[END] != cmd[START]) || !cmd[LEN])
        cmd[START] = cmd[END] + 1;

    size_t tok_len = ft_strlen(argv[cmd[END]]);

    for (cmd[END] = cmd[START];
         argv[cmd[END]] && strncmp(argv[cmd[END]], "|", tok_len) &&
         strncmp(argv[cmd[END]], ";", tok_len);
         cmd[END]++)

        tok_len = ft_strlen(argv[cmd[END]]);

    ;
    cmd[LEN] = cmd[END] - cmd[START];
}
