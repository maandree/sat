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
#define _DEFAULT_SOURCE
#include "parse_time.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>



#define W(...)  do { __VA_ARGS__ } while (0)


/**
 * The number of seconds in a day.
 */
#define ONE_DAY  (time_t)(24 * 60 * 60)

/**
 * Set errno and return.
 * 
 * @return  e:int  The error number.
 */
#define FAIL(e)  return errno = (e), -1

/**
 * Set errno to EINVAL and return if a condition is not met.
 * 
 * @return  cond:int  The condition.
 */
#define REQUIRE(cond)  W(if (!(cond))  FAIL(EINVAL);)

/**
 * `a *= b` with overflow check.
 */
#define MUL(a, b)  W(if (a > timemax / (b))  FAIL(ERANGE);  else  a *= (b);)

/**
 * `a += b` with overflow check.
 */
#define ADD(a, b)  W(if (a > timemax - (b))  FAIL(ERANGE);  else  a += (b);)



/**
 * The name of the process.
 */
extern char *argv0;

/**
 * The highest value that can be stored in `time_t`.
 */
static const time_t timemax = (sizeof(time_t) == sizeof(long long int)) ? (time_t)LLONG_MAX : (time_t)LONG_MAX;



/**
 * Wrapper for `strtoll` and `strtol` that selects
 * the appropriate function for `time_t`, does not
 * allow whitespace or signs. It also forces the
 * base to be 10, and will only return 0 on error,
 * and will set `errno` on success.
 * 
 * @param   str  Please see strtol(3), first parameter.
 * @param   end  Please see strtol(3), second parameter.
 * @return       Please see strtol(3), only 0 may be
 *               returned at error.
 * 
 * @throws  0  On success.
 * @throws     Please see strtol(3).
 */
static time_t
strtotime(const char *str, const char **end)
{
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif
	time_t rc;

	REQUIRE(isdigit(*str));
	errno = 0;

	/* The outer if-statement is for when `time_t` is update to `long long int`.
	 * which is need to avoid the year 2038 problem. Be we have to use `strtol`
	 * if `time_t` is `long int`, otherwise we will not detect overflow. */

	if (sizeof(time_t) == sizeof(long long int))
		rc = (time_t)strtoll(str, (char **)end, 10);
	else
		rc = (time_t)strtol(str, (char **)end, 10);

	return errno ? 0 : rc;
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
}


/**
 * Parse a time on the format HH:MM[:SS].
 * 
 * @param   str  Pointer to time string, will be updated to the end
 *               of the parsing of it.
 * @param   ts   Output parameter for the POSIX time the string
 *               represents.
 * @return       0 on success, -1 on error.
 * 
 * @throws  EINVAL  `str` could not be parsed.
 * @throws  ERANGE  `str` specifies a time beyond what can be stored.
 */
static int
parse_time_time(const char **str, struct timespec *ts)
{
	time_t tm;

	/* Hours. */
	t (ts->tv_sec = strtotime(*str, str), errno);
	/* Must not be restricted to 23, beyond 24 is legal. */
	MUL(ts->tv_sec, (time_t)(60 * 60));

	/* Minutes. */
	REQUIRE(*(*str)++ == ':');
	t (tm = strtotime(*str, str), errno);
	REQUIRE(tm < 60);
	MUL(tm, (time_t)60);
	ADD(ts->tv_sec, tm);

	/* Seconds. */
	if (*(*str)++ != ':')  return 0;
	t (tm = strtotime(*str, str), errno);
	/* Do not restrict to 59, or 60, there can be more than +1 leap second. */
	ADD(ts->tv_sec, tm);

	return 0;
fail:
	return -1;
}


/**
 * Parse a time with only a second-count.
 * 
 * @param   str  Pointer to time string, will be updated to the end
 *               of the parsing of it.
 * @param   ts   Output parameter for the POSIX time the string
 *               represents.
 * @return       0 on success, -1 on error.
 * 
 * @throws  EINVAL  `str` could not be parsed.
 * @throws  ERANGE  `str` specifies a time beyond what can be stored.
 */
