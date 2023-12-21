#include <fcntl.h>  // open, close,
#include <signal.h> // signal
#include <stdbool.h>
#include <stdlib.h> // malloc, free,
#include <string.h> // strcmp, strncmp
#include <sys/types.h>
#include <sys/wait.h> // waitpid
#include <unistd.h>

#define FAIL -1
#define SUC  0

#define CD_ERR  "error: cd: bad arguments"
#define SYS_ERR "error: fatal"

#define START 0
#define END   1
#define LEN   2

#define LAST 0
#define PIPE '|'
#define SEQ  ';'

int cmd[3] = { 1, 1, 0 };

typedef struct _sc
{
	int         start;
	int         end;
	int         len;
	int         type;
	char      **args;
	struct _sc *next;
} simple_command;

#define error_exit(path)                                            \
	do                                                          \
	{                                                           \
		write(STDERR_FILENO, "error: cannot execute ", 22); \
		write(STDERR_FILENO, path, ft_strlen(path));        \
		write(STDERR_FILENO, "\n", 1);                      \
		exit(FAIL);                     \
	}                                                           \
	while(false)

#define error(error_msg)                                            \
	do                                                          \
	{                                                           \
		write(STDERR_FILENO, error_msg, strlen(error_msg)); \
		write(STDERR_FILENO, "\n", 1);                      \
		return (FAIL);                                      \
	}                                                           \
	while(false)

#define exit_error(error_msg)                                                              \
	do                                                                                 \
	{                                                                                  \
		write(STDERR_FILENO, "error: cannot execute ", 22);                        \
		write(STDERR_FILENO, path, ft_strlen(path)) write(STDERR_FILENO, "\n", 1); \
		return (FAIL);                                                             \
	}                                                                                  \
	while(false)

size_t ft_strlen(const char *str)
{
	size_t len = 0;
	while(str && str[len])
		len++;
	return (len);
}

int cd(simple_command *cmd)
{
	if(cmd->len - 1 != 2)
		error(CD_ERR);
	if(chdir(cmd->args[1]) < 0)
	{
		write(STDERR_FILENO, "error: cd: cannot change directory to ", 38);
		write(STDERR_FILENO, cmd->args[1], ft_strlen(cmd->args[1]));
		write(STDERR_FILENO, "\n", 1);
		return (FAIL);
	}
	return (SUC);
}

simple_command *get_cmd(char **argv)
{
	if(cmd[LEN] > 0)
		cmd[START] = cmd[END] + 1;
	for(cmd[END] = cmd[START];
	    argv[cmd[END]] && strcmp(argv[cmd[END]], "|") && strcmp(argv[cmd[END]], ";");
	    cmd[END]++)
		;
	cmd[LEN] = cmd[END] - cmd[START] + 1;
	if(cmd[LEN] == 1)
		return NULL;

	simple_command *command;
	if(!(command = malloc(sizeof(simple_command))))
		return NULL;
	command->start = cmd[START];
	command->end   = cmd[END];
	command->next  = NULL;
	command->args  = malloc(sizeof(char *) * cmd[LEN]);
	command->type = (argv[cmd[END]] ? argv[cmd[END]][0] : LAST);
	if(!command->args)
		return NULL;
	for(int i = cmd[START]; i < cmd[END]; i++)
		command->args[i - cmd[START]] = argv[i];
	command->args[cmd[LEN] - 1] = NULL;
	return (command);
}

simple_command *generate_list(char **argv)
{
	simple_command *prev = NULL;
	simple_command *head = NULL;
	while(argv[cmd[END]])
	{
		simple_command *cur = get_cmd(argv);
		if(prev)
			prev->next = cur;
		else
			head = cur;
		prev = cur;
	}
	return (head);
}

#define IN  0
#define OUT 1
#define close_pipe(pipe_fd)          \
	do                           \
	{                            \
		close(pipe_fd[IN]);  \
		close(pipe_fd[OUT]); \
	}                            \
	while(false)

#include <stdio.h>
int main(int argc, char **argv, char **envp)
{
	(void)argc;
	simple_command *head = generate_list(argv);
	int             pipe_fd[2];
	int             old_fd[2] = {dup(STDIN_FILENO), dup(STDOUT_FILENO)};
	for(simple_command *current = head; current; current = current->next)
	{
		switch(current->type)
		{
			case PIPE: {
				if(pipe(pipe_fd) < 0)
					error(SYS_ERR);
				if(current->args[0] && !fork())
				{
					if(dup2(pipe_fd[OUT], STDOUT_FILENO) < 0)
						error(SYS_ERR);
					close_pipe(pipe_fd);
					if(current->args && !strcmp(current->args[0], "cd"))
						cd(current);
					else if(execve(current->args[0], current->args, envp))
						error_exit(current->args[0]);
				}
				else
				{
					wait(NULL);
					if(dup2(pipe_fd[IN], STDIN_FILENO) < 0)
						error(SYS_ERR);
					close_pipe(pipe_fd);
				}
				break;
			}
			case SEQ:{
				if(dup2(STDIN_FILENO, old_fd[IN]) < 0)
					error(SYS_ERR);
				if(dup2(STDOUT_FILENO, old_fd[OUT]) < 0)
					error(SYS_ERR);
				if(current->args && !strcmp(current->args[0], "cd"))
					cd(current);
				else if(current->args && !fork())
				{
					if(execve(current->args[0], current->args, envp))
						error_exit(current->args[0]);
				}
				else
					wait(NULL);
				break;
			}

			default: {
				if(current->args && !fork())
				{
					if(current->args[0] && !strcmp(current->args[0], "cd"))
						cd(current);
					else if(execve(current->args[0], current->args, envp))
						error_exit(current->args[0]);
				}
				else
					wait(NULL);
			}
		}
	}
	return (SUC);
}

#if 0
int main(int argc, char **argv, char **envp)
{
	simple_command *head = generate_list(argv);
	for(simple_command *cur = head; cur; cur = cur->next)
	{
		printf("-----------------------------------------\n");
		for(int i = cur->start; i < cur->end; i++)
			printf("[%d] %s\n", i, cur->args[i - cur->start]);
	}
}
#endif // printing
