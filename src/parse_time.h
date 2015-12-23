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
#include <time.h>



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
parse_time(const char *str, struct timespec *ts, clockid_t *clk);

