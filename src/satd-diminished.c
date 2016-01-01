/**
 * Copyright © 2015, 2016  Mattias Andrée <maandree@member.fsf.org>
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
#include "common.h"
#include <sys/wait.h>



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
 * Child counter.
 */
static volatile pid_t child_count = 0;

/**
 * Timer specification for an unset timer.
 */
static const struct itimerspec nilspec = {
	.it_interval.tv_sec  = 0, .it_value.tv_sec  = 0,
	.it_interval.tv_nsec = 0, .it_value.tv_nsec = 0,
};



/**
 * Invoked when a signal is received.
 * 
 * @param  int  The signal.
 */
static void
sighandler(int signo)
{
	int no_reap = 1, saved_errno = errno;
	pid_t pid;
	if (signo == SIGCHLD) {
		for (; (pid = waitpid(-1, NULL, WNOHANG)) > 0; child_count--) {
			no_reap = 0;
			if (pid == timer_pid)
				timer_pid = NO_TIMER_SPAWNED;
			else
				received_signo = (sig_atomic_t)signo;
		}
	}
	if (no_reap) /* Phony SIGCHLD or not SIGCHLD at all. */
		received_signo = (sig_atomic_t)signo;
	errno = saved_errno;
}


/**
 * Spawn a satd-timer.
 * 
 * @param   argv  `argv` from `main`.
 * @param   envp  `envp` from `main`.
 * @return        0 on success, -1 on error.
 */
static int
spawn(char *argv[], char *envp[])
{
	pid_t pid;

	/* Try forking until we success. */
	while ((pid = fork()) == -1) {
		if (errno != EAGAIN)
			return -1;
		(void) sleep(1); /* Possibly shorter because of SIGCHLD. */
	}

	/* Parent. */
	if (pid) {
		child_count++;
		timer_pid = pid;
		return 0;
	}

	/* Child. */
	close(LOCK_FILENO);
	execve(DAEMON_IMAGE("timer"), argv, envp);
	perror(argv[0]);
	exit(1);
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
 * @return        1 if the timer expired, 0 otherwise, -1 on error.
 */
static int
test_timer(int fd, const fd_set *fdset)
{
	int64_t _overrun;
	if (!FD_ISSET(fd, fdset))                return 0;
	if (read(fd, &_overrun, (size_t)8) < 8)  return -1;
	if (timer_pid == NO_TIMER_SPAWNED)       return 1;
	return timerfd_settime(fd, TFD_TIMER_ABSTIME, &nilspec, NULL) * 2 + 1;
}


/**
 * The sat daemon.
 * 
 * @param   argc  Should be 2.
 * @param   argv  The name of the process, and the pathname to the state file.
 * @param   envp  The environment.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 */
int
main(int argc, char *argv[], char *envp[])
{
	int fd = -1, rc = 0, r, expired = 0;
	fd_set fdset;
	struct stat attr;

	/* Set up signal handlers. */
	t (signal(SIGHUP,  sighandler) == SIG_ERR);
	t (signal(SIGCHLD, sighandler) == SIG_ERR);

	/* The magnificent loop. */
again:
	/* Update the a newer version of the daemon? */
	if (received_signo == SIGHUP) {
		execve(DAEMON_IMAGE("diminished"), argv, envp);
		perror(argv[0]);
	}
	/* Need to set new timer values? */
	if ((received_signo == SIGCHLD) && !child_count)
		t (spawn(argv, envp));
	received_signo = 0;
#if 1 || !defined(DEBUG)
	/* Can we quit yet? */
	if (expired && !child_count) {
		t (r = is_timer_set(BOOT_FILENO), r < 0);  if (r) goto not_done;
		t (r = is_timer_set(REAL_FILENO), r < 0);  if (r) goto not_done;
		t (fstat(STATE_FILENO, &attr));
		if (attr.st_size > (off_t)sizeof(size_t))
			t (spawn(argv, envp));
		else
			goto done;
	 }
#endif
not_done:
	/* Wait for something to happen. */
	FD_ZERO(&fdset);
	FD_SET(BOOT_FILENO, &fdset);
	FD_SET(REAL_FILENO, &fdset); /* This is the highest one. */
	if (select(REAL_FILENO + 1, &fdset, NULL, NULL, NULL) == -1) {
		t (errno != EINTR);
		goto again;
	}
	/* Was any jobs expired? */
	t ((expired |= test_timer(BOOT_FILENO, &fdset)) < 0);
	t ((expired |= test_timer(REAL_FILENO, &fdset)) < 0);
	goto again;

fail:
	perror(argv[0]);
	close(fd);
	rc = 1;
done:
	while (waitpid(-1, NULL, 0) > 0);
	if (!rc)
		unlink(argv[2]);
	close(STATE_FILENO);
	return rc;
	(void) argc;
}

