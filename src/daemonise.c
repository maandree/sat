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
 * 
 * This file is copied from <http://github.com/maandree/slibc>,
 * but with unused stuff removed. It has also been slightly modified
 * and changed code style (these "slight" modifications do not change
 * the behaviour.)
 */
#define _POSIX_C_SOURCE  200809L
#include "daemonise.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/resource.h>


#define t(...)  do { if (__VA_ARGS__) goto fail; }  while (0)
#define S(...)  (saved_errno = errno, __VA_ARGS__, errno = saved_errno)


/**
 * The process's environment variables.
 */
extern char** environ;

/**
 * The pidfile created by `daemonise`.
 */
static char* __pidfile = NULL;



/**
 * Wrapper for `dup` create a new file descriptor
 * with a number not less than 3.
 * 
 * @param   old  The file descriptor to duplicate.
 * @return       The new file descriptor, -1 on error.
 * 
 * @throws  Any error specified for dup(3).
 */
static int
dup_at_least_3(int old)
{
	int intermediary[3];
	int i = 0, saved_errno;

	do {
		t (old = dup(old), old == -1);
		assert(i < 3);
		if (i >= 3)
			abort();
		intermediary[i++] = old;
	} while (old < 3);
	i--;

fail:
	saved_errno = errno;
	while (i--)
		close(intermediary[i]);
	errno = saved_errno;
	return old;
}


/**
 * Daemonise the process. This means to:
 * 
 * -  close all file descritors except for those to
 *    stdin, stdout, and stderr,
 * 
 * -  remove all custom signal handlers, and apply
 *    the default handlers.
 * 
 * -  unblock all signals,
 * 
 * -  remove all malformatted entries in the
 *    environment (this not containing an '=',)
 * 
 * -  set the umask to zero, to be ensure that all
 *    file permissions are set as specified,
 * 
 * -  change directory to '/', to ensure that the
 *    process does not block any mountpoint from being
 *    unmounted,
 * 
 * -  fork to become a background process,
 * 
 * -  temporarily become a session leader to ensure
 *    that the process does not have a controlling
 *    terminal.
 * 
 * -  fork again to become a child of the daemon
 *    supervisor, (subreaper could however be in the
 *    say, so one should not merely rely on this when
 *    writing a daemon supervisor,) (the first child
 *    shall exit after this,)
 * 
 * -  create, exclusively, a PID file to stop the daemon
 *    to be being run twice concurrently, and to let
 *    the daemon supervicer know the process ID of the
 *    daemon,
 * 
 * -  redirect stdin and stdout to /dev/null,
 *    as well as stderr if it is currently directed
 *    to a terminal, and
 * 
 * -  exit in the original process to let the daemon
 *    supervisor know that the daemon has been
 *    initialised.
 * 
 * Before calling this function, you should remove any
 * environment variable that could negatively impact
 * the runtime of the process.
 * 
 * After calling this function, you should remove
 * unnecessary privileges.
 * 
 * Do not try do continue the process in failure unless
 * you make sure to only do this in the original process.
 * But not that things will not necessarily be as when
 * you make the function call. The process can have become
 * partially deamonised.
 * 
 * If $XDG_RUNTIME_DIR is set and is not empty, its value
 * should be used instead of /run for the runtime data-files
 * directory, in which the PID file is stored.
 * 
 * This is a slibc extension.
 * 
 * @etymology  (Daemonise) the process!
 * 
 * @param   name   The name of the daemon. Use a hardcoded value,
 *                 not the process name. Must not be `NULL`.
 * @param   ...    This is a `-1`-terminated list
 *                 of file descritors to keep open.
 *                 All arguments are of type `int`.
 * @return         Zero on success, -1 on error.
 * 
 * @throws  EEXIST  The PID file already exists on the system.
 *                  Unless your daemon supervisor removs old
 *                  PID files, this could mean that the daemon
 *                  has exited without removing the PID file.
 * @throws          Any error specified for signal(3).
 * @throws          Any error specified for sigemptyset(3).
 * @throws          Any error specified for sigprocmask(3).
 * @throws          Any error specified for chdir(3).
 * @throws          Any error specified for pipe(3).
 * @throws          Any error specified for dup(3).
 * @throws          Any error specified for dup2(3).
 * @throws          Any error specified for fork(3).
 * @throws          Any error specified for setsid(3).
 * @throws          Any error specified for open(3).
 * @throws          Any error specified for malloc(3).
 * 
 * @since  Always.
 */
