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
#include "daemon.h"
#include <sys/file.h>
#include <sys/stat.h>



/**
 * Subroutine to the sat daemon: add job.
 * 
 * @param   argc  Should be 3.
 * @param   argv  The name of the process, the pathname of the socket,
 *                and the pathname to the state file.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 */
int
main(int argc, char *argv[])
{
#define M  ((struct job *)message)

	size_t n = 0, elements = 0, i;
	ssize_t r;
	char *message = NULL;
	struct stat attr;
	DAEMON_PROLOGUE;

	/* Receive and validate message. */
	t (readall(SOCK_FILENO, &message, &n) || (n < sizeof(struct job)));
	t (M->n != (n -= sizeof(struct job)));
	t ((M->argc < 1) || !n || message[sizeof(struct job) - 1 + n]);
	for (i = n; i--; elements += !message[sizeof(struct job) + i]);
	t (elements < (size_t)(M->argc));

	/* Update state file and run hook. */
	t (flock(STATE_FILENO, LOCK_EX));
	t (fstat(STATE_FILENO, &attr));
	t (r = preadn(STATE_FILENO, &(M->no), sizeof(M->no), (size_t)0), r < 0);
	if (r < (ssize_t)sizeof(M->no))
		M->no = 0;
	else
		M->no += 1;
	t (pwriten(STATE_FILENO, &(M->no), sizeof(M->no), (size_t)0) < (ssize_t)sizeof(M->no));
	if (attr.st_size < (off_t)sizeof(M->no))
		attr.st_size = (off_t)sizeof(M->no);
	n += sizeof(struct job);
	t (pwriten(STATE_FILENO, message, n, (size_t)(attr.st_size)) < (ssize_t)n);
	fsync(STATE_FILENO);
	run_job_or_hook(M, "queued");
	t (flock(STATE_FILENO, LOCK_UN));

	DAEMON_CLEANUP_START;
	free(message);
	DAEMON_CLEANUP_END;
}

