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

#include "daemonise.h"



/**
 * The name of the process.
 */
char *argv0 = "satd";



/**
 * Print usage information.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: [-f] %s\n",
	        strrchr(argv0) ? (strrchr(argv0) + 1) : argv0);
	exit(2);
}


int
main(int argc, char *argv[])
{
	int foreground = 0;

	if (argc > 0)
		argv0 = argv[0];
	if (argc > 2)
		usage();
	if (argc == 2) {
		if (strcmp(argv[1], "-f"))
			usage();
		foreground = 1;
	}

	if (foreground ? 0 : daemonise(0))
		goto fail;

	undaemonise();
	return 0;

fail:
	perror(argv0);
	undaemonise();
	return 1;
}

