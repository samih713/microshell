#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// error-messages
#define ERR_SYS  "error: fatal"
#define ERR_ARGS "error: cd: bad arguments"
#define ERR_PATH "error: cd: cannot change directory to " // + path
#define ERR_EXEC "error: cannot execute "                 // + command
// constans-commands
#define LAST 0
#define PIPE '|'
#define SEQ  ';'
// constants-pipe_fds
#define IN  0
#define OUT 1
// helper-functions
size_t ft_strlen(const char *string)
{
	size_t length = 0;
	while(string && string[length])
		length++;
	return length;
}
// error-macros
#define error(error_message, path, bail)                                       \
	do                                                                     \
	{                                                                      \
		write(STDERR_FILENO, error_message, ft_strlen(error_message)); \
		if(path)                                                       \
			write(STDERR_FILENO, path, ft_strlen(path));           \
		write(STDERR_FILENO, "\n", 1);                                 \
		if(bail)                                                       \
			exit(bail);                                            \
		else                                                           \
			return (-1);                                           \
	}                                                                      \
	while(0)
// close-pipes
#define close_pipe(pipe_fd)          \
	do                           \
	{                            \
		close(pipe_fd[IN]);  \
		close(pipe_fd[OUT]); \
	}                            \
	while(0)
// cd-thefunction
int cd(const char *path, size_t arg_count)
{
	if(arg_count != 2)
		error(ERR_ARGS, 0, -1);
	if(chdir(path) < 0)
		error(ERR_PATH, path, -1);
	return (0);
}

int main(int argc, char **argv)
{
	(void)argc;
	// variables
	int  start  = 1;
	int  end    = 1;
	int  length = 0;
	char type   = 0;
	// pipe_fds
	int pipe_fd[2];
	int old_fd[2] = { dup(STDIN_FILENO), dup(STDOUT_FILENO) };
	// big man loop
	while(argv[end])
	{
		// parse
		for(end = start; argv[end] && strcmp(argv[end], "|") && strcmp(argv[end], ";");
		    end++)
			;
		length = end - start;
		type   = (argv[end] ? argv[end][0] : LAST);
		// if no command return
		// if(length <= 0)
		// 	return -1;
		// else make a command
		int    i       = 0;
		char **command = malloc(sizeof(char *) * length + 1);
		if(!command)
			return -1;
		for(i = start; i < end; i++)
			command[i - start] = argv[i];
		command[i - start] = 0;
		// execute
		switch(type)
		{
			case PIPE:
				{
					if(pipe(pipe_fd) < 0)
						error(ERR_SYS, 0, 0);
					if(command[0] && !fork())
					{
						if(dup2(pipe_fd[OUT], STDOUT_FILENO) < 0)
							error(ERR_SYS, 0, 0);
						close_pipe(pipe_fd);
						if(!strcmp(command[0], "cd"))
							cd(command[0], length);
						else if(execve(command[0], command, NULL))
						{
							if(dup2(STDOUT_FILENO, old_fd[OUT]) < 0)
								error(ERR_SYS, 0, 0);
							error(ERR_EXEC, command[0], -1);
						}
					}
					else
					{
						waitpid(0, NULL, 0);
						if(dup2(pipe_fd[IN], STDIN_FILENO)
						   < 0) // dup2 in
							error(ERR_SYS, 0, 0);
						close_pipe(pipe_fd);
					}
					break;
				}
			case SEQ:
				{
					if(command[0] && !fork())
					{
						if(!strcmp(command[0], "cd"))
							cd(command[0], length);
						else if(execve(command[0], command, NULL))
							error(ERR_EXEC, command[0], -1);
					}
					else
					{
						waitpid(0, NULL, 0);
					}
					break;
				}
			default:
				{
					if(command[0] && !fork())
					{
						if(!strcmp(command[0], "cd"))
							cd(command[0], length);
						else if(execve(command[0], command, NULL))
							error(ERR_EXEC, command[0], -1);
					}
					else
					{
						waitpid(0, NULL, 0);
					}
				}
		}
    free(command);
    start = end + 1;
	}
}
