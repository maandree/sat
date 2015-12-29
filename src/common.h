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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>



#ifndef t
/**
 * Go to `fail` if a statement evaluates to non-zero.
 * 
 * @param  ...  The statement.
 */
# define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)
#endif



/**
 * Defines the function `void usage(void)`
 * that prints usage information.
 * 
 * @param  synopsis:const char*  The command's synopsis sans command name.
 *                               `NULL` if there are not arguments.
 */
#define USAGE(synopsis)  \
static void  \
usage(void)  \
{  \
	fprintf(stderr, "usage: %s%s%s\n",  \
	        strrchr(argv0, '/') ? (strrchr(argv0, '/') + 1) : argv0,  \
	        synopsis ? " " : "", synopsis ? synopsis : "");  \
	exit(2);  \
}


/**
 * Declares `argv0` and its value to
 * a specified string.
 * 
 * @param  name:sitrng literal  The name of the command.
 */
#define COMMAND(name)  \
const char *argv0 = name;


/**
 * Print usage and exit if there is any argument
 * that is an option.
 */
#define NO_OPTIONS  \
do {  \
	int i;  \
	if (!strcmp(argv[1], "--"))  \
		argv++, argc--;  \
	for (i = 1; i < argc; i++)  \
		if (strchr("-", argv[i][0]))  \
			usage();  \
} while (0)


/**
 * Construct a message from `argv`
 * to send to the daemon.
 */
#define CONSTRUCT_MESSAGE  \
	n = measure_array(argv + 1);  \
	t (n ? !(msg = malloc(n)) : 0);  \
	store_array(msg, argv + 1)


/**
 * Send message to daemon.
 * 
 * @param   cmd:enum command  Command type.
 * @param   n:size_t          The length of the message, 0 if
 *                            `msg` is `NULL` or NUL-terminated.
 * @param   msg:char *        The message to send.
 */
#define SEND(type, n, msg)  \
do {  \
	if (send_command(type, n, msg)) {  \
		t (errno);  \
		free(msg);  \
		return 3;  \
	}  \
} while (0)


/**
 * Exit the process with status indicating success.
 * 
 * Defined the label `fail`.
 * 
 * @param  msg  The message send to the daemon.
 */
#define END(msg)  \
	free(msg);  \
	return 0;  \
fail:  \
	perror(argv0);  \
	free(msg);  \
	return 1;

