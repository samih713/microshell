#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// error messages
#define SYS_ERROR  "error: fatal"
#define ARGS_ERROR "error: cd: bad arguments"
#define PATH_ERROR "error: cd: cannot change directory to "
#define EXEC_ERROR "error: cannot execute "

// command types
#define LAST 0
#define PIPE '|'
#define SEQ  ';'

// pipe in and out
#define IN  0
#define OUT 1

// strlen
size_t ft_strlen(const char *string)
{
	size_t length = 0;
	while(string && string[length])
		length++;
	return length;
}

// error macros
#define error(error_message)                                                   \
	do                                                                     \
	{                                                                      \
		write(STDERR_FILENO, error_message, ft_strlen(error_message)); \
		write(STDERR_FILENO, "\n", 1);                                 \
		return -1;                                                     \
	}                                                                      \
	while(0)

#define error_exit(path, EXEC_ERROR)                                     \
	do                                                               \
	{                                                                \
		write(STDERR_FILENO, EXEC_ERROR, ft_strlen(EXEC_ERROR)); \
		write(STDERR_FILENO, path, ft_strlen(path));             \
		write(STDERR_FILENO, "\n", 1);                           \
		exit(1);                                                 \
	}                                                                \
	while(0)

// close pipe fds
#define close_pipe(pipe_fd)          \
	do                           \
	{                            \
		close(pipe_fd[IN]);  \
		close(pipe_fd[OUT]); \
	}                            \
	while(0)

// cd
void cd(const char *path, int arg_count)
{
	if(arg_count != 2)
		error_exit(0, ARGS_ERROR);
	if(chdir(path) < 0)
	{
		write(STDERR_FILENO, PATH_ERROR, ft_strlen(PATH_ERROR));
		write(STDERR_FILENO, path, ft_strlen(path));
		write(STDERR_FILENO, "\n", 1);
		exit (1);
	}
	exit(0);
}

#include <stdio.h>

int main(int argc, char **argv)
{
	// if(argc < 2)
	// 	return -1;
	// variables
	int  length = 0;
	int  start  = 1;
	int  end    = 1;
	char type   = 0;

	// pipe fds
	int pipe_fd[2];
	int old_fd[2] = { dup(STDIN_FILENO), dup(STDOUT_FILENO) };

	// main loop
	while(argv[end])
	{
		// creating the command
		for(end = start; argv[end] && strcmp(argv[end], "|") && strcmp(argv[end], ";");
		    end++)
			;
		length = end - start;
		type   = (argv[end] ? argv[end][0] : 0);

    (void)argc;
  
		// // if no command return
		// if(length <= 0)
		// 	return 0;
		// else make a command
		char **command = malloc(sizeof(char *) * length + 1);
		if(!command)
			error(SYS_ERROR);
		// set the pointers
		int i = 0;
		for(i = start; i < end; i++)
			command[i - start] = argv[i];
		command[i - start] = 0;

		// execute
		switch(type)
		{
			case PIPE:
				{
					if(pipe(pipe_fd) < 0)
						error(SYS_ERROR);
					if(command[0] && !fork()) // exec out => pipe
					{
						if(dup2(pipe_fd[OUT], STDOUT_FILENO)
						   < 0) // dup2 out
							error(SYS_ERROR);
						close_pipe(pipe_fd);
						if(!strcmp(command[0], "cd"))
							cd(command[0], length);
						else if(execve(command[0], command, NULL))
						{
							if(dup2(STDOUT_FILENO, old_fd[OUT])
							   < 0)
								error(SYS_ERROR);
							error_exit(command[0], EXEC_ERROR);
						}
					}
					else
					{
						waitpid(0, NULL, 0);
						if(dup2(pipe_fd[IN], STDIN_FILENO)
						   < 0) // dup2 in
							error(SYS_ERROR);
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
							error_exit(command[0], EXEC_ERROR);
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
							error_exit(command[0], EXEC_ERROR);
					}
					else
					{
						waitpid(0, NULL, 0);
					}
				}
		}
		// free command
		free(command);
		// move to next command
		start = end + 1;
	}
	return 0;
}
