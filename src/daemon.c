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
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/file.h>



/**
 * Common code for `preadn` and `pwriten`.
 * 
 * @param  FUN  `pread` or `pwrite`.
 */
#define PIO(FUN)  \
	char *buffer = buf;  \
	ssize_t r, n = 0;  \
	while (nbyte) {  \
		r = FUN(fildes, buffer, nbyte, offset);  \
		if (r <  0)  return -1;  \
		if (r == 0)  break;  \
		n += r;  \
		nbyte -= (size_t)r;  \
		offset += (size_t)r;  \
		buffer += (size_t)r;  \
	}  \
	return n


/**
 * Wrapper for `pread` that reads the required amount of data.
 * 
 * @param   fildes  See pread(3).
 * @param   buf     See pread(3).
 * @param   nbyte   See pread(3).
 * @param   offset  See pread(3).
 * @return          See pread(3), only short if the file is shorter.
 */
static ssize_t
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
static ssize_t
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
	size_t ptr = 0;
	size_t size = 0;
	ssize_t got;
	char *new;
	int saved_errno;

	for (;;) {
		if (ptr == size) {
			new = realloc(buffer, size <<= 1);
			t (!new);
			buffer = new;
		}
		got = read(fd, buffer + ptr, size - ptr);
		t (got < 0);
		if (got == 0)
			break;
		ptr += (size_t)got;
	}

	new = realloc(buffer, ptr);
	*buf = new ? new : buffer;
	*n = ptr;
	return 0;

fail:
	saved_errno = errno;
	free(buffer);
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
	char **new;
	size_t i, e = 0;
	t (!rc);
	while (i < len) {
		rc[++e] = buf + i;
		i += strlen(buf + i);
	}
	rc[e] = NULL;
	new = realloc(rc, (e + 1) * sizeof(char*));
	if (n)
		*n = e;
	return new ? new : rc;
fail:
	return NULL;
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
	rc[n] = NULL;
	while (n--)
		rc[n] = list[n];
	return rc;
fail:
	return NULL;
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
	r = open(path, oflag);
	if (r < 0)
		return -1;
	if (dup2(r, fd) == -1)
		return saved_errno = errno, close(r), errno = saved_errno, -1;
	close(r);
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
 * Removes (and optionally runs) a job.
 * 
 * @param   jobno   The job number, `NULL` for any job.
 * @param   runjob  Shall we run the job too?
 * @return          0 on success, -1 on error.
 * 
 * @throws  0  The job is not in the queue.
 */
int
remove_job(const char *jobno, int runjob)
{
	char *end;
	char *buf = NULL;
	size_t no = 0, off = 0, n;
	ssize_t r;
	struct stat attr;
	struct job job;
	int saved_errno;

	if (jobno) {
		no = (errno = 0, strtoul)(jobno, &end, 10);
		if (errno || *end || !isdigit(*jobno))
			return 0;
	}

	t (flock(STATE_FILENO, LOCK_EX));
	t (fstat(STATE_FILENO, &attr));
	n = (size_t)(attr.st_size);
	while (off < n) {
		t (preadn(STATE_FILENO, &job, sizeof(job), off) < (ssize_t)sizeof(job));
		if (!jobno || (job.no == no))
			goto found_it;
		off += sizeof(job) + job.n;
	}
	t (flock(STATE_FILENO, LOCK_UN));
	return 0;

found_it:
	n -= off + sizeof(job) + job.n;
	t (!(buf = malloc(n)));
	t (r = preadn(STATE_FILENO, buf, n, off + sizeof(job) + job.n), r < 0);
	t (pwriten(STATE_FILENO, buf, (size_t)r, off) < 0);
	t (ftruncate(STATE_FILENO, (size_t)r + off));
	free(buf), buf = NULL;
	fsync(STATE_FILENO);
	flock(STATE_FILENO, LOCK_UN);
	if (runjob == 0)
		return 0;

	/* TODO run job (when running, remember to use PATH from the job's envp) */

fail:
	saved_errno = errno;
	flock(STATE_FILENO, LOCK_UN);
	free(buf);
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
	size_t off = 0, n, j = 0;
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
		t (preadn(STATE_FILENO, js[j++] + sizeof(job), job.n, off) < (ssize_t)(job.n));
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

