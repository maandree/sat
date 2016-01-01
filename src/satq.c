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
#include <stdarg.h>



COMMAND("satq")
USAGE(NULL)



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
#define UNSAFE(c)      strchr(" \"$()[]{};|&^#!?*~`<>", c)
#define N(I, S, B, Q)  (I*in + S*sn + B*bn + Q*qn + rn)

	size_t in = 0; /* < ' ' or 127 */
	size_t sn = 0; /* in UNSAFE    */
	size_t bn = 0; /* = '\\'       */
	size_t qn = 0; /* = '\''       */
	size_t rn = 0; /* other        */
	size_t n, i = 0;
	const unsigned char *s;
	char *rc = NULL;

	for (s = (const unsigned char *)str; *s; s++) {
		if      (*s <  ' ')   in++;
		else if (*s == 127)   in++;
		else if (UNSAFE(*s))  sn++;
		else if (*s == '\\')  bn++;
		else if (*s == '\'')  qn++;
		else                  rn++;
	}
	if (N(1, 1, 1, 1) == rn)
		return strdup(rn ? str : "''");

	n = in ? (N(4, 1, 2, 2) + 3) : (N(0, 1, 1, 4) + 2);
	t (!(rc = malloc((n + 1) * sizeof(char))));
	rc[i += !!in] = '$';
	rc[i += 1]    = '\'';
	if (in == 0) {
		for (s = (const unsigned char *)str; *s; s++) {
			rc[i++] = (char)*s;
			if (*s == '\'')
				rc[i++] = '\\', rc[i++] = '\'', rc[i++] = '\'';
		}
	} else {
		for (s = (const unsigned char *)str; *s; s++) {
			if ((*s <  ' ') || (*s == 127)) {
				rc[i++] = '\\';
				rc[i++] = 'x';
				rc[i++] = "0123456789ABCDEF"[(*s >> 4) & 15];
				rc[i++] = "0123456789ABCDEF"[(*s >> 0) & 15];
			}
			else if (strchr("\\'", *s))  rc[i++] = '\\', rc[i++] = (char)*s;
			else                         rc[i++] = (char)*s;
		}
	}
	rc[i++] = '\'';
	rc[i] = '\0';
fail:
	return rc;
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
	int secs, mins, hours, sd = 0, md = 0, hd = 0;
	secs  = (int)(s % 60), s /= 60;
	mins  = (int)(s % 60), s /= 60;
	hours = (int)(s % 24), s /= 24;
	if (s)           hd++, buf += sprintf(buf, "%llid", (long long int)s);
	if (hd | hours)  md++, buf += sprintf(buf, "%0*i:", ++hd, hours);
	if (md | mins)   sd++, buf += sprintf(buf, "%0*i:", ++md, mins);
	/*just for alignment*/ buf += sprintf(buf, "%0*i",  ++sd, secs);
}


/**
 * Prints a series of strings without any restrict
 * (in constrast to the `printf` function) of the
 * length of the strings.
 * 
 * @param   s...  The strings to print. `NULL`-terminated.
 * @return        0 on success, -1 on error.
 */
static int
print(const char *s, ...)
{
	va_list args;
	size_t i, n = 0;
	ssize_t r = 1;
	va_start(args, s);
	do
		for (i = 0, n = strlen(s); (r > 0) && (i < n); i += (size_t)r)
			r = write(STDOUT_FILENO, s + i, n - i);
	while ((r > 0) && ((s = va_arg(args, const char *))));
	va_end(args);
	return r > 0 ? 0 : -1;
}


/**
 * Dump a job to the socket.
 * 
 * @param   job  The job.
 * @return       0 on success, -1 on error.
 */
static int
print_job(struct job *job)
{
#define FIX_NSEC(T)  (((T)->tv_nsec < 0L) ? ((T)->tv_sec -= 1, (T)->tv_nsec += 1000000000L) : 0L)
#define ARRAY(LIST)  \
	for (arg = LIST; *arg; arg++) {  \
		free(qstr);  \
		t (!(qstr = quote(*arg)));  \
		t (print(" ", qstr, NULL));  \
	}

	struct tm *tm;
	struct timespec rem;
	const char *clk;
	char rem_s[3 * sizeof(time_t) + sizeof("d00:00:00")];
	char *qstr = NULL;
	char *wdir = NULL;
	char line[sizeof("job: %zu clock: unrecognised argc: %i remaining:  argv[0]: ")
		  + 3 * sizeof(size_t) + 3 * sizeof(int) + sizeof(rem_s) + 9];
	char timestr_a[sizeof("-00-00 00:00:00") + 3 * sizeof(time_t)];
	char timestr_b[10];
	char **args = NULL;
	char **arg;
	char **argv = NULL;
	char **envp = NULL;
	size_t argsn;
	int rc = 0, saved_errno;

	/* Get remaining time. */
	if (clock_gettime(job->clk, &rem))
		return errno == EINVAL ? 0 : -1;
	rem.tv_sec  = job->ts.tv_sec  - rem.tv_sec;
	rem.tv_nsec = job->ts.tv_nsec - rem.tv_nsec;
	FIX_NSEC(&rem);
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
	if (job->clk == CLOCK_REALTIME) {
		t (!(tm = localtime(&(job->ts.tv_sec))));
		strftime(timestr_a, sizeof(timestr_a), "%Y-%m-%d %H:%M:%S", tm);
	} else {
		strduration(timestr_a, job->ts.tv_sec);
	}
	sprintf(timestr_b, "%09li", job->ts.tv_nsec);

	/* Get arguments. */
	t (!(args = restore_array(job->payload, job->n, &argsn)));
	t (!(argv = sublist(args, (size_t)(job->argc))));
	t (!(envp = sublist(args + job->argc, argsn - (size_t)(job->argc)))); /* Includes wdir. */

	/* Send message. */
	t (!(qstr = quote(args[0])));
	t (!(wdir = quote(envp[0])));
	sprintf(line, "job: %zu clock: %s argc: %i remaining: %s.%09li argv[0]: ",
		job->no, clk, job->argc, rem_s, rem.tv_nsec);
	t (print(line, qstr,
	         "\n  time: ", timestr_a, ".", timestr_b,
	         "\n  wdir: ", wdir,
	         "\n  argv:", NULL));
	ARRAY(argv);      t (print("\n  envp:", NULL));
	ARRAY(envp + 1);  t (print("\n\n", NULL));

done:
	S(free(qstr), free(args), free(argv), free(wdir), free(envp));
	return rc;
fail:
	rc = -1;
	goto done;
}


/**
 * Print all queued jobs.
 * 
 * @param   argc  Should be 1 or 0.
 * @param   argv  The command line, should only include the name of the process.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 * @return  2     User error, you do not know what you are doing.
 */
int
main(int argc, char *argv[])
{
	struct job **jobs = NULL;
	struct job **job;
	PROLOGUE(argc < 2, O_RDONLY);

	t (!(jobs = get_jobs()));
	for (job = jobs; *job; job++)
		t (print_job(*job));

	CLEANUP_START;
	for (job = jobs; jobs && *job; job++)
		free(*job);
	free(jobs);
	CLEANUP_END;
}

