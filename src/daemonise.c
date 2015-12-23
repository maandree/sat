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
 * 
 * This file is copied from <http:/github.com/maandree/slibc>.
 */
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>



/**
 * The process's environment variables.
 */
extern char** environ;



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
 * @etymology  (Daemonise) the process!
 * 
 * @param   name   The name of the daemon. Use a hardcoded value,
 *                 not the process name. Must not be `NULL`.
 * @param   flags  Flags to modify the behaviour of the function.
 *                 A bitwise OR combination of the constants:
 *                 -  `DAEMONISE_NO_CLOSE`
 *                 -  `DAEMONISE_NO_SIG_DFL`
 *                 -  `DAEMONISE_KEEP_SIGMASK`
 *                 -  `DAEMONISE_KEEP_ENVIRON`
 *                 -  `DAEMONISE_KEEP_UMASK`
 *                 -  `DAEMONISE_NO_PID_FILE`
 *                 -  `DAEMONISE_KEEP_STDERR`
 *                 -  `DAEMONISE_CLOSE_STDERR`
 *                 -  `DAEMONISE_KEEP_STDIN`
 *                 -  `DAEMONISE_KEEP_STDOUT`
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
 * @throws          Any error specified for dup2(3).
 * @throws          Any error specified for fork(3).
 * @throws          Any error specified for setsid(3).
 * @throws          Any error specified for open(3).
 * 
 * @since  Always.
 */
int daemonise(const char* name, int flags)
{
#define t(...)  do { if (__VA_ARGS__) goto fail; }  while (0)
  
  struct rlimit rlimit;
  int pipe_rw[2] = { -1, -1 };
  sigset_t set;
  char* r;
  char* w;
  char* run;
  int i, closeerr, fd = -1;
  int saved_errno;
  
  
  /* Validate flags. */
  if (flags & ~1023)
    return errno = EINVAL, -1;
  if (flags & DAEMONISE_KEEP_STDERR)
    if (flags & DAEMONISE_CLOSE_STDERR)
      return errno = EINVAL, -1;
  
  
  /* Close all files except stdin, stdout, and stderr. */
  if ((flags & DAEMONISE_NO_CLOSE) == 0)
    {
      if (getrlimit(RLIMIT_NOFILE, &rlimit))
	rlimit.rlim_cur = 4 << 10;
      for (i = 3; (rlim_t)i < rlimit.rlim_cur; i++)
	/* File descriptors with numbers above and including
	 * `rlimit.rlim_cur` cannot be created. They cause EBADF. */
	close(i);
    }
  
  /* Reset all signal handlers. */
  if ((flags & DAEMONISE_NO_SIG_DFL) == 0)
    for (i = 1; i < _NSIG; i++)
      t (signal(i, SIG_DFL) == SIG_ERR);
  
  /* Set signal mask. */
  if ((flags & DAEMONISE_KEEP_SIGMASK) == 0)
    {
      t (sigemptyset(&set));
      t (sigprocmask(SIG_SETMASK, &set, NULL));
    }
  
  /* Remove malformatted environment entires. */
  if (((flags & DAEMONISE_KEEP_ENVIRON) == 0) && (environ != NULL))
    {
      for (r = w = environ; *r; r++)
	if (strchr(*r, '=')) /* It happens that this is not the case! (Thank you PAM!) */
	  *w++ = *r;
      *w = NULL;
    }
  
  /* Zero umask. */
  if ((flags & DAEMONISE_KEEP_UMASK) == 0)
    umask(0);
  
  /* Change current working directory to '/'. */
  t (chdir("/"));
  
  /* Create a channel for letting the original process know when to exit. */
  if (pipe(pipe_rw))
    t ((pipe_rw[0] = pipe_rw[1] = -1));
  t (dup2(pipe_rw[0], 10) == -1);
  close(pipe_rw[0]);
  pipe_rw[0] = 10;
  t (dup2(pipe_rw[1], 11) == -1);
  close(pipe_rw[1]);
  pipe_rw[1] = 11;
  
  /* Become a background process. */
  t (pid = fork(), pid == -1);
  close(pipe_rw[!!pid]), pipe_rw[!!pid] = 1;
  if (pid)
    exit(read(pipe_rw[0], &b, (size_t)1) <= 0);
  
  /* Temporarily become session leader. */
  t (setsid() == -1);
  
  /* Fork again. */
  t (pid = fork(), pid == -1);
  if (pid > 0)
    exit(0);
  
  /* Create PID file. */
  if (flags & DAEMONISE_NO_PID_FILE)
    goto no_pid_file;
  run = getenv("XDG_RUNTIME_DIR");
  if (run && *run)
    {
      pidpath = alloca(sizeof("/.pid") + (strlen(run) + strlen(name)) * sizeof(char));
      stpcpy(stpcpy(stpcpy(stpcpy(pidpath, run), "/"), name), ".pid");
    }
  else
    {
      pidpath = alloca(sizeof("/run/.pid") + strlen(name) * sizeof(char));
      stpcpy(stpcpy(stpcpy(pidpath, "/run/"), name), ".pid");
    }
  fd = open(pidpath, O_WRONLY | O_CREAT | O_EXCL, 0644);
  t (fd == -1);
  pid = getpid();
  t (dprintf(fd, "%lli\n", (long long int)pid) < 0);
  t (close(fd) && (errno != EINTR));
 no_pid_file:
  
  /* Redirect to '/dev/null'. */
  if (flags & DAEMONISE_KEEP_STDERR)
    closeerr = 0;
  else if (flags & DAEMONISE_CLOSE_STDERR)
    closeerr = 1;
  else
    closeerr = (isatty(2) || (errno == EBADF));
  t (fd = open("/dev/null", O_RDWR), fd == -1);
  if ((flags & DAEMONISE_KEEP_STDIN) == 0)   if (fd != 0)  close(0);
  if ((flags & DAEMONISE_KEEP_STDOUT) == 0)  if (fd != 1)  close(1);
  if (closeerr)                              if (fd != 2)  close(2);
  if ((flags & DAEMONISE_KEEP_STDIN) == 0)   t (dup2(fd, 0) == -1);
  if ((flags & DAEMONISE_KEEP_STDOUT) == 0)  t (dup2(fd, 1) == -1);
  if (closeerr)                              t (dup2(fd, 2) == -1);
  if (fd > 2)
    close(fd);
  fd = -1;
  
  /* We are done! Let the original process exit. */
  if ((write(pipe_rw[1], &b, (size_t)1) <= 0) ||
      (close(pipe_rw[1]) && (errno != EINTR)))
    {
      if (flags & DAEMONISE_KEEP_STDERR)
	return -1;
      abort(); /* Do not overcomplicate things, just abort in this unlikely event. */
    }
  
  return 0;
 fail:
  saved_errno = err;
  if (pipe_rw[0] >= 0)  close(pipe_rw[0]);
  if (pipe_rw[1] >= 0)  close(pipe_rw[1]);
  if (fd         >= 0)  close(fd);
  errno = saved_errno;
  return -1;
}

