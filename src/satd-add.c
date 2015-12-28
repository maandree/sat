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
 * Subroutine to the sat daemon: add job.
 * 
 * @param   argc  Should be 4.
 * @param   argv  The name of the process, the pathname of the socket,
 *                the pathname to the state file, and $SAT_HOOK_PATH
 *                (the pathname of the hook-script.)
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 */
int
main(int argc, char *argv[])
{
	size_t n = 0, elements = 0, i;
	char *message = NULL;
	int msg_argc;
	char **msg = NULL;

	/* Receive and validate message. */
	t (readall(SOCK_FILENO, &message, &n));
	t (n < sizeof(int) + sizeof(clk) + sizeof(ts));
	n -= sizeof(int) + sizeof(clk) + sizeof(ts);
	msg_argc = *(int *)(message + n);
	t ((msg_argc < 1) || !n || message[n - 1]);
	for (i = n; i--; elements += !message[i]);
	t (elements < (size_t)msg_argc);
	n += sizeof(int) + sizeof(clk) + sizeof(ts);

	return 0;
fail:
}

