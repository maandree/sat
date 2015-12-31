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
#include <time.h>
#include <sys/timerfd.h>



/**
 * Pretty self-explanatory.
 */
#ifdef __GNUC__
__attribute__((__pure__))
#endif
static inline int
timecmp(const struct timespec *a, const struct timespec *b)
{
	if (a->tv_sec  != b->tv_sec)   return (a->tv_sec  < b->tv_sec  ? -1 : +1);
	if (a->tv_nsec != b->tv_nsec)  return (a->tv_nsec < b->tv_nsec ? -1 : +1);
	return 0;
}


/**
 * Subroutine to the sat daemon: list jobs.
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
#define TIME(job, suffix)  ((job)->clk == CLOCK_BOOTTIME ? &boot##suffix : &real##suffix)

	char jobno[3 * sizeof(size_t) + 1];
	struct itimerspec bootspec;
	struct itimerspec realspec;
	struct timespec bootnow;
	struct timespec realnow;
	struct job **jobs = NULL;
	struct job **job;
	int rc = 0;

	t (reopen(STATE_FILENO, O_RDWR));

	/* Get current expiration time. */
	t (timerfd_gettime(BOOT_FILENO, &bootspec));
	t (timerfd_gettime(REAL_FILENO, &realspec));

	/* Run expired jobs, and find new expiration times. */
	t (clock_gettime(CLOCK_BOOTTIME, &bootnow));
	t (clock_gettime(CLOCK_REALTIME, &realnow));
	t (!(jobs = get_jobs()));
	for (job = jobs; *job; job++) {
		if (timecmp(&(job[0]->ts), TIME(*job, now)) >= 0) {
			sprintf(jobno, "%zu", job[0]->no);
			remove_job(jobno, 2);
		} else if (timecmp(&(job[0]->ts), &(TIME(*job, spec)->it_value)) > 0) {
			TIME(*job, spec)->it_value = job[0]->ts;
		}
	}

	/* Update expiration time. */
	t (timerfd_settime(BOOT_FILENO, TFD_TIMER_ABSTIME, &bootspec, NULL));
	t (timerfd_settime(REAL_FILENO, TFD_TIMER_ABSTIME, &realspec, NULL));

done:
	for (job = jobs; *job; job++)
		free(*job);
	free(jobs);
	close(STATE_FILENO);
	return rc;
fail:
	perror(argv[0]);
	rc = 1;
	goto done;
	(void) argc;
}

