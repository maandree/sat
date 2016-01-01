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
# ifndef DEBUG
#  define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)
# else
#  define t(...)  do { if ((__VA_ARGS__) ? (failed__ = #__VA_ARGS__) : 0) { (perror)(failed__); goto fail; } } while (0)
static const char *failed__ = NULL;
#  define perror(_)  ((void)(_))
# endif
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


/**
 * This block of code allows us to compile with DEBUG=valgrind
 * or DEBUG=strace and have all exec:s be wrapped in
 * valgrind --leak-check=full --show-leak-kinds=all or
 * strace, respectively. Very useful for debugging. However,
 * children do not inherit strace, so between forking and
 * exec:ing we do not have strace.
 */
#if 1 && defined(DEBUG)
# if ((DEBUG == 2) || (DEBUG == 3))
#  ifdef __GNUC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
__attribute__((__used__))
#  endif
#  if DEBUG == 2
#   define DEBUGPROG  "strace"
#  else
#   define DEBUGPROG  "valgrind"
#  endif
#  define execl(path, _, ...) (execl)("/usr/bin/" DEBUGPROG, "/usr/bin/" DEBUGPROG, path, __VA_ARGS__)
#  define execve(...) execve_(__VA_ARGS__)
static int
execve_(const char *path, char *const argv[], char *const envp[])
{
	size_t n = 0;
	char **new_argv = NULL;
	int x = (DEBUG - 2) * 2, saved_errno;
	while (argv[n++]);
	t (!(new_argv = malloc((n + 1 + (size_t)x) * sizeof(char *))));
	new_argv[x] = "--show-leak-kinds=all";
	new_argv[1] = "--leak-check=full";
	new_argv[0] = "/usr/bin/" DEBUGPROG;
	new_argv[1 + x] = path;
	memcpy(new_argv + 2 + x, argv + 1, (n - 1) * sizeof(char *));
	(execve)(*new_argv, new_argv, envp);
fail:
	return saved_errno = errno, free(new_argv), errno = saved_errno, -1;
}
#  ifdef __GNUC__
#   pragma GCC diagnostic pop
#  endif
# endif
#endif

