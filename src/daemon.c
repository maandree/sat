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
	return 0; /* TODO send_string */
	(void) sockfd, (void) outfd;
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
	return 0; /* TODO remove_job */
	(void) jobno, (void) runjob;
}


/**
 * Get a `NULL` terminated list of all queued jobs.
 * 
 * @return  A `NULL` terminated list of all queued jobs. `NULL` on error.
 */
struct job **
get_jobs(void)
{
	return NULL; /* TODO get_jobs */
}

