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
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/file.h>
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
	char *buffer = buf;  \
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
		nbyte -= (size_t)r;  \
		offset += (size_t)r;  \
		buffer += (size_t)r;  \
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
pwriten(int fildes, void *buf, size_t nbyte, size_t offset)
{
	PIO(pwrite);
}


/**
 * Wrapper for `read` that reads all available data.
 * 
 * Sets `errno` to `EBADMSG` on success.
 * 
 * @param   fd   The file descriptor from which to to read.
 * @param   buf  Output parameter for the data.
 * @param   n    Output parameter for the number of read bytes.
 * @return       0 on success, -1 on error.
 * 
 * @throws  Any exception specified for read(3).
 * @throws  Any exception specified for realloc(3).
 */
int
readall(int fd, char **buf, size_t *n)
{
	char *buffer = NULL;
	size_t ptr = 0, size = 0;
	ssize_t got = 1;
	char *new;
	int saved_errno;

	for (; got; ptr += (size_t)got) {
		if (ptr == size) {
			t (!(new = realloc(buffer, size <<= 1)));
			buffer = new;
		}
		t (got = read(fd, buffer + ptr, size - ptr), got < 0);
	}

	new = realloc(buffer, ptr);
	*buf = new ? new : buffer;
	*n = ptr;
	shutdown(SOCK_FILENO, SHUT_RD);
	return 0;

fail:
	saved_errno = errno;
	free(buffer);
	shutdown(SOCK_FILENO, SHUT_RD);
	errno = saved_errno;
	return -1;
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
	while (i < len) {
		rc[e++] = buf + i;
		i += strlen(buf + i);
	}
	rc[e] = NULL;
	new = realloc(rc, (e + 1) * sizeof(char*));
	if (n)
		*n = e;
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
	if (dup2(r, fd) == -1)
		return saved_errno = errno, close(r), errno = saved_errno, -1;
	close(fd);
	return 0;
}


/**
 * Send a string to a client.
 * 
 * @param   sockfd  The file descriptor of the socket.
 * @param   outfd   The file descriptor to which the client shall output the message.
 * @param   ...     `NULL`-terminated list of string to concatenate.
 * @return          0 on success, -1 on error.
 */
int
send_string(int sockfd, int outfd, ...)
{
	va_list args;
	size_t i, n = 0;
	ssize_t r;
	char out = (char)outfd;
	const char *s;

	va_start(args, outfd);
	while ((s = va_arg(args, const char *)))
		n += strlen(s);
	va_end(args);

	t (write(sockfd, &out, sizeof(out)) < (ssize_t)sizeof(out));
	t (write(sockfd, &n, sizeof(n)) < (ssize_t)sizeof(n));

	va_start(args, outfd);
	while ((s = va_arg(args, const char *)))
		for (i = 0, n = strlen(s); i < n; i += (size_t)r)
			t (r = write(sockfd, s + i, n - i), r <= 0);
	va_end(args);

	return 0;
fail:
	return -1;
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
	t (!(envp = sublist(args + job->argc, argsn - (size_t)(job->argc))));

	if (hook) {
		t (!(new = realloc(argv, ((size_t)(job->argc) + 3) * sizeof(*argv))));
		argv = new;
		memmove(argv + 2, argv, ((size_t)(job->argc) + 1) * sizeof(*argv));
		argv[0] = getenv("SAT_HOOK_PATH");
		argv[1] = strstr(hook, hook); /* strstr: just to remove a warning */
	}

	switch ((pid = fork())) {
	case -1:
		goto fail;
	case 0:
		close(SOCK_FILENO);
		close(STATE_FILENO);
		environ = envp;
		execve(*argv, argv, envp);
		exit(1);
	default:
		t (waitpid(pid, &status, 0) != pid);
		break;
	}

fail:
	saved_errno = errno;
	free(args);
	free(argv);
	free(envp);
	errno = saved_errno;
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
	int rc = 0, saved_errno;

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
	t (flock(STATE_FILENO, LOCK_UN));
	return 0;

found_it:
	job_full = malloc(sizeof(job) + job.n);
	*job_full = job;
	t (preadn(STATE_FILENO, job_full->payload, job.n, off) < (ssize_t)(job.n));
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
		free(job_full);
		errno = saved_errno;
	} else {
		run_job_or_hook(job_full, "removed");
		free(job_full);
	}

	flock(STATE_FILENO, LOCK_UN); /* Unlock late so that hooks are synchronised. */
	return rc;

fail:
	saved_errno = errno;
	flock(STATE_FILENO, LOCK_UN);
	free(buf);
	free(job_full);
	errno = saved_errno;
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
		t (!(js[j] = malloc(sizeof(job) + sizeof(job.n))));
		*(js[j]) = job;
		t (preadn(STATE_FILENO, js[j++]->payload, job.n, off) < (ssize_t)(job.n));
		off += job.n;
	}
	js[j] = NULL;
	t (flock(STATE_FILENO, LOCK_UN));
	return js;

fail:
	saved_errno = errno;
	while (j--)
		free(js[j]);
	free(js);
	flock(STATE_FILENO, LOCK_UN);
	errno = saved_errno;
	return NULL;
}

