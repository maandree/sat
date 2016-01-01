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
#include <ctype.h>
#include <stdarg.h>
#include <pwd.h>
#include <sys/wait.h>



/**
 * The environment.
 */
extern char **environ;



/**
 * Common code for `preadn` and `pwriten`.
 * 
 * @param  FUN  `pread` or `pwrite`.
 */
#define PIO(FUN)  \
	ssize_t r, n = 0;  \
	int saved_errno = 0;  \
	sigset_t mask, oldmask;  \
	sigfillset(&mask);  \
	sigprocmask(SIG_BLOCK, &mask, &oldmask);  \
	while (nbyte) {  \
		t (r = FUN(fildes, buffer, nbyte, (off_t)offset), r < 0);  \
		if (r == 0)  \
			break;  \
		n += r;  \
		buffer += r;  \
		offset += (size_t)r;  \
		nbyte -= (size_t)r;  \
	}  \
done:  \
	sigprocmask(SIG_SETMASK, &oldmask, NULL);  \
	errno = saved_errno;  \
	return n;  \
fail:  \
	saved_errno = errno;  \
	n = -1;  \
	goto done


/**
 * Wrapper for `pread` that reads the required amount of data.
 * 
 * @param   fildes  See pread(3).
 * @param   buf     See pread(3).
 * @param   nbyte   See pread(3).
 * @param   offset  See pread(3).
 * @return          See pread(3), only short if the file is shorter.
 */
ssize_t
preadn(int fildes, void *buf, size_t nbyte, size_t offset)
{
	char *buffer = buf;
	PIO(pread);
}


/**
 * Wrapper for `pwrite` that writes all specified data.
 * 
 * @param   fildes  See pwrite(3).
 * @param   buf     See pwrite(3).
 * @param   nbyte   See pwrite(3).
 * @param   offset  See pwrite(3).
 * @return          See pwrite(3).
 */
ssize_t
pwriten(int fildes, const void *buf, size_t nbyte, size_t offset)
{
	const char *buffer = buf;
	PIO(pwrite);
}


/**
 * Unmarshal a `NULL`-terminated string array.
 * 
 * The elements are not actually copied, subpointers
 * to `buf` are stored in the returned list.
 * 
 * @param   buf  The marshalled array. Must end with a NUL byte.
 * @param   len  The length of `buf`.
 * @param   n    Output parameter for the number of elements. May be `NULL`
 * @return       The list, `NULL` on error.
 * 
 * @throws  Any exception specified for realloc(3).
 */
char **
restore_array(char *buf, size_t len, size_t *n)
{
	char **rc = malloc((len + 1) * sizeof(char*));
	char **new = NULL;
	size_t i = 0, e = 0;

	t (!rc);
	while (i < len)  i += strlen(rc[e++] = buf + i) + 1;
	if (n)           *n = e;
	rc[e++] = NULL;
	new = realloc(rc, e * sizeof(char*));
fail:
	return new ? new : rc;
}


/**
 * Create `NULL`-terminate subcopy of an list,
 * 
 * @param   list  The list.
 * @param   n     The number of elements in the new sublist.
 * @return        The sublist, `NULL` on error.
 * 
 * @throws  Any exception specified for malloc(3).
 */
char **
sublist(char *const *list, size_t n)
{
	char **rc = malloc((n + 1) * sizeof(char*));
	t (!rc);
	for (rc[n] = NULL; n--; rc[n] = list[n]);
fail:
	return rc;
}


/**
 * Create a new open file descriptor for an already
 * existing file descriptor.
 * 
 * @param   fd     The file descriptor that shall be promoted
 *                 to a new open file descriptor.
 * @param   oflag  See open(3), `O_CREAT` is not allowed.
 * @return         0 on success, -1 on error.
 */
int
reopen(int fd, int oflag)
{
	char path[sizeof("/dev/fd/") + 3 * sizeof(int)];
	int r, saved_errno;

	sprintf(path, "/dev/fd/%i", fd);
	if (r = open(path, oflag), r < 0)
		return -1;
	if (DUP2_AND_CLOSE(r, fd) == -1)
		return S(close(r)), -1;
	return 0;
}


/**
 * Run a job or a hook.
 * 
 * @param   job   The job.
 * @param   hook  The hook, `NULL` to run the job.
 * @return        0 on success, -1 on error, 1 if the child failed.
 */
