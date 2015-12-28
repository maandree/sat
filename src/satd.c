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
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "daemonise.h"
#include "common.h"



/**
 * The size of the backlog on the socket.
 */
#ifndef SATD_BACKLOG
# define SATD_BACKLOG  5
#endif



COMMAND("satd")
USAGE("[-f]")



/**
 * Create the socket.
 * 
 * @param   address  Output parameter for the socket address.
 * @return           The file descriptor for the socket, -1 on error.
 */
static int
create_socket(struct sockaddr_un *address)
{
	int fd = -1;
	ssize_t len;
	char *dir;
	int saved_errno;

	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	t (snprintf(NULL, 0, "%s/satd.socket%zn", dir, &len) == -1);
	if ((len < 0) || ((size_t)len >= sizeof(address->sun_path)))
		t ((errno = ENAMETOOLONG));
	sprintf(address->sun_path, "%s/satd.socket", dir);
	address->sun_family = AF_UNIX;
	/* TODO test flock */
	unlink(address->sun_path);
	t ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1);
	t (fchmod(fd, S_IRWXU) == -1);
	t (fchown(fd, getuid(), getgid()) == -1);
	t (bind(fd, (struct sockaddr *)address, sizeof(*address)) == -1);
	/* TODO flock */

	return fd;
fail:
	saved_errno = errno;
	if (fd >= 0)
		close(fd);
	errno = saved_errno;
	return -1;
}


/**
 * The sat daemon.
 * 
 * @param   argc  Any value in [0, 2] is accepted.
 * @param   argv  The name of the process, and -f if the process
 *                shall not be daemonised.
 * @return  0     The process was successful.
 * @return  1     The process failed queuing the job.
 * @return  2     User error, you do not know what you are doing.
 */
int
main(int argc, char *argv[])
{
	struct sockaddr_un address;
	int sock = -1, foreground = 0;

	if (argc > 0)  argv0 = argv[0];
	if (argc > 2)  usage();
	if (argc == 2)
		if (!(foreground = !strcmp(argv[1], "-f")))
			usage();

	t (foreground ? 0 : daemonise("satd", 0));

	t (sock = create_socket(&address), sock == -1);
	if (sock != 3) {
		t (dup2(sock, 3) == -1);
		close(sock), sock = 3;
	}

#ifdef SOMAXCONN < SATD_BACKLOG
	t (listen(sock, SOMAXCONN));
#else
	t (listen(sock, SATD_BACKLOG));
#endif

	close(sock);
	unlink(address.sun_path);
	undaemonise();
	return 0;

fail:
	perror(argv0);
	if (sock >= 0) {
		close(sock);
		unlink(address.sun_path);
	}
	undaemonise();
	return 1;
}

