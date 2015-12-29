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
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h>



/**
 * The file descriptor for the socket.
 */
#define SOCK_FILENO  3

/**
 * The file descriptor for the state file.
 */
#define STATE_FILENO  4


/**
 * Command: queue a job.
 */
#define SAT_QUEUE  0

/**
 * Command: remove jobs.
 */
#define SAT_REMOVE  1

/**
 * Command: print job queue.
 */
#define SAT_PRINT  2

/**
 * Command: run jobs.
 */
#define SAT_RUN  3



#ifndef t
/**
 * Go to `fail` if a statement evaluates to non-zero.
 * 
 * @param  ...  The statement.
 */
# define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)
#endif



/**
 * A queued job.
 */
struct job {
	/**
	 * The job number.
	 */
	size_t no;

	/**
	 * The number of “argv” elements in `payload`.
	 */
	int argc;

	/**
	 * The clock in which `ts` is measured.
	 */
	clockid_t clk;

	/**
	 * The time when the job shall be executed.
	 */
	struct timespec ts;

	/**
	 * The number of bytes in `payload`.
	 */
	size_t n;

	/**
	 * “argv” followed by “envp”.
	 */
	char payload[];
};



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
int readall(int fd, char **buf, size_t *n);

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
char **restore_array(char *buf, size_t len, size_t *n);

/**
 * Create `NULL`-terminate subcopy of an list,
 * 
 * @param   list  The list.
 * @param   n     The number of elements in the new sublist.
 * @return        The sublist, `NULL` on error.
 * 
 * @throws  Any exception specified for malloc(3).
 */
char **sublist(char *const *list, size_t n);

/**
 * Create a new open file descriptor for an already
 * existing file descriptor.
 * 
 * @param   fd     The file descriptor that shall be promoted
 *                 to a new open file descriptor.
 * @param   oflag  See open(3), `O_CREAT` is not allowed.
 * @return         0 on success, -1 on error.
 */
int reopen(int fd, int oflag);

/**
 * Send a string to a client.
 * 
 * @param   sockfd  The file descriptor of the socket.
 * @param   outfd   The file descriptor to which the client shall output the message.
 * @param   ...     `NULL`-terminated list of string to concatenate.
 * @return          0 on success, -1 on error.
 */
int send_string(int sockfd, int outfd, ...);

/**
 * Removes (and optionally runs) a job.
 * 
 * @param   jobno   The job number, `NULL` for any job.
 * @param   runjob  Shall we run the job too?
 * @return          0 on success, -1 on error.
 * 
 * @throws  0  The job is not in the queue.
 */
int remove_job(const char *jobno, int runjob);

/**
 * Get a `NULL`-terminated list of all queued jobs.
 * 
 * @return  A `NULL`-terminated list of all queued jobs. `NULL` on error.
 */
struct job **get_jobs(void);