int
run_job_or_hook(struct job *job, const char *hook)
{
	pid_t pid;
	char **args = NULL;
	char **argv = NULL;
	char **envp = NULL;
	size_t argsn;
	void *new;
	int status = 0, saved_errno;

	t (!(args = restore_array(job->payload, job->n, &argsn)));
	t (!(argv = sublist(args, (size_t)(job->argc))));
	t (!(envp = sublist(args + job->argc, argsn - (size_t)(job->argc)))); /* Includes wdir. */
	free(args), args = NULL;

	if (hook) {
		t (!(new = realloc(argv, ((size_t)(job->argc) + 3) * sizeof(*argv))));
		argv = new;
		memmove(argv + 2, argv, ((size_t)(job->argc) + 1) * sizeof(*argv));
		argv[0] = getenv("SAT_HOOK_PATH");
		argv[1] = (strstr)(hook, hook); /* strstr: just to remove a warning */
	}

	if (!(pid = fork())) {
		close(STATE_FILENO), close(BOOT_FILENO), close(REAL_FILENO);
		(void)(status = chdir(envp[0]));
		environ = envp + 1;
		execvp(*argv, argv);
		exit(1);
	}

	t ((pid < 0) || (waitpid(pid, &status, 0) != pid));
fail:
	S(free(args), free(argv), free(envp));
	return status ? 1 : -!!saved_errno;
}


/**
 * Removes (and optionally runs) a job.
 * 
 * @param   jobno   The job number, `NULL` for any job.
 * @param   runjob  Shall we run the job too? 2 if its time has expired (not forced).
 * @return          0 on success, -1 on error.
 * 
 * @throws  0  The job is not in the queue.
 */
int
remove_job(const char *jobno, int runjob)
{
	char *end;
	char *buf = NULL;
	size_t no = 0, off = sizeof(size_t), n;
	ssize_t r;
	struct stat attr;
	struct job job;
	struct job *job_full = NULL;
	int rc = 0, saved_errno = 0;

	if (jobno) {
		no = (errno = 0, strtoul)(jobno, &end, 10);
		if (errno || *end || !isdigit(*jobno))
			return 0;
	}

	t (flock(STATE_FILENO, LOCK_EX));
	t (fstat(STATE_FILENO, &attr));
	for (n = (size_t)(attr.st_size); off < n; off += sizeof(job) + job.n) {
		t (preadn(STATE_FILENO, &job, sizeof(job), off) < (ssize_t)sizeof(job));
		if (!jobno || (job.no == no))
			goto found_it;
	}
	flock(STATE_FILENO, LOCK_UN); /* Failure isn't fatal. */
	return errno = 0, -1;

found_it:
	t (!(job_full = malloc(sizeof(job) + job.n)));
	*job_full = job;
	t (preadn(STATE_FILENO, job_full->payload, job.n, off + sizeof(job)) < (ssize_t)(job.n));
	n -= off + sizeof(job) + job.n;
	t (!(buf = malloc(n)));
	t (r = preadn(STATE_FILENO, buf, n, off + sizeof(job) + job.n), r < 0);
	t (pwriten(STATE_FILENO, buf, (size_t)r, off) < 0);
	t (ftruncate(STATE_FILENO, (off_t)r + (off_t)off));
	free(buf), buf = NULL;
	fsync(STATE_FILENO);

	if (runjob) {
		run_job_or_hook(job_full, runjob == 2 ? "expired" : "forced");
		rc = run_job_or_hook(job_full, NULL);
		saved_errno = errno;
		run_job_or_hook(job_full, rc ? "failure" : "success");
		rc = rc == 1 ? 0 : rc;
	} else {
		run_job_or_hook(job_full, "removed");
	}

	free(job_full);
	flock(STATE_FILENO, LOCK_UN); /* Unlock late so that hooks are synchronised. Failure isn't fatal. */
	errno = saved_errno;
	return rc;

fail:
	S(flock(STATE_FILENO, LOCK_UN), free(buf), free(job_full));
	return -1;
}


/**
 * Get a `NULL`-terminated list of all queued jobs.
 * 
 * @return  A `NULL`-terminated list of all queued jobs. `NULL` on error.
 */
struct job **
get_jobs(void)
{
	size_t off = sizeof(size_t), n, j = 0;
	struct stat attr;
	struct job **js = NULL;
	struct job job;
	int saved_errno;

	t (flock(STATE_FILENO, LOCK_SH));
	t (fstat(STATE_FILENO, &attr));
	n = (size_t)(attr.st_size);
	t (!(js = malloc((n / sizeof(**js) + 1) * sizeof(*js))));
	while (off < n) {
		t (preadn(STATE_FILENO, &job, sizeof(job), off) < (ssize_t)sizeof(job));
		off += sizeof(job);
		t (!(js[j] = malloc(sizeof(job) + job.n)));
		*(js[j]) = job;
		t (preadn(STATE_FILENO, js[j++]->payload, job.n, off) < (ssize_t)(job.n));
		off += job.n;
	}
	t (flock(STATE_FILENO, LOCK_UN));
	return js[j] = NULL, js;

fail:
	saved_errno = errno;
	flock(STATE_FILENO, LOCK_UN);
	while (j--)  free(js[j]);
	free(js);
	errno = saved_errno;
	return NULL;
}


/**
 * Duplicate a file descriptor, and
 * open /dev/null to the old file descriptor.
 * However, if `old` is 3 or greater, it will
 * be closed rather than /dev/null.
 * 
 * @param   old  The old file descriptor.
 * @param   new  The new file descriptor.
 * @return       `new`, -1 on error.
 */
