#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void ERR_MSG(char *msg)
{
	/**
	 * for printing to stderr and exiting
	 **/
	char *err = strerror(errno);

	fprintf(stderr, "%s\nerrno: %d\nerrno translation: %s\n", msg, errno, err);
	exit(1);
}

void ERR_MSG_WOExit(char *msg)
{
	/**
	 * for printing to stderr
	 **/
	char *err = strerror(errno);

	fprintf(stderr, "%s\nerrno: %d\nerrno translation: %s\n", msg, errno, err);
}
void waitAndCheck(int pid, int *wstatus, int options)
{
	/**
	 * execute waitpid and check the result
	 **/
	if (waitpid(pid, wstatus, options) == -1 && errno != ECHILD && errno != EINTR)
	{
		ERR_MSG("error in waiting for child process");
	}
}

int waitAndCheck_WOExit(int pid, int *wstatus, int options)
{
	/**
	 * execute waitpid and check the result
	 * return 1 iff there is an error
	 **/
	if (waitpid(pid, wstatus, options) == -1 && errno != ECHILD)
	{
		char *err = strerror(errno);
		fprintf(stderr, "error in waiting for child process\nerrno: %d\nerrno translation: %s\n", errno, err);
		return 1;
	}
	return 0;
}

void sigactionAndCheck(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	/**
	 * execute sigaction and check the result
	 **/
	if (sigaction(signum, act, oldact) == -1)
	{
		ERR_MSG("error in changing signal action");
	}
}
int change_sa(int background)
{
	/**
	 * change the sigaction based on the type of command
	 * if command is in background, change its SIGCHLD action
	 * if command is in foreground, change its SIGINT action(because we already changed the shell process' SIGINT)
	 **/
	int signal;
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	if (background)
	{
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_NOCLDWAIT;
		sa.sa_handler = SIG_IGN;
		signal = SIGCHLD;
	}

	if (!background)
	{
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = SIG_DFL;
		signal = SIGINT;
	}

	return sigaction(signal, &sa, NULL);
}

void changesaAndCheck(int background)
{
	/**
	 * chagne the sigaction and check the result
	 **/
	if (change_sa(background) == -1)
	{
		ERR_MSG("error in changing signal action");
	}
}

int isWeird(char **arglist)
{
	/**
	 * check if arglist contains any of the following: |, >, &
	* returns:
	* 1: contains pipe
	* 2: contains background
	* 3: contains redirecting
	* 0: otherwise 
	**/
	char *arg;
	int i = 0;
	while ((arg = arglist[i++]) != NULL)
	{
		if (strcmp(arg, "|") == 0)
			return 1;
		else if (strcmp(arg, "&") == 0)
			return 2;
		else if (strcmp(arg, ">") == 0)
			return 3;
	}

	return 0;
}

int prepare(void)
{
	/**
	 * change the SIGINT handling to do nothing
	 **/
	struct sigaction sigint;

	sigint.sa_handler = SIG_IGN;
	sigemptyset(&sigint.sa_mask);
	sigactionAndCheck(SIGINT, &sigint, NULL);

	return 0;
}
int finalize(void)
{
	/**
	 * this one's for you guys :>
	 **/
	return 0;
}