static int
parse_time_seconds(const char **str, struct timespec *ts)
{
	ts->tv_sec = strtotime(*str, str);
	return errno ? -1 : 0;
}


/**
 * Trivial time-parsing.
 * 
 * @param   str  The time string.
 * @param   ts   Output parameter for the POSIX time the string
 *               represents. If the time is in the pasted, but
 *               within a day, one day is added. This is in case
 *               you are using a external parser that did not
 *               release they you wanted a future time but
 *               thought you wanted the time to be the closed or
 *               in the same day.
 * @param   clk  Output parameter for the clock in which the
 *               returned time is specified.
 * @return       0 on success, -1 on error.
 * 
 * @throws  EINVAL  `str` could not be parsed.
 * @throws  ERANGE  `str` specifies a time beyond what can be stored.
 * @throws  EDOM    The specified the was over a day ago.
 * 
 * @futuredirection  Add environment variable that allows
 *                   parsing through another program.
 */
int
parse_time(const char *str, struct timespec *ts, clockid_t *clk)
{
#define FIX_NSEC(T)  (((T)->tv_nsec >= 1000000000L) ? ((T)->tv_sec += 1, (T)->tv_nsec -= 1000000000L) : 0L)

	struct timespec now;
	int points, plus = *str == '+';
	const char *start = str;
	time_t adj;

	ts->tv_nsec = 0;

	/* Get current time and clock. */
	*clk = plus ? CLOCK_BOOTTIME : CLOCK_REALTIME;
	clock_gettime(*clk, &now);

	/* Mañana? */
	if (!strcmp(str, "mañana")) { /* Do not documented. */
		ts->tv_sec = now.tv_sec + ONE_DAY;
		ts->tv_nsec = now.tv_nsec;
		return 0;
	}

	/* HH:MM[:SS[.NNNNNNNNN]] or seconds? */
	if (strchr(str += plus, ':')) {
		t (parse_time_time(&str, ts));
		adj = now.tv_sec - (now.tv_sec % ONE_DAY);
		ADD(ts->tv_sec, adj); /* In case the HH is really large. */
	} else {
		t (parse_time_seconds(&str, ts));
	}

	/* Parse up to nanosecond resolution. */
	if (*str != '.')
		goto no_nanoseconds;
	for (points = 0, str++; isdigit(*str); points++, str++) {
		if (points < 9) {
			ts->tv_nsec *= 10;
			ts->tv_nsec += *str & 15;
		} else if ((points == 9) && (*str >= '5')) {
			ts->tv_nsec += 1;
			/* I would like to have FIX_NSEC here, but the
			 * compile will complain and it is not worth it. */
		}
	}
	while (points++ < 9)  ts->tv_nsec *= 10;
	FIX_NSEC(ts);
no_nanoseconds:

	/* Check for error at end, and missing explicit UTC. */
	if (*str) {
		REQUIRE(*clk != CLOCK_BOOTTIME);
		while (*str == ' ')  str++;
		REQUIRE(!strcasecmp(str, "Z") || !strcasecmp(str, "UTC"));
	} else if (*clk == CLOCK_REALTIME) {
		fprintf(stderr,
		        "%s: warning: parsing as UTC, you can avoid this warning "
		        "by adding a 'Z' at the end of the time argument.\n", argv0);
	}

	/* Add offset or djust the day? */
	if (*clk == CLOCK_BOOTTIME) {
		ts->tv_sec += now.tv_sec;
		ts->tv_nsec += now.tv_nsec;
		FIX_NSEC(ts);
	} else if (ts->tv_sec < now.tv_sec) { /* Ignore partial second. */
		ts->tv_sec += ONE_DAY;
		if (ts->tv_sec < now.tv_sec)
			FAIL(EDOM);
		if (!strchr(start, ':'))
			fprintf(stderr,
			        "%s: warning: the specified time is in the past, "
			        "it is being adjust to be tomorrow instead.\n", argv0);
	}

	return 0;
fail:
	return -1;
}

