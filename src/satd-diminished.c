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
#include <stdint.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/timerfd.h>



/**
 * Macro that adds the common beginning of for
 * all daemon pathnames to a string literal.
 * 
 * @param  name:string literal  The unique part of the pathname.
 */
#define DAEMON_IMAGE(name)  LIBEXECDIR "/" PACKAGE "/satd-" name

/**
 * The value on `timer_pid` when there is not "timer" image running.
 */
#define NO_TIMER_SPAWNED  -2  /* Must be negative, but not -1. */



/**
 * Signal that has been received, 0 if none.
 */
static volatile sig_atomic_t received_signo = SIGCHLD; /* Forces the "timer" image to run. */

/**
 * The PID the "timer" fork, `NO_TIMER_SPAWNED` if not running.
 * 
 * It does not matter that this is lost on a successful SIGHUP.
 */
static volatile pid_t timer_pid = NO_TIMER_SPAWNED;



/**
 * Invoked when a signal is received.
 * 
 * @param  int  The signal.
 */
static void
sighandler(int signo)
{
	int saved_errno = errno;
	pid_t pid;
	if (signo == SIGCHLD) {
		while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
			if (pid == timer_pid)
				timer_pid = NO_TIMER_SPAWNED;
			else
				received_signo = (sig_atomic_t)signo;
		}
	} else {
		received_signo = (sig_atomic_t)signo;
	}
	errno = saved_errno;
}


/**
 * Spawn a libexec.
 * 
 * @param   command  The command to spawn, -1 for "timer".
 * @param   fd       File descriptor to the socket, -1 iff `command` is -1.
 * @param   argv     `argv` from `main`.
 * @param   envp     `envp` from `main`.
 * @return           0 on success, -1 on error.
 */
static int
spawn(int command, int fd, char *argv[], char *envp[])
{
	const char *image;
	pid_t pid;

fork_again:
	switch ((pid = fork())) {
	case -1:
		if (errno != EAGAIN)
			return -1;
		(void) sleep(1); /* Possibly shorter because of SIGCHLD. */
		goto fork_again;
	case 0:
		switch (command) {
		case SAT_QUEUE:   image = DAEMON_IMAGE("add");    break;
		case SAT_REMOVE:  image = DAEMON_IMAGE("rm");     break;
		case SAT_PRINT:   image = DAEMON_IMAGE("list");   break;
		case SAT_RUN:     image = DAEMON_IMAGE("run");    break;
		case -1:          image = DAEMON_IMAGE("timer");  break;
		default:
			fprintf(stderr, "%s: invalid command received.\n", argv[0]);
			goto child_fail;
		}
		if (command < 0) {
			close(SOCK_FILENO), close(LOCK_FILENO);
			execve(image, argv, envp);
		} else {
			close(BOOT_FILENO), close(REAL_FILENO), close(LOCK_FILENO);
			if (dup2(fd, SOCK_FILENO) != -1)
				close(fd), fd = SOCK_FILENO, execve(image, argv, envp);
		}
		perror(argv[0]);
	child_fail:
		close(fd);
		exit(1);
	default:
		if (command < 0)
			timer_pid = pid;
		return 0;
	}
}


/**
 * Determine whether a timer is set.
 * 
 * @param   fd  The file descriptor of the timer.
 * @return      1 if set, 0 if unset, -1 on error.
 */
static int
is_timer_set(int fd)
{
	struct itimerspec spec;
	if (timerfd_gettime(fd, &spec))
		return -1;
	return (spec.it_interval.tv_sec  || spec.it_value.tv_sec ||
		spec.it_interval.tv_nsec || spec.it_value.tv_nsec);
}


/**
 * If a timer has expired, unset it.
 * 
 * @param  fd     The file descriptor of the timer.
 * @param  fdset  Set that shall contain `fd` iff it has expired.
 * @return        0 on sucess, -1 on error.
 */
static int
test_timer(int fd, const fd_set *fdset)
{
	int64_t _overrun;
	struct itimerspec spec = {
		.it_interval.tv_sec  = 0, .it_value.tv_sec  = 0,
		.it_interval.tv_nsec = 0, .it_value.tv_nsec = 0,
	};
	if (!FD_ISSET(BOOT_FILENO, fdset))                return 0;
	if (read(BOOT_FILENO, &_overrun, (size_t)8) < 8)  return -1;
	if (timer_pid == NO_TIMER_SPAWNED)                return 0;
	return timerfd_settime(fd, TFD_TIMER_ABSTIME, &spec, NULL);
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
	int fd = -1, rc = 0, accepted = 0, r;
	unsigned char type;
	fd_set fdset;
	struct stat attr;

	/* Set up signal handlers. */
	t (signal(SIGHUP,  sighandler) == SIG_ERR);
	t (signal(SIGCHLD, sighandler) == SIG_ERR);

	/* The magnificent loop. */
again:
#if 0 || !defined(DEBUG)
	if (accepted && (timer_pid == NO_TIMER_SPAWNED)) {
		t (r = is_timer_set(BOOT_FILENO), r < 0);  if (r) goto not_done;
		t (r = is_timer_set(REAL_FILENO), r < 0);  if (r) goto not_done;
		t (fstat(STATE_FILENO, &attr));
		if (attr.st_size > (off_t)sizeof(size_t)) {
			t (spawn(-1, -1, argv, envp));
			goto not_done;
		}
		goto done;
	 }
not_done:
#endif
	if (received_signo == SIGHUP) {
		execve(DAEMON_IMAGE("diminished"), argv, envp);
		perror(argv[0]);
	} else if ((received_signo == SIGCHLD) && (timer_pid == NO_TIMER_SPAWNED)) {
		t (spawn(-1, -1, argv, envp));
	}
	received_signo = 0;
	FD_ZERO(&fdset);
	FD_SET(SOCK_FILENO, &fdset);
	// FIXME ---- FD_SET(BOOT_FILENO, &fdset);
	// FIXME ---- FD_SET(REAL_FILENO, &fdset); /* This is the highest one. */
	if (select(REAL_FILENO + 1, &fdset, NULL, NULL, NULL) == -1)
		t (errno != EINTR);
	t (test_timer(BOOT_FILENO, &fdset));
	t (test_timer(REAL_FILENO, &fdset));
	if (!FD_ISSET(SOCK_FILENO, &fdset))
		goto again;
	if (fd = accept(SOCK_FILENO, NULL, NULL), fd == -1) {
		switch (errno) {
		case ECONNABORTED:
		case EINTR:
			goto again;
		default:
			/* Including EMFILE, ENFILE, and ENOMEM
			 * because of potential resource leak. */
			goto fail;
		}
	}
	accepted = 1;
	if (read(fd, &type, sizeof(type)) <= 0)
		perror(argv[0]);
	else
		t (spawn((int)type, fd, argv, envp));
	close(fd), fd = -1;
	goto again;

fail:
	perror(argv[0]);
	if (fd >= 0)
		close(fd);
	rc = 1;
done:
	while (waitpid(-1, NULL, 0) > 0);
	unlink(argv[1]);
	if (!rc)
		unlink(argv[2]);
	close(SOCK_FILENO);
	close(STATE_FILENO);
	return rc;

	(void) argc;
}

