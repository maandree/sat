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
#include "parse_time.h"
#include "client.h"
#include "common.h"
#include <errno.h>
#include <unistd.h>



COMMAND("sat")
USAGE("TIME COMMAND...")



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
#define E(CASE, DESC)  case CASE: return fprintf(stderr, "%s: %s: %s\n", argv0, DESC, argv[1]), 2

	struct timespec ts;
	clockid_t clk;
	char *msg = NULL;
	void *new;
	size_t n, size = 64;

	if ((argc < 3) || (argv[1][0] == '-'))
		usage();

	argv0 = argv[0];

	/* Parse the time argument. */
	if (parse_time(argv[1], &ts, &clk)) {
		switch (errno) {
		E (EINVAL, "time parameter cound not be parsed, perhaps you need an external parser");
		E (ERANGE, "the specified time is beyond the limit of what can be stored");
		E (EDOM,   "the specified time is in past, and more than a day ago");
		default: goto fail;
		}
	}

	argc -= 2;
	argv += 2;

retry:
	/* Get the size of the current working directory's pathname. */
	t (!(new = realloc(msg, size <<= 1)));
	if (!getcwd(msg = new, size)) {
		t (errno != ERANGE);
		goto retry;
	}
	size = strlen(getcwd(msg, size)) + 1, free(msg);

	/* Construct message to send to the daemon. */
	n = measure_array(argv) + size + measure_array(envp);
	t (!(msg = malloc(n + sizeof(argc) + sizeof(clk) + sizeof(ts))));
	store_array(getcwd(store_array(msg, argv), size) + size, envp);
	memcpy(msg + n, &argc, sizeof(argc)), n += sizeof(argc);
	memcpy(msg + n, &clk,  sizeof(clk)),  n += sizeof(clk);
	memcpy(msg + n, &ts,   sizeof(ts)),   n += sizeof(ts);

	/* Send job to daemon, start daemon if necessary. */
	SEND(SAT_QUEUE, n, msg);

	END(msg);
}

