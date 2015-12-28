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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>



/**
 * Wrapper for `read` that reads all available data.
 * 
 * Sets `errno` to `EBADMSG` on success.
 * 
 * @param   fd   The file descriptor from which to to read.
 * @param   buf  Output parameter for the data.
 * @param   n    Output parameter for the number of read bytes.
 * @return       0 on success, -1 on error.
 * 
 * @throws  Any exception specified for read(3).
 * @throws  Any exception specified for realloc(3).
 */
int
readall(int fd, char **buf, size_t *n)
{
	char *buffer = NULL;
	size_t ptr = 0;
	size_t size = 0;
	ssize_t got;
	char *new;
	int saved_errno;

	for (;;) {
		if (ptr == size) {
			new = realloc(buffer, size <<= 1);
			t (!new);
			buffer = new;
		}
		got = read(fd, buffer + ptr, size - ptr);
		t (got < 0);
		if (got == 0)
			break;
		ptr += (size_t)got;
	}

	new = realloc(buffer, ptr);
	*buf = new ? new : buffer;
	*n = ptr;
	return 0;

fail:
	saved_errno = errno;
	free(buffer);
	errno = saved_errno;
	return -1;
}


/**
 * Unmarshal a `NULL`-terminated string array.
 * 
 * The elements are not actually copied, subpointers
 * to `buf` are stored in the returned list.
 * 
 * @param   buf  The marshalled array. Must end with a NUL byte.
 * @param   len  The length of `buf`.
 * @param   n    Output parameter for the number of elements. May be `NULL`
 * @return       The list, `NULL` on error.
 * 
 * @throws  Any exception specified for realloc(3).
 */
char **
restore_array(char *buf, size_t len, size_t *n)
{
	char **rc = malloc((len + 1) * sizeof(char*));
	char **new;
	size_t i, e = 0;
	t (!rc);
	while (i < len) {
		rc[++e] = buf + i;
		i += strlen(buf + i);
	}
	rc[e] = NULL;
	new = realloc(rc, (e + 1) * sizeof(char*));
	if (n)
		*n = e;
	return new ? new : rc;
fail:
	return NULL;
}


/**
 * Create `NULL`-terminate subcopy of an list,
 * 
 * @param   list  The list.
 * @param   n     The number of elements in the new sublist.
 * @return        The sublist, `NULL` on error.
 * 
 * @throws  Any exception specified for malloc(3).
 */
char **
sublist(char *const *list, size_t n)
{
	char **rc = malloc((n + 1) * sizeof(char*));
	t (!rc);
	rc[n] = NULL;
	while (n--)
		rc[n] = list[n];
	return rc;
fail:
	return NULL;
}

