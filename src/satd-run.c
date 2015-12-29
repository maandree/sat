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
 * Subroutine to the sat daemon: run jobs early.
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
	size_t n = 0;
	char *message = NULL;
	char **msg_argv = NULL;
	char **arg;
	int rc = 0;

	assert(argc == 3);
	t (reopen(STATE_FILENO, O_RDWR));

	/* Receive and validate message. */
	t (readall(SOCK_FILENO, &message, &n) || (n && message[n - 1]));
	shutdown(SOCK_FILENO, SHUT_RD);
	if (n) {
		msg_argv = restore_array(message, n, NULL);
		t (!msg_argv);
	}

	/* Perform action. */
	if (msg_argv) {
		for (arg = msg_argv; *arg; arg++)
			t (remove_job(*arg, 1) && errno);
	} else {
		for (;;)
			if (remove_job(NULL, 1))
				t (errno);
	}

done:
	/* Cleanup. */
	shutdown(SOCK_FILENO, SHUT_WR);
	close(SOCK_FILENO);
	close(STATE_FILENO);
	free(msg_argv);
	free(message);
	return rc;
fail:
	if (send_string(SOCK_FILENO, STDERR_FILENO, argv[0], ": ", strerror(errno), "\n", NULL))
		perror(argv[0]);
	rc = 1;
	goto done;

	(void) argc;
}

