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
#include <errno.h>

#include "client.h"



/**
 * The name of the process.
 */
char *argv0 = "satrm";



/**
 * Print usage information.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: %s JOB-ID\n",
	        strrchr(argv0) ? (strrchr(argv0) + 1) : argv0);
	exit(2);
}


/**
 * Remove a job from the queue of jobs.
 * 
 * @param   argc  Should be 2.
 * @param   argv  The command line, should only include the name
 *                of the process and the ID of the job to remove.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 * @return  2     User error, you do not know what you are doing.
 * @return  3     satd(1) failed.
 */
int
main(int argc, char *argv[])
{
	if (argc != 2) {
		usage();
	}
	argv0 = argv[0];

	if (send_command(0, argv[1]))
		return errno ? (perror(argv0), 1) : 3;
	return 0;
}

