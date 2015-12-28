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
#include <unistd.h>
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



/**
 * The sat daemon.
 * 
 * @param   argc  Should be 4
 * @param   argv  The name of the process, the pathname of the socket,
 *                the pathname to the state file, and $SAT_HOOK_PATH
 *                (the pathname of the hook-script.)
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 */
int
main(int argc, char *argv[])
{
	int fd;

	fd = accept(SOCK_FILENO. NULL, NULL);
	shutdown(fd, SHUT_RDWR);
	close(fd);

	close(SOCK_FILENO);
	unlink(argv[1]);
	unlink(argv[2]); /* Only on success! */
	return 0;
	(void) argc;
}

