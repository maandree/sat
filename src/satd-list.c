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
 * Quote a string, in shell (Bash-only if necessary) compatible
 * format, if necessary. Here, just adding quotes around all not
 * do. The string must be single line, and there must not be
 * any invisible characters; it should be possible to copy
 * a string from the terminal by marking it, hence all of this
 * ugliness.
 * 
 * @param   str  The string.
 * @return       Return a safe representation of the string,
 *               `NULL` on error.
 */
static char *
quote(const char *str)
{
#define UNSAFE(c)  strchr(" \"$()[]{};|&^#!?*~`<>", c)

	size_t in = 0; /* < ' '     */
	size_t sn = 0; /* in UNSAFE */
	size_t bn = 0; /* = '\\'    */
	size_t qn = 0; /* = '\''    */
	size_t rn = 0; /* other     */
	size_t n, i = 0;
	const unsigned char *s;
	char *rc;

	for (s = (const unsigned char *)str; *s; s++) {
		if      (*s <  ' ')   in++;
		else if (UNSAFE(*s))  sn++;
		else if (*s == '\\')  bn++;
		else if (*s == '\'')  qn++;
		else                  rn++;
	}

	switch (in ? 2 : (sn + bn + qn) ? 1 : 0) {
	case 0:
		return strdup(str);
	case 1:
		n = rn + sn + bn + 4 * qn + 4 * in + 2;
		t (!(rc = malloc((n + 1) * sizeof(char))));
		rc[i++] = '\'';
		for (s = (const unsigned char *)str; *s; s++) {
			rc[i++] = (char)*s;
			if (*s == '\'')
				rc[i++] = '\\', rc[i++] = '\'', rc[i++] = '\'';
				
		}
		rc[i++] = '\'';
		rc[i] = '\0';
		return rc;
	case 2:
		n = 4 * in + rn + sn + 2 * bn + 2 * qn + 3;
		t (!(rc = malloc((n + 1) * sizeof(char))));
		rc[i++] = '$';
		rc[i++] = '\'';
		for (s = (const unsigned char *)str; *s; s++) {
			if (*s <  ' ') {
				rc[i++] = '\\';
				rc[i++] = 'x';
				rc[i++] = "0123456789ABCDEF"[(*s >> 4) & 15];
				rc[i++] = "0123456789ABCDEF"[(*s >> 0) & 15];
			}
			else if (*s == '\\')  rc[i++] = '\\', rc[i++] = (char)*s;
			else if (*s == '\'')  rc[i++] = '\\', rc[i++] = (char)*s;
			else                  rc[i++] = (char)*s;
		}
		rc[i++] = '\'';
		rc[i] = '\0';
		return rc;
	}
fail:
	return NULL;
}


/**
 * Create a textual representation of the a duration.
 * 
 * @param  buffer  Output buffer, with a size of at least
 *                 10 `char`:s plus enough to encode a `time_t`.
 * @param  s       The duration in seconds.
 */
static void
strduration(char *buffer, time_t s)
{
	char *buf = buffer;
	int seconds, minutes, hours;
	seconds = s % 60, s /= 60;
	minutes = s % 60, s /= 60;
	hours   = s % 24, s /= 24;
	if (s) {
		buf += sprintf(buf, "%llid", (long long int)s);
		buf += sprintf(buf, "%02i:", hours);
		buf += sprintf(buf, "%02i", minutes);
	} else if (hours) {
		buf += sprintf(buf, "%i:", hours);
		buf += sprintf(buf, "%02i", minutes);
	} else if (minutes) {
		buf += sprintf(buf, "%i", minutes);
	}
	sprintf(buf, "%02i", seconds);
}


/**
 * Dump a job to the socket.
 * 
 * @param   job  The job.
 * @return       0 on success, -1 on error.
 */
