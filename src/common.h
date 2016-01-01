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
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/timerfd.h>



/**
 * Go to `fail` if a statement evaluates to non-zero.
 * 
 * @param  ...  The statement.
 */
#ifndef t
# ifndef DEBUG
#  define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)
# else
#  define t(...)  do { if ((__VA_ARGS__) ? (failed__ = #__VA_ARGS__) : 0) { (perror)(failed__); goto fail; } } while (0)
static const char *failed__ = NULL;
#  define perror(_)  ((void)(_))
# endif
#endif


/**
 * Perform some actions without changing `errno`.
 * 
 * You must have `int saved_errno` declared.
 * 
 * @param   ...  The actions.
 * @return       The value on `errno`.
 */
#define S(...)  (saved_errno = errno, __VA_ARGS__, errno = saved_errno)


/**
 * The file descriptor for the state file.
 */
#define STATE_FILENO  3

/**
 * The file descriptor for the CLOCK_BOOTTIME timer.
 */
#define BOOT_FILENO  4

/**
 * The file descriptor for the CLOCK_REALTIME timer.
 */
#define REAL_FILENO  5

/**
 * The file descriptor for the lock file.
 */
#define LOCK_FILENO  6



/**
 * A queued job.
 */
struct job {
	/**
	 * The job number.
	 */
	size_t no;

	/**
	 * The number of “argv” elements in `payload`.
	 */
	int argc;

	/**
	 * The clock in which `ts` is measured.
	 */
	clockid_t clk;

	/**
	 * The time when the job shall be executed.
	 */
	struct timespec ts;

	/**
	 * The number of bytes in `payload`.
	 */
	size_t n;

	/**
	 * “argv” followed by “envp”.
	 */
	char payload[];
};



/**
 * `dup2(OLD, NEW)` and, on success, `close(OLD)`.
 * 
 * @param   OLD:int  The file descriptor to duplicate and close.
 * @param   NEW:int  The new file descriptor.
 * @return           0 on success, -1 on error.
 */
#define DUP2_AND_CLOSE(OLD, NEW)  (dup2(OLD, NEW) == -1 ? -1 : (close(OLD), 0))

/**
 * Call `CALL` which returns a file descriptor.
 * Than make sure that the file descriptor's
 * number is `WANT`. Go to `fail' on error.
 * 
 * @param  FD:int variable  The variable where the file descriptor shall be stored.
 * @param  WANT:int         The file descriptor the file should have.
 * @parma  CALL:int call    Call to function that creates and returns the file descriptor.
 */
#define GET_FD(FD, WANT, CALL)              \
	t (FD = CALL, FD == -1);            \
	t (dup2_and_null(FD, WANT) == -1);  \
	FD = WANT


/**
 * Defines the function `void usage(void)`
 * that prints usage information.
 * 
 * @param  synopsis:const char*  The command's synopsis sans command name.
 *                               `NULL` if there are not arguments.
 */
#define USAGE(synopsis)  \
static void  \
usage(void)  \
{  \
	fprintf(stderr, "usage: %s%s%s\n",  \
	        strrchr(argv0, '/') ? (strrchr(argv0, '/') + 1) : argv0,  \
	        synopsis ? " " : "", synopsis ? synopsis : "");  \
	exit(2);  \
}

/**
 * Declares `argv0` and its value to
 * a specified string.
 * 
 * @param  name:sitrng literal  The name of the command.
 */
#define COMMAND(name)  \
const char *argv0 = name;


/**
 * Print usage and exit if there is any argument
 * that is an option.
 */
#define NO_OPTIONS  \
do {  \
	int i;  \
	if (!strcmp(argv[1], "--"))  \
		argv++, argc--;  \
	for (i = 1; i < argc; i++)  \
		if (strchr("-", argv[i][0]))  \
			usage();  \
} while (0)

/**
 * Macro to put directly after the variable definitions in `main`.
 */
#define PROLOGUE(USAGE_ASSUMPTION, ACCESS)  \
	int state = -1;                     \
	if (argc > 0)  argv0 = argv[0];     \
	if (!(USAGE_ASSUMPTION))  usage();  \
	GET_FD(state, STATE_FILENO, open_state(ACCESS, NULL))

/**
 * Macro to put before the cleanup code in `main`.
 */
#define CLEANUP_START                       \
	errno = 0;                          \
fail:                                       \
	if (errno)       perror(argv[0]);   \
	if (state >= 0)  close(state)

/**
 * Macro to put after the cleanup code in `main`.
 */
#define CLEANUP_END  \
	return !!errno



/**
 * Wrapper for `pread` that reads the required amount of data.
 * 
 * @param   fildes  See pread(3).
 * @param   buf     See pread(3).
 * @param   nbyte   See pread(3).
 * @param   offset  See pread(3).
 * @return          See pread(3), only short if the file is shorter.
 */
