/**
 * Copyright © 2015  Mattias Andrée <maandree@member.fsf.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "daemon.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>



/**
 * The common beginning of for all daemon pathnames.
 */
#define DAEMON_PREFIX  LIBEXECDIR "/" PACKAGE "/satd-"



/**
 * Signal that has been received, 0 if none.
 */
static volatile sig_atomic_t received_signo = 0;



/**
 * Invoked when a signal is received.
 * 
 * @param  int  The signal.
 */
static void sighandler(int signo)
{
	int saved_errno = errno;
	if (signo == SIGCHLD)
		waitpid(-1, NULL, WNOHANG);
	else
		received_signo = (sig_atomic_t)signo;
	errno = saved_errno;
}



/**
 * The sat daemon.
 * 
 * @param   argc  Should be 3.
 * @param   argv  The name of the process, the pathname of the socket,
 *                and the pathname to the state file.
 * @param   envp  The environment.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 */
int
main(int argc, char *argv[], char *envp[])
{
	int fd = -1, rc = 0;
	pid_t pid;
	char type;
	const char *image;

	/* Set up signal handlers. */
	if (signal(SIGHUP,  sighandler) == SIG_ERR)  goto fail;
	if (signal(SIGCHLD, sighandler) == SIG_ERR)  goto fail;

	/* The magnificent loop. */
accept_again:
	/* TODO run jobs */
	if (received_signo == SIGHUP) {
		execve(DAEMON_PREFIX "diminished", argv, envp);
		perror(argv[0]);
	}
	received_signo = 0;
	if (fd = accept(SOCK_FILENO, NULL, NULL), fd == -1) {
		switch (errno) {
		case ECONNABORTED:
		case EINTR:
			goto accept_again;
		default:
			/* Including EMFILE, ENFILE, and ENOMEM
			 * because of potential resource leak. */
			goto fail;
		}
	}
	if (read(fd, &type, sizeof(char)) <= 0) {
		perror(argv[0]);
		goto connection_done;
	}
fork_again:
	switch ((pid = fork())) {
	case -1:
		if (errno != EAGAIN)
			goto fail;
		(void) sleep(1); /* Possibly shorter because of SIGCHLD. */
		goto fork_again;
	case 0:
		switch (type) {
		case SAT_QUEUE:   image = DAEMON_PREFIX "add";   break;
		case SAT_REMOVE:  image = DAEMON_PREFIX "rm";    break;
		case SAT_PRINT:   image = DAEMON_PREFIX "list";  break;
		case SAT_RUN:     image = DAEMON_PREFIX "run";   break;
		default:
			fprintf(stderr, "%s: invalid command received.\n", argv[0]);
			exit(1);
		}
		if (dup2(fd, SOCK_FILENO) != -1)
			close(fd), fd = SOCK_FILENO, execve(image, argv, envp);
		perror(argv[0]);
		close(fd);
		exit(1);
	default:
		break;
	}
connection_done:
	close(fd), fd = -1;
	goto accept_again;

done:
	unlink(argv[1]);
	if (!rc)
		unlink(argv[2]);
	close(SOCK_FILENO);
	close(STATE_FILENO);
	return 0;

fail:
	perror(argv[0]);
	if (fd >= 0)
		close(fd);
	rc = 1;
	goto done;

	(void) argc;
}

