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
#include <stddef.h>



/**
 * Commands for `send_command`.
 */
enum command {
	/**
	 * Queue a job.
	 */
	SAT_QUEUE = 0,

	/**
	 * Remove jobs.
	 */
	SAT_REMOVE = 1,

	/**
	 * Print job queue.
	 */
	SAT_PRINT = 2,

	/**
	 * Run jobs.
	 */
	SAT_RUN = 3
};



/**
 * Send a command to satd. Start satd if it is not running.
 * 
 * @param   cmd  Command type.
 * @param   n    The length of the message, 0 if `msg` is
 *               `NULL` or NUL-terminated.
 * @param   msg  The message to send.
 * @return       Zero on success.
 * 
 * @throws  0  Error at the daemon-side.
 */
int send_command(enum command cmd, size_t n, const char *restrict msg);


/**
 * Return the number of bytes required to store a string array.
 * 
 * @param   array  The string array.
 * @return         The number of bytes required to store the array.
 */
size_t measure_array(char *array[]);

/**
 * Store a string array.
 * 
 * @param   storage  The buffer where the array is to be stored.
 * @param   array    The array to store.
 * @return           Where in the buffer the array ends.
 */
char *store_array(char *restrict storage, char *array[]);