ssize_t preadn(int fildes, void *buf, size_t nbyte, size_t offset);

/**
 * Wrapper for `pwrite` that writes all specified data.
 * 
 * @param   fildes  See pwrite(3).
 * @param   buf     See pwrite(3).
 * @param   nbyte   See pwrite(3).
 * @param   offset  See pwrite(3).
 * @return          See pwrite(3).
 */
ssize_t pwriten(int fildes, const void *buf, size_t nbyte, size_t offset);

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
char **restore_array(char *buf, size_t len, size_t *n);

/**
 * Create `NULL`-terminate subcopy of an list,
 * 
 * @param   list  The list.
 * @param   n     The number of elements in the new sublist.
 * @return        The sublist, `NULL` on error.
 * 
 * @throws  Any exception specified for malloc(3).
 */
char **sublist(char *const *list, size_t n);

/**
 * Create a new open file descriptor for an already
 * existing file descriptor.
 * 
 * @param   fd     The file descriptor that shall be promoted
 *                 to a new open file descriptor.
 * @param   oflag  See open(3), `O_CREAT` is not allowed.
 * @return         0 on success, -1 on error.
 */
int reopen(int fd, int oflag);

/**
 * Run a job or a hook.
 * 
 * @param   job   The job.
 * @param   hook  The hook, `NULL` to run the job.
 * @return        0 on success, -1 on error, 1 if the child failed.
 */
int run_job_or_hook(struct job *job, const char *hook);

/**
 * Removes (and optionally runs) a job.
 * 
 * @param   jobno   The job number, `NULL` for any job.
 * @param   runjob  Shall we run the job too? 2 if its time has expired (not forced).
 * @return          0 on success, -1 on error.
 * 
 * @throws  0  The job is not in the queue.
 */
int remove_job(const char *jobno, int runjob);

/**
 * Get a `NULL`-terminated list of all queued jobs.
 * 
 * @return  A `NULL`-terminated list of all queued jobs. `NULL` on error.
 */
struct job **get_jobs(void);

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
int dup2_and_null(int old, int new);

/**
 * Create or open the state file.
 * 
 * @param   open_flags  Flags (the second parameter) for `open`.
 * @param   state_path  Output parameter for the state file's pathname.
 *                      May be `NULL`;
 * @return              A file descriptor to the state file, -1 on error.
 * 
 * @throws  0  `!(open_flags & O_CREAT)` and the file does not exist.
 */
int open_state(int open_flags, char **state_path);

/**
 * Let the daemon know that it may need to
 * update the timers, and perhaps exit.
 * 
 * @param   start  Start the daemon if it is not running?
 * @param   name   The name of the process.
 * @return         0 on success, -1 on error.
 */
int poke_daemon(int start, const char *name);

/**
 * Set SAT_HOOK_PATH.
 * 
 * @return  0 on success, -1 on error.
 */
int set_hookpath(void);



/**
 * This block of code allows us to compile with DEBUG=valgrind
 * or DEBUG=strace and have all exec:s be wrapped in
 * valgrind --leak-check=full --show-leak-kinds=all or
 * strace, respectively. Very useful for debugging. However,
 * children do not inherit strace, so between forking and
 * exec:ing we do not have strace.
 */
#if 1 && defined(DEBUG)
# if ((DEBUG == 2) || (DEBUG == 3))
#  ifdef __GNUC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
__attribute__((__used__))
#  endif
#  if DEBUG == 2
#   define DEBUGPROG  "strace"
#  else
#   define DEBUGPROG  "valgrind"
#  endif
#  define execl(path, _, ...) (execl)("/usr/bin/" DEBUGPROG, "/usr/bin/" DEBUGPROG, path, __VA_ARGS__)
#  define execve(...) execve_(__VA_ARGS__)
static int
execve_(const char *path, char *const argv[], char *const envp[])
{
	size_t n = 0;
	char **new_argv = NULL;
	int x = (DEBUG - 2) * 2, saved_errno;
	while (argv[n++]);
	t (!(new_argv = malloc((n + 1 + (size_t)x) * sizeof(char *))));
	new_argv[x] = "--show-leak-kinds=all";
	new_argv[1] = "--leak-check=full";
	new_argv[0] = "/usr/bin/" DEBUGPROG;
	new_argv[1 + x] = path;
	memcpy(new_argv + 2 + x, argv + 1, (n - 1) * sizeof(char *));
	(execve)(*new_argv, new_argv, envp);
fail:
	return saved_errno = errno, free(new_argv), errno = saved_errno, -1;
}
#  ifdef __GNUC__
#   pragma GCC diagnostic pop
#  endif
# endif
#endif

