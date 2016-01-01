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
#include "common.h"
#include "parse_time.h"



COMMAND("sat")
USAGE("TIME COMMAND...")



/**
 * Return the number of bytes required to store a string array.
 * 
 * @param   array  The string array.
 * @return         The number of bytes required to store the array.
 */
#ifdef __GNUC__
__attribute__((__pure__))
#endif
static size_t
measure_array(char *array[])
{
	size_t rc = 0;
	while (*array)  rc += strlen(*array++) + 1;
	return rc * sizeof(char);
}


/**
 * Store a string array.
 * 
 * @param   storage  The buffer where the array is to be stored.
 * @param   array    The array to store.
 * @return           Where in the buffer the array ends.
 */
static char *
store_array(char *restrict storage, char *array[])
{
	for (; *array; array++, storage++)
		storage = stpcpy(storage, *array);
	return storage;
}


/**
 * Construct the job specifications, as a storable unit.
 * 
 * @param   argc  `argc` from `main`, see `main` for descriptor.
 * @param   argv  `argv` from `main`, see `main` for descriptor.
 * @param   envp  `envp` from `main`, see `main` for descriptor.
 * @return        The job (sans serial number) on success, `NULL` on error.
 */
static struct job *
construct_job(int argc, char *argv[], char *envp[])
{
#define E(CASE, DESC)       case CASE: fprintf(stderr, "%s: %s: %s\n", argv0, DESC, argv[1]), exit(2)

	char *dummy = NULL;
	char *timearg;
	void *new;
	size_t size = 64;
	struct job job = { .no = 0 };
	struct job *job_full = NULL;
	int saved_errno;

	timearg = argv[1];
	job.argc = argc -= 2, argv += 2;

	/* Parse the time argument. */
	if (parse_time(timearg, &(job.ts), &(job.clk))) {
		switch (errno) {
		E (EINVAL, "time parameter cound not be parsed, perhaps you need an external parser");
		E (ERANGE, "the specified time is beyond the limit of what can be stored");
		E (EDOM,   "the specified time is in past, and more than a day ago");
		default: goto fail;
		}
	}

retry:
	/* Get the size of the current working directory's pathname. */
	t (!(new = realloc(dummy, size <<= 1)));
	if (!getcwd(dummy = new, size)) {
		t (errno != ERANGE);
		goto retry;
	}
	size = strlen(getcwd(dummy, size)) + 1;

	/* Construct full specification. */
	job.n = measure_array(argv) + size + measure_array(envp);
	t (!(job_full = malloc(sizeof(job) + job.n)));
	memcpy(job_full, &job, sizeof(job));
	store_array(getcwd(store_array(job_full->payload, argv), size) + size, envp);

fail:
	return S(free(dummy)), job_full;
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
#define WRITE(MSG, N, OFF)  t (pwriten(STATE_FILENO, MSG, (size_t)(N), (size_t)(OFF)) < (ssize_t)(N))

	struct job *job = NULL;
	struct stat attr;
	ssize_t r;
	PROLOGUE((argc > 2) && (argv[1][0] != '-'), O_RDWR);
	t (set_hookpath());

	t (!(job = construct_job(argc, argv, envp)));

	/* Update state file and run hook. */
	t (flock(STATE_FILENO, LOCK_EX));
	t (fstat(STATE_FILENO, &attr));
	t (r = preadn(STATE_FILENO, &(job->no), sizeof(job->no), (size_t)0), r < 0);
	if (r < (ssize_t)sizeof(job->no))  job->no = 0;
	else                               job->no += 1;
	WRITE(&(job->no), sizeof(job->no), (size_t)0);
	if (attr.st_size < (off_t)sizeof(job->no))
		attr.st_size = (off_t)sizeof(job->no);
	WRITE(job, sizeof(*job) + job->n, (size_t)(attr.st_size));
	fsync(STATE_FILENO);
	run_job_or_hook(job, "queued");
	t (flock(STATE_FILENO, LOCK_UN));

	t (poke_daemon(1, argv0));
	CLEANUP_START;
	free(job);
	CLEANUP_END;
}

