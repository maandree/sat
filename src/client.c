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
#include "client.h"
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/wait.h>



/**
 * The name of the process.
 */
extern char *argv0;



/**
 * Send a command to satd. Start satd if it is not running.
 * 
 * @param   cmd  Command type.
 * @param   n    The length of the message, 0 if
 *               `msg` is `NULL` or NUL-terminated.
 * @param   msg  The message to send.
 * @return       0 on success, -1 on error.
 * 
 * @throws  0  Error at the daemon-side.
 */
int
send_command(enum command cmd, size_t n, const char *restrict msg)
{
	struct sockaddr_un address;
	int fd = -1, start = 1, status, outfd, goterr = 0;
	char *dir;
	pid_t pid;
	ssize_t r, wrote;
	char *buf = NULL;
	signed char cmd_ = (signed char)cmd;
	int saved_errno;

	/* Get socket address. */
	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	if (strlen(dir) + sizeof("/satd.socket") > sizeof(address.sun_path))
		t ((errno = ENAMETOOLONG));
	stpcpy(stpcpy(address.sun_path, dir), "/satd.socket");
	address.sun_family = AF_UNIX;

	/* Any daemon listening? */
	fd = open(address.sun_path, O_RDONLY);
	if (fd == -1) {
		t (errno != ENOENT);
	} else {
		if (flock(fd, LOCK_SH | LOCK_NB) == -1)
			t (start = 0, errno != EWOULDBLOCK);
		else
			flock(fd, LOCK_UN);
		close(fd), fd = -1;
	}

	/* Start daemon if not running. */
	if (start) {
		switch ((pid = fork())) {
		case -1:
			goto fail;
		case 0:
			execl(BINDIR "/satd", BINDIR "/satd", NULL);
			perror(argv0);
			exit(1);
		default:
			t (waitpid(pid, &status, 0) != pid);
			t (errno = 0, status);
			break;
		}
	}

	/* Create socket. */
	t ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1);
	t (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1);

	/* Send message. */
	t (write(fd, &cmd_, sizeof(cmd_)) < (ssize_t)sizeof(cmd_));
	while (n) {
		r = write(fd, msg, n);
		t (r <= 0);
		msg += (size_t)r;
		n -= (size_t)r;
	}
	t (shutdown(fd, SHUT_WR)); /* Very important. */

	/* Receive. */
receive_again:
	t (r = read(fd, &cmd_, sizeof(cmd_)), r < (ssize_t)sizeof(cmd_));
	if (r == 0)
		goto done;
	outfd = (int)cmd_;
	goterr |= outfd == STDERR_FILENO;
	t (r = read(fd, &n, sizeof(n)), r < (ssize_t)sizeof(n));
	t (!(buf = malloc(n)));
	while (n) {
		t (r = read(fd, buf, n), r < 0);
		t (errno = 0, r == 0);
		n -= (size_t)r;
		for (dir = buf; r;) {
			t (wrote = write(outfd, dir, r), wrote <= 0);
			dir += (size_t)wrote;
			r -= (size_t)wrote;
		}
	}
	free(buf), buf = NULL;
	goto receive_again;

done:
	shutdown(fd, SHUT_RD);
	close(fd);
	errno = 0;
	return -goterr;

fail:
	saved_errno = (goterr ? 0 : errno);
	if (fd >= 0)
		close(fd);
	free(buf);
	errno = saved_errno;
	return -1;
}


/**
 * Return the number of bytes required to store a string array.
 * 
 * @param   array  The string array.
 * @return         The number of bytes required to store the array.
 */
size_t
measure_array(char *array[])
{
	size_t rc = 0;
	for (; *array; array++)
		rc += strlen(*array) + 1;
	return rc * sizeof(char);
}


/**
 * Store a string array.
 * 
 * @param   storage  The buffer where the array is to be stored.
 * @param   array    The array to store.
 * @return           Where in the buffer the array ends.
 */
char *
store_array(char *restrict storage, char *array[])
{
	for (; *array; array++) {
		storage = stpcpy(storage, *array);
		*storage++ = 0;
	}
	return storage;
}

