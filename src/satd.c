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
#include "common.h"
#include "daemon.h"
#include "daemonise.h"
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/timerfd.h>



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
	int fd = -1, bound = 0;
	const void *_cvoid;
	const char *dir;
	int saved_errno;

	/* Get socket address. */
	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	if (strlen(dir) + sizeof("/" PACKAGE "/socket") > sizeof(address->sun_path))
		t ((errno = ENAMETOOLONG));
	stpcpy(stpcpy(address->sun_path, dir), "/" PACKAGE "/socket");
	address->sun_family = AF_UNIX;

	/* Create socket. */
	unlink(address->sun_path);
	t ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1);
	t (fchmod(fd, S_IRWXU) == -1);
	t (bind(fd, (const struct sockaddr *)(_cvoid = address), (socklen_t)sizeof(*address)) == -1);
	/* EADDRINUSE just means that the file already exists, not that it is actually used. */
	bound = 1;

	return fd;
fail:
	saved_errno = errno;
	if (fd >= 0)
		close(fd);
	if (bound)
		unlink(address->sun_path);
	errno = saved_errno;
	return -1;
}


/**
 * Create the state file.
 * 
 * @param   state_path  Output parameter for the state file's pathname.
 * @return              A file descriptor to the state file, -1 on error.
 */
static int
create_state(char **state_path)
{
	const char *dir;
	char *path;
	int fd = -1, saved_errno;

	/* Create directory. */
	dir = getenv("XDG_RUNTIME_DIR"), dir = (dir ? dir : "/run");
	t (!(path = malloc(strlen(dir) * sizeof(char) + sizeof("/" PACKAGE "/state"))));
	stpcpy(stpcpy(path, dir), "/" PACKAGE "/state");
	t (fd = open(path, O_RDWR | O_CREAT /* but not O_EXCL or O_TRUNC */, S_IRUSR | S_IWUSR), fd == -1);
	*state_path = path, path = NULL;

fail:
	saved_errno = errno;
	free(path);
	errno = saved_errno;
	return fd;
}


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
	int fd = -1, saved_errno;

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
		t (fd = -1, errno != EWOULDBLOCK);
		fprintf(stderr, "%s: the daemon is already in reading.\n", argv0);
		errno = 0;
		goto fail;
	}

fail:
	saved_errno = errno;
	free(path);
	errno = saved_errno;
	return fd;
}


/**
 * Construct the pathname for the hook script.
 * 
 * @param   env     The environment variable to use for the beginning
 *                  of the pathname, `NULL` for the home directory.
 * @param   suffix  The rest of the pathname.
 * @return          The pathname.
 * 
 * @throws  0  The environment variable is not set, or, if `env` is
 *             `NULL` the user is root or homeless.
 */
static char *
hookpath(const char *env, const char *suffix)
{
	const char *prefix = NULL;
	char *path;
	struct passwd *pwd;

	if (!env) {
		if (!getuid())
			goto try_next;
		pwd = getpwuid(getuid());
		prefix = pwd ? pwd->pw_dir : NULL;
	} else {
		prefix = getenv(env);
	}
	if (!prefix || !*prefix)
		goto try_next;

	t (!(path = malloc((strlen(prefix) + strlen(suffix) + 1) * sizeof(char))));
	stpcpy(stpcpy(path, prefix), suffix);

	return path;
try_next:
	errno = 0;
fail:
	return NULL;
}


/**
 * Duplicate a file descriptor, and
 * open /dev/null to the old file descriptor.
 * However, if `old` is 3 or greater, it will
 * be closed rather than /dev/null.
 * 
 * @param   old  The old file descriptor.
 * @param   new  The new file descriptor.
 * @return       `new`, -1 on error.
 */
static int
dup2_and_null(int old, int new)
{
	int fd = -1;
	int saved_errno;

	if (old == new)  goto done;
	t (dup2(old, new) == -1);
	close(old);
	if (old >= 3)   goto done;
	t (fd = open("/dev/null", O_RDWR), fd == -1);
	if (fd == old)  goto done;
	t (dup2(fd, old) == -1);
	close(fd), fd = -1;

done:
	return new;
fail:
	saved_errno = errno;
	if (fd >= 0)
		close(fd);
	errno = saved_errno;
	return -1;
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
#define GET_FD(FD, WANT, CALL)              \
	t (FD = CALL, FD == -1);            \
	t (dup2_and_null(FD, WANT) == -1);  \
	FD = WANT
#define HOOKPATH(PRE, SUF)  \
	t (path = path ? path : hookpath(PRE, SUF), !path && errno)

	struct sockaddr_un address;
	int sock = -1, state = -1, boot = -1, real = -1, lock = -1, foreground = 0;
	char *path = NULL;
	struct itimerspec spec;

	/* Parse command line. */
	if (argc > 0)  argv0 = argv[0];
	if (argc > 2)  usage();
	if (argc == 2)
		if (!(foreground = !strcmp(argv[1], "-f")))
			usage();

	/* Get hook-script pathname. */
	if (!getenv("SAT_HOOK_PATH")) {
		HOOKPATH("XDG_CONFIG_HOME", "/sat/hook");
		HOOKPATH("HOME", "/.config/sat/hook");
		HOOKPATH(NULL, "/.config/sat/hook");
		t (setenv("SAT_HOOK_PATH", path ? path : "/etc/sat/hook", 1));
		free(path), path = NULL;
	}

	/* Open/create lock file and state file, and create socket. */
	GET_FD(lock,  LOCK_FILENO,  create_lock());
	GET_FD(state, STATE_FILENO, create_state(&path));
	GET_FD(sock,  SOCK_FILENO,  create_socket(&address));

	/* Create timers. */
	GET_FD(boot, BOOT_FILENO, timerfd_create(CLOCK_BOOTTIME, 0));
	GET_FD(real, REAL_FILENO, timerfd_create(CLOCK_REALTIME, 0));

	/* Configure timers. */
	memset(&spec, 0, sizeof(spec));
	t (timerfd_settime(boot, TFD_TIMER_ABSTIME, &spec, NULL));
	t (timerfd_settime(real, TFD_TIMER_ABSTIME, &spec, NULL));

	/* Listen for incoming conections. */
#if SOMAXCONN < SATD_BACKLOG
	t (listen(sock, SOMAXCONN));
#else
	t (listen(sock, SATD_BACKLOG));
#endif

	/* Daemonise. */
	t (foreground ? 0 : daemonise("satd", DAEMONISE_KEEP_FDS | DAEMONISE_NEW_PID, 3, 4, 5, 6, 7, -1));

	/* Change to a process image without all this initialisation text. */
	execl(LIBEXECDIR "/" PACKAGE "/satd-diminished", argv0, address.sun_path, path, NULL);

fail:
	if (errno)
		perror(argv0);
	free(path);
	if (sock >= 0) {
		unlink(address.sun_path);
		close(sock);
	}
	if (state >= 0)  close(state);
	if (boot  >= 0)  close(boot);
	if (real  >= 0)  close(real);
	if (lock  >= 0)  close(lock);
	undaemonise();
	return 1;
}