int daemonise(const char* name, ...)
{
	struct rlimit rlimit;
	int pipe_rw[2] = { -1, -1 };
	sigset_t set;
	char** r;
	char** w;
	char* run;
	int i, closeerr, fd = -1;
	char* keep = NULL;
	int keepmax = 0;
	pid_t pid;
	va_list args;
	int saved_errno;

	/* Find out which file descriptors not too close. */
	va_start(args, name);
	while ((fd = va_arg(args, int)) >= 0)
		keepmax = fd;
	fd = -1;
	va_end(args);
	keep = calloc((size_t)keepmax + 1, sizeof(char));
	t (keep == NULL);
	va_start(args, name);
	while ((fd = va_arg(args, int)) >= 0)
		keep[fd] = 1;
	fd = -1;
	va_end(args);
	/* We assume that the maximum file descriptor is not extremely large.
	 * We also assume the number of file descriptors too keep is very small,
	 * but this does not affect us. */
  
	/* Close all files except stdin, stdout, and stderr. */
	if (getrlimit(RLIMIT_NOFILE, &rlimit))
		rlimit.rlim_cur = 4 << 10;
	for (i = 3; (rlim_t)i < rlimit.rlim_cur; i++)
		/* File descriptors with numbers above and including
		 * `rlimit.rlim_cur` cannot be created. They cause EBADF. */
		if ((i > keepmax) || (keep[i] == 0))
			close(i);
	free(keep), keep = NULL;

	/* Reset all signal handlers. */
	for (i = 1; i < _NSIG; i++)
		if (signal(i, SIG_DFL) == SIG_ERR)
			t (errno != EINVAL);

	/* Set signal mask. */
	t (sigemptyset(&set));
	t (sigprocmask(SIG_SETMASK, &set, NULL));

	/* Remove malformatted environment entires. */
	if (environ) {
		for (r = w = environ; *r; r++)
			if (strchr(*r, '=')) /* It happens that this is not the case! (Thank you PAM!) */
				*w++ = *r;
		*w = NULL;
	}

	/* Zero umask. */
	umask(0);

	/* Change current working directory to '/'. */
	t (chdir("/"));

	/* Create a channel for letting the original process know when to exit. */
	if (pipe(pipe_rw))
		t ((pipe_rw[0] = pipe_rw[1] = -1));
	t (fd = dup_at_least_3(pipe_rw[0]), fd == -1);
	close(pipe_rw[0]);
	pipe_rw[0] = fd;
	t (fd = dup_at_least_3(pipe_rw[1]), fd == -1);
	close(pipe_rw[1]);
	pipe_rw[1] = fd;
  
	/* Become a background process. */
	t (pid = fork(), pid == -1);
	close(pipe_rw[!!pid]), pipe_rw[!!pid] = 1;
	if (pid)
		exit(read(pipe_rw[0], &fd, (size_t)1) <= 0);
  
	/* Temporarily become session leader. */
	t (setsid() == -1);
  
	/* Fork again. */
	t (pid = fork(), pid == -1);
	if (pid > 0)
		exit(0);
  
	/* Create PID file. */
	run = getenv("XDG_RUNTIME_DIR");
	if (run && *run) {
		__pidfile = malloc(sizeof("/.pid") + (strlen(run) + strlen(name)) * sizeof(char));
		t (__pidfile == NULL);
		stpcpy(stpcpy(stpcpy(stpcpy(__pidfile, run), "/"), name), ".pid");
	} else {
		__pidfile = malloc(sizeof("/run/.pid") + strlen(name) * sizeof(char));
		t (__pidfile == NULL);
		stpcpy(stpcpy(stpcpy(__pidfile, "/run/"), name), ".pid");
	}
	fd = open(__pidfile, O_WRONLY | O_CREAT, 0644);
	if (fd == -1) {
		S(free(__pidfile), __pidfile = NULL);
		goto fail;
	}
	pid = getpid();
	t (dprintf(fd, "%lli\n", (long long int)pid) < 0);
	t (close(fd) && (errno != EINTR));

	/* Redirect to '/dev/null'. */
	closeerr = (isatty(2) || (errno == EBADF));
	t (fd = open("/dev/null", O_RDWR), fd == -1);
	if (fd != 0)  close(0);
	if (fd != 1)  close(1);
	if (closeerr)  if (fd != 2)  close(2);
	t (dup2(fd, 0) == -1);
	t (dup2(fd, 1) == -1);
	if (closeerr)  t (dup2(fd, 2) == -1);
	if (fd > 2)
		close(fd);
	fd = -1;

	/* We are done! Let the original process exit. */
	if ((write(pipe_rw[1], &fd, (size_t)1) <= 0) ||
	    (close(pipe_rw[1]) && (errno != EINTR))) {
		undaemonise();
		abort(); /* Do not overcomplicate things, just abort in this unlikely event. */
	}

	return 0;
fail:
	return S(close(pipe_rw[0]), close(pipe_rw[1]), close(fd), free(keep)), -1;
}


/**
 * Remove the PID file created by `daemonise`. This shall
 * always be called before exiting after calling `daemonise`,
 * even if it failed.
 * 
 * This is a slibc extension.
 * 
 * @etymology  (Un)link PID file created by `(daemonise)`!
 * 
 * @return  Zero on success, -1 on error.
 * 
 * @throws  Any error specified for unlink(3).
 * 
 * @since  Always.
 */
int undaemonise(void)
{
	int r, saved_errno;
	if (__pidfile == NULL)
		return 0;
	r = unlink(__pidfile);
	S(free(__pidfile), __pidfile = NULL);
	return r;
}

