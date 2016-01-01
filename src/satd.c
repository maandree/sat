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
#include "daemonise.h"



COMMAND("satd")
USAGE("[-f]")



/**
 * Create and lock the lock file, and its directory.
 * 
 * @return  A file descriptor to the lock file, -1 on error.
 */
static int
create_lock(void)
{
	const char *dir;
	char *path;
	char *p;
	pid_t pid = getpid();
	int fd = -1, saved_errno = 0;

	/* Create directory. */
	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	t (!(path = malloc(strlen(dir) * sizeof(char) + sizeof("/" PACKAGE "/lock"))));
	p = stpcpy(stpcpy(path, dir), "/" PACKAGE);
	t (mkdir(path, S_IRWXU) && (errno != EEXIST));

	/* Open file. */
	stpcpy(p, "/lock");
	t (fd = open(path, O_RDWR | O_CREAT /* but not O_EXCL or O_TRUNC */, S_IRUSR | S_IWUSR), fd == -1);

	/* Check that the daemon is not running, and mark it as running. */
	if (flock(fd, LOCK_EX | LOCK_NB)) {
		t (errno != EWOULDBLOCK);
		fprintf(stderr, "%s: the daemon is already in reading.\n", argv0);
		errno = 0;
		goto fail;
	}

	/* Store PID in the file. */
	/* Yes it is coming similar to a PID file, but this works if the started with -f. */
	t (pwrite(fd, &pid, sizeof(pid), (off_t)0) < (ssize_t)sizeof(pid));

	goto done;
fail:
	saved_errno = errno, close(fd), fd = -1;
done:
	free(path), errno = saved_errno;
	return fd;
}


/**
 * The sat daemon initialisation.
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
	int state = -1, boot = -1, real = -1, lock = -1, foreground = 0;
	char *path = NULL;
	struct itimerspec spec;

	/* Parse command line. */
	if (argc > 0)  argv0 = argv[0];
	if (argc > 2)  usage();
	if ((argc == 2) && (!(foreground = !strcmp(argv[1], "-f"))))
		usage();

	/* Get hook-script pathname. */
	t (set_hookpath());

	/* Open/create lock file and state file. */
	GET_FD(lock,  LOCK_FILENO,  create_lock());
	GET_FD(state, STATE_FILENO, open_state(O_RDWR | O_CREAT, &path));

	/* Create timers. */
	GET_FD(boot, BOOT_FILENO, timerfd_create(CLOCK_BOOTTIME, 0));
	GET_FD(real, REAL_FILENO, timerfd_create(CLOCK_REALTIME, 0));

	/* Configure timers. */
	memset(&spec, 0, sizeof(spec));
	t (timerfd_settime(boot, TFD_TIMER_ABSTIME, &spec, NULL));
	t (timerfd_settime(real, TFD_TIMER_ABSTIME, &spec, NULL));

	/* Daemonise. */
	t (foreground ? 0 : daemonise("satd", /*DAEMONISE_KEEP_FDS | DAEMONISE_NEW_PID,*/ 3, 4, 5, 6, -1));

	/* Change to a process image without all this initialisation text. */
	execl(LIBEXECDIR "/" PACKAGE "/satd-diminished", argv0, path, NULL);

fail:
	if (errno)
		perror(argv0);
	free(path);
	close(state), close(boot), close(real), close(lock);
	undaemonise();
	return 1;
}