static int
send_job_human(struct job* job)
{
	struct tm *tm;
	struct timespec rem;
	const char *clk;
	char rem_s[3 * sizeof(time_t) + sizeof("d00:00:00")];
	char *qstr = NULL;
	char line[sizeof("job: %zu clock: unrecognised argc: %i remaining: , argv[0]: ")
		  + 3 * sizeof(size_t) + 3 * sizeof(int) + sizeof(rem_s) + 9];
	char timestr_a[sizeof("0000-00-00 00:00:00") + 3 * sizeof(time_t)];
	char timestr_b[10];
	char **args = NULL;
	char **arg;
	char **argv = NULL;
	char **envp = NULL;
	size_t argsn;
	int rc = 0;
	int saved_errno;

	/* Get remaining time. */
	if (clock_gettime(job->clk, &rem))
		return errno == EINVAL ? 0 : -1;
	rem.tv_sec  -= job->ts.tv_sec;
	rem.tv_nsec -= job->ts.tv_nsec;
	if (rem.tv_nsec < 0) {
		rem.tv_sec -= 1;
		rem.tv_nsec += 1000000000L;
	}
	if (rem.tv_sec < 0)
		/* This job will be removed momentarily, do not list it. (To simply things.) */
		return 0;

	/* Get clock name. */
	switch (job->clk) {
	case CLOCK_REALTIME:  clk = "walltime";      break;
	case CLOCK_BOOTTIME:  clk = "boottime";      break;
	default:              clk = "unrecognised";  break;
	}

	/* Get textual representation of the remaining time. (Seconds only.) */
	strduration(rem_s, rem.tv_sec);

	/* Get textual representation of the expiration time. */
	switch (job->clk) {
	case CLOCK_REALTIME:
		t (!(tm = localtime(&(job->ts.tv_sec))));
		strftime(timestr_a, sizeof(timestr_a), "%Y-%m-%d %H:%M:%S", tm);
		break;
	default:
		strduration(timestr_a, job->ts.tv_sec);
		break;
	}
	sprintf(timestr_b, "%09li", job->ts.tv_nsec);

	/* Get arguments. */
	t (!(args = restore_array(job->payload, job->n, &argsn)));
	t (!(argv = sublist(args, (size_t)(job->argc))));
	t (!(envp = sublist(args + job->argc, argsn - (size_t)(job->argc))));

	/* Send message. */
	t (!(qstr = quote(args[0])));
	sprintf(line, "job: %zu clock: %s argc: %i remaining: %s.%09li, argv[0]: ",
		job->no, clk, job->argc, rem_s, rem.tv_nsec);
	t (send_string(SOCK_FILENO, STDOUT_FILENO,
		       line, qstr, "\n",
		       "  time: ", timestr_a, ".", timestr_b, "\n",
		       "  argv:",
		       NULL));
	for (arg = argv; *arg; arg++) {
		free(qstr);
		t (!(qstr = quote(*arg)));
		t (send_string(SOCK_FILENO, STDOUT_FILENO, " ", qstr, NULL));
	}
	free(qstr), qstr = NULL;
	t (send_string(SOCK_FILENO, STDOUT_FILENO, "\n  envp:", NULL));
	for (arg = envp; *arg; arg++) {
		t (!(qstr = quote(*arg)));
		t (send_string(SOCK_FILENO, STDOUT_FILENO, " ", qstr, NULL));
		free(qstr);
	}
	qstr = NULL;
	t (send_string(SOCK_FILENO, STDOUT_FILENO, "\n\n", NULL));

done:
	saved_errno = errno;
	free(qstr);
	free(args);
	free(argv);
	free(envp);
	errno = saved_errno;
	return rc;

fail:
	rc = -1;
	goto done;
}



/**
 * Subroutine to the sat daemon: list jobs.
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
	size_t n = 0;
	char *message = NULL;
	struct job** jobs;
	struct job** job;
	int rc = 0;

	assert(argc == 4);
	t (reopen(STATE_FILENO, O_RDWR));

	/* Receive and validate message. */
	t (readall(SOCK_FILENO, &message, &n) || n);
	shutdown(SOCK_FILENO, SHUT_RD);

	/* Perform action. */
	t (!(jobs = get_jobs()));
	for (job = jobs; *job; job++)
		t (send_job_human(*job));

done:
	/* Cleanup. */
	shutdown(SOCK_FILENO, SHUT_WR);
	close(SOCK_FILENO);
	for (job = jobs; *job; job++)
		free(*job);
	free(jobs);
	free(message);
	return rc;
fail:
	if (send_string(SOCK_FILENO, STDERR_FILENO, argv[0], ": ", strerror(errno), "\n", NULL))
		perror(argv[0]);
	rc = 1;
	goto done;

	(void) argc;
}

