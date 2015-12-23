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
 * @param  argc  You guess!
 * @param  argv  The first element should be the name of the process,
 *               the second argument should be the POSIX time (seconds
 *               since Epoch (1970-01-01 00:00:00 UTC), disregarding
 *               leap seconds) the job shall be executed. The rest of
 *               the arguments (being at least one) shoul be the
 *               command line arguments for the job the run.
 * @return  0    The process was successful.
 * @return  1    The process failed queuing the job.
 * @return  2    User error, you do not know what you are doing.
 * @return  3    satd(1) failed to queue the job.
 */
int
main(int argc, char *argv[])
{
	struct timespec ts;
	clockid_t clk;

	if ((argc < 3) || (argv[1][0] == '-')) {
		usage();
	}

	argv0 = argv[0];

	if (parse_time(str, &ts, &clk)) {
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

fail:
	perror(argv0);
	return 1;
}