int handlePipe(int count, char **arglist)
{
	/**
	 * handle the behavior of a command with pipe (|)
	 **/
	int pipe_index = 0;
	while (strcmp(arglist[++pipe_index], "|")) /*check where the pipe is*/
		;
	arglist[pipe_index] = NULL;						/* "cut" the list in the index of the pipe, giving "2 lists" */
	char **write_command = arglist;					/* command until pipe */
	char **read_command = arglist + pipe_index + 1; /* from pipe to the end */

	int pipefds[2];
	if (pipe(pipefds) == -1)
	{
		ERR_MSG("error creating pipe");
	}
	int read_fds = pipefds[0];
	int write_fds = pipefds[1];

	int rpid; /* reading process */
	if ((rpid = fork()) == -1)
	{
		ERR_MSG_WOExit("fork failed");
		return 0;
	}
	int wpid = 0; /* writing process */
	if (rpid == 0)
	{
		changesaAndCheck(0);
		if (close(write_fds) == -1)
		{
			fprintf(stderr, "\n\nhere %d\n", 0);
			ERR_MSG("error in closing writing end of pipe");
		}
		if (dup2(read_fds, 0) == -1)
		{
			ERR_MSG("error in connecting reading end of pipe to stdin");
		}
		if (close(read_fds) == -1)
		{
			fprintf(stderr, "\n\nhere %d\n", 1);
			ERR_MSG("error in closing reading end of pipe");
		}
		execvp(read_command[0], read_command);
		ERR_MSG("error in executing command");
	}
	else
	{
		if ((wpid = fork()) == -1) /* creating the writing process */
		{
			ERR_MSG_WOExit("fork failed");
			return 0;
		}
		if (wpid == 0)
		{
			changesaAndCheck(0);
			if (close(read_fds) == -1)
			{
				fprintf(stderr, "\n\nhere %d\n", 2);
				ERR_MSG("error in closing reading end of pipe");
			}
			if (dup2(write_fds, 1) == -1)
			{
				fprintf(stderr, "\n\nhere %d\n", 3);
				ERR_MSG("error in connecting writimg end of pipe to stdout");
			}
			if (close(write_fds) == -1)
			{
				fprintf(stderr, "\n\nhere %d\n", 4);
				ERR_MSG("error in closing writing end of pipe");
			}
			execvp(write_command[0], write_command);
			ERR_MSG("error in executing command");
		}
		else
		{
			/* closing the file descriptors */
			if (close(read_fds) == -1)
			{
				fprintf(stderr, "\n\nhere %d\n", 5);
				ERR_MSG_WOExit("error in closing reading end of pipe");
				return 0;
			}
			if (close(write_fds) == -1)
			{
				fprintf(stderr, "\n\nhere %d\n", 6);
				ERR_MSG_WOExit("error in closing writing end of pipe");
				return 0;
			}
			if (waitAndCheck_WOExit(rpid, NULL, 0) || waitAndCheck_WOExit(wpid, NULL, 0))
			{
				return 0;
			}
		}
	}

	return 1;
}
int handleBackground(int count, char **arglist)
{
	/**
	 * handle the behavior of command in background (&)
	 **/

	/* change the SIGCHLD to not create zombies */
	changesaAndCheck(1);
	int pid;
	if ((pid = fork()) == -1)
	{
		ERR_MSG_WOExit("fork failed");
		return 0;
	}
	if (pid == 0)
	{
		/* remove the ampersand */
		arglist[count - 1] = NULL;
		execvp(arglist[0], arglist);

		ERR_MSG("error in executing program");
	}
	/* no need to wait, because we want to continue */
	return 1;
}
int handleRedirect(int count, char **arglist)
{
	/**
	 * handle the behavior of commands with output redirection (>)
	 **/

	/* the file is in the last cell of the arglist */
	char *file = arglist[count - 1];

	/* cut the arglist in the location of the > */
	arglist[count - 2] = NULL;

	int pid;
	if ((pid = fork()) == -1)
	{
		ERR_MSG_WOExit("fork failed");
		return 0;
	}
	if (pid == 0)
	{
		changesaAndCheck(0);
		/* open the file, or create if it does not exist with permission of rw-rw-r-- */
		int fd = open(file, O_CREAT | O_WRONLY, 0664);
		if (fd == -1)
		{
			ERR_MSG("error in connecting to wanted file");
		}
		if (dup2(fd, 1) == -1)
		{
			close(fd);
			ERR_MSG("error in dup (trying to connect to stdout)");
		}
		close(fd);
		execvp(arglist[0], arglist);
		ERR_MSG("error in executing program");
	}
	else
	{
		if (waitAndCheck_WOExit(pid, NULL, 0))
		{
			return 0;
		}
	}
	return 1;
}
int process_arglist(int count, char **arglist)
{
	/**
	 * check what the arglist contains and act accordingly
	 **/
	int weird = isWeird(arglist);
	switch (weird)
	{
	case 1:
		return handlePipe(count, arglist);
	case 2:
		return handleBackground(count, arglist);
	case 3:
		return handleRedirect(count, arglist);
	default:
		break;
	}

	/* handle normal*/
	int pid;
	if ((pid = fork()) == -1)
	{
		ERR_MSG_WOExit("fork failed");
		return 0;
	}

	if (pid == 0)
	{
		changesaAndCheck(0);
		execvp(arglist[0], arglist);
		ERR_MSG("error in executing program");
	}
	else
	{
		if (waitAndCheck_WOExit(pid, NULL, 0))
		{
			return 0;
		}
	}

	return 1;
}