int
dup2_and_null(int old, int new)
{
	int fd = -1, saved_errno;

	if (old == new)  return new;  t (DUP2_AND_CLOSE(old, new));
	if (old >= 3)    return new;  t (fd = open("/dev/null", O_RDWR), fd == -1);
	if (fd == old)   return new;  t (DUP2_AND_CLOSE(fd, old));
	return new;
fail:
	return S(close(fd)), -1;
}


/**
 * Create or open the state file.
 * 
 * @param   open_flags  Flags (the second parameter) for `open`.
 * @param   state_path  Output parameter for the state file's pathname.
 *                      May be `NULL`;
 * @return              A file descriptor to the state file, -1 on error.
 * 
 * @throws  0  `!(open_flags & O_CREAT)` and the file does not exist.
 */
int
open_state(int open_flags, char **state_path)
{
	const char *dir;
	char *path;
	int fd = -1, saved_errno;

	/* Create directory. */
	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	t (!(path = malloc(strlen(dir) * sizeof(char) + sizeof("/" PACKAGE "/state"))));
	stpcpy(stpcpy(path, dir), "/" PACKAGE "/state");
	t (fd = open(path, open_flags, S_IRUSR | S_IWUSR), fd == -1);

	if (state_path)  *state_path = path, path = NULL;
	else             free(path), path = NULL;
fail:
	S(free(path));
	if (!(open_flags & O_CREAT) && ((errno == ENOENT) || (errno == ENOTDIR)))
		errno = 0;
	return fd;
}


/**
 * Let the daemon know that it may need to
 * update the timers, and perhaps exit.
 * 
 * @param   start  Start the daemon if it is not running?
 * @param   name   The name of the process.
 * @return         0 on success, -1 on error.
 */
int
poke_daemon(int start, const char *name)
{
	char *path = NULL;
	const char *dir;
	pid_t pid;
	int fd = -1, status, saved_errno;

	/* Get the lock file's pathname. */
	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	t (!(path = malloc(strlen(dir) * sizeof(char) + sizeof("/" PACKAGE "/lock"))));
	stpcpy(stpcpy(path, dir), "/" PACKAGE "/lock");

	/* Any daemon listening? */
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		t ((errno != ENOENT) && (errno != ENOTDIR));
	} else {
		if (flock(fd, LOCK_SH | LOCK_NB /* and LOCK_DRY if that was ever added... */))
			t (start = 0, errno != EWOULDBLOCK);
		else
			flock(fd, LOCK_UN);
		t (read(fd, &pid, sizeof(pid)) < (ssize_t)sizeof(pid));
		close(fd), fd = -1;
	}
	free(path), path = NULL;

	/* Start daemon if not running, otherwise poke it. */
	if (start) {
		switch ((pid = fork())) {
		case -1:
			goto fail;
		case 0:
			execl(BINDIR "/satd", BINDIR "/satd", NULL);
			perror(name);
			exit(1);
		default:
			t (waitpid(pid, &status, 0) != pid);
			t (errno = 0, status);
			break;
		}
	} else {
		t (kill(pid, SIGCHLD));
	}

	return 0;
fail:
	return S(close(fd), free(path)), -1;
}


/**
 * Construct the pathname for the hook script.
 * 
 * @param   env     The environment variable to use for the beginning
 *                  of the pathname, `NULL` for the home directory.
 * @param   suffix  The rest of the pathname.
 * @return          The pathname.
 * 
 * @throws  0  The environment variable is not set, or, if `env` is
 *             `NULL` the user is root or homeless.
 */
static char *
hookpath(const char *env, const char *suffix)
{
	struct passwd *pwd;
	const char *prefix = NULL;
	char *path;

	if (env) {
		prefix = getenv(env);
	} else if (getuid()) {
		pwd = getpwuid(getuid());
		prefix = pwd ? pwd->pw_dir : NULL;
	}
	if (!prefix || !*prefix)
		return errno = 0, NULL;

	t (!(path = malloc((strlen(prefix) + strlen(suffix) + 1) * sizeof(char))));
	stpcpy(stpcpy(path, prefix), suffix);
fail:
	return path;
}


/**
 * Set SAT_HOOK_PATH.
 * 
 * @return  0 on success, -1 on error.
 */
int
set_hookpath(void)
{
#define HOOKPATH(PRE, SUF)  \
	t (path = path ? path : hookpath(PRE, SUF), !path && errno)
	char *path = NULL;
	int saved_errno;
	if (!getenv("SAT_HOOK_PATH")) {
		HOOKPATH("XDG_CONFIG_HOME", "/sat/hook");
		HOOKPATH("HOME", "/.config/sat/hook");
		HOOKPATH(NULL, "/.config/sat/hook");
		t (setenv("SAT_HOOK_PATH", path ? path : "/etc/sat/hook", 1));
	}
	return free(path), 0;
fail:
	return S(free(path)), -1;
}

