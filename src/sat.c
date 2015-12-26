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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "parse_time.h"
#include "client.h"



/**
 * The name of the process.
 */
char *argv0 = "sat";



/**
 * Print usage information.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: %s TIME COMMAND...\n",
	        strrchr(argv0) ? (strrchr(argv0) + 1) : argv0);
	exit(2);
}


/**
 * Queue a job for later execution.
 * 
 * @param   argc  You guess!
 * @param   argv  The first element should be the name of the process,
 *                the second argument should be the POSIX time (seconds
 *                since Epoch (1970-01-01 00:00:00 UTC), disregarding
 *                leap seconds) the job shall be executed. The rest of
 *                the arguments (being at least one) shoul be the
 *                command line arguments for the job the run.
 * @param   envp  The environment.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 * @return  2     User error, you do not know what you are doing.
 * @return  3     satd(1) failed to queue the job.
 */
int
main(int argc, char *argv[], char *envp[])
{
	struct timespec ts;
	clockid_t clk;
	char *msg = NULL;
	size_t n;

	if ((argc < 3) || (argv[1][0] == '-')) {
		usage();
	}

	argv0 = argv[0];

	/* Parse the time argument. */
	if (parse_time(argv[1], &ts, &clk)) {
		switch (errno) {
		case EINVAL:
			fprintf(stderr,
			        "%s: time parameter cound not be parsed, perhaps you "
			        "you need an external parser: %s\n", argv0, str);
			return 2;
		case ERANGE:
			fprintf(stderr,
			        "%s: the specified time is beyond the limit of what "
				"can be represented by `struct timespec`: %s\n", argv0, str);
			return 2;
		case EDOM:
			fprintf(stderr,
			        "%s: the specified time is in past, and more than "
				"a day ago: %s\n", argv0, str);
			return 2;
		default:
			goto fail;
		}
	}

	argc -= 2;
	argv += 2;

	/* Construct message to send to the daemon. */
	n = measure_array(argv) + measure_array(envp);
	if (!(msg = malloc(n + sizeof(clk) + sizeof(ts))))
		goto fail;
	store_array(store_array(msg, argv), envp);
	memcpy(msg + n, clk, sizeof(clk)), n += sizeof(clk);
	memcpy(msg + n, ts,  sizeof(ts)),  n += sizeof(ts);

	/* Send job to daemon, start daemon if necessary. */
	if (send_command(SAT_QUEUE, n, msg)) {
		if (errno)
			goto fail;
		free(msg);
		return 3;
	}
	return 0;

fail:
	perror(argv0);
	free(msg);
	return 1;
}

