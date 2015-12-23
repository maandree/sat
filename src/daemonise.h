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


/**
 * Leave all opened files open.
 */
#define DAEMONISE_NO_CLOSE  1

/**
 * Leave all signal handlers rather than
 * appling default signal handlers.
 */
#define DAEMONISE_NO_SIG_DFL  2

/**
 * Leave the signal block mask as-is.
 */
#define DAEMONISE_KEEP_SIGMASK  4

/**
 * Do not remove malformatted environment entries.
 */
#define DAEMONISE_KEEP_ENVIRON  8

/**
 * Do not set umask to zero.
 */
#define DAEMONISE_KEEP_UMASK  16

/**
 * Do not create a PID file.
 */
#define DAEMONISE_NO_PID_FILE  32

/**
 * Do not close stderr even if it is
 * a terminal device.
 * 
 * Cannot be combined with `DAEMONISE_KEEP_STDERR`.
 */
#define DAEMONISE_KEEP_STDERR  64

/**
 * Close stderr even if it is
 * not a terminal device.
 * 
 * Cannot be combined with `DAEMONISE_KEEP_STDERR`.
 */
#define DAEMONISE_CLOSE_STDERR  128

/**
 * Do not close stdin.
 */
#define DAEMONISE_KEEP_STDIN  256

/**
 * Do not close stdout.
 */
#define DAEMONISE_KEEP_STDOUT  512



/**
 * Daemonise the process. This means to:
 * 
 * -  close all file descritors except for those to
 *    stdin, stdout, and stderr,
 * 
 * -  remove all custom signal handlers, and apply
 *    the default handlers.
 * 
 * -  unblock all signals,
 * 
 * -  remove all malformatted entries in the
 *    environment (this not containing an '=',)
 * 
 * -  set the umask to zero, to be ensure that all
 *    file permissions are set as specified,
 * 
 * -  change directory to '/', to ensure that the
 *    process does not block any mountpoint from being
 *    unmounted,
 * 
 * -  fork to become a background process,
 * 
 * -  temporarily become a session leader to ensure
 *    that the process does not have a controlling
 *    terminal.
 * 
 * -  fork again to become a child of the daemon
 *    supervisor, (subreaper could however be in the
 *    say, so one should not merely rely on this when
 *    writing a daemon supervisor,) (the first child
 *    shall exit after this,)
 * 
 * -  create, exclusively, a PID file to stop the daemon
 *    to be being run twice concurrently, and to let
 *    the daemon supervicer know the process ID of the
 *    daemon,
 * 
 * -  redirect stdin and stdout to /dev/null,
 *    as well as stderr if it is currently directed
 *    to a terminal, and
 * 
 * -  exit in the original process to let the daemon
 *    supervisor know that the daemon has been
 *    initialised.
 * 
 * Before calling this function, you should remove any
 * environment variable that could negatively impact
 * the runtime of the process.
 * 
 * After calling this function, you should remove
 * unnecessary privileges.
 * 
 * Do not try do continue the process in failure unless
 * you make sure to only do this in the original process.
 * But not that things will not necessarily be as when
 * you make the function call. The process can have become
 * partially deamonised.
 * 
 * If $XDG_RUNTIME_DIR is set and is not empty, its value
 * should be used instead of /run for the runtime data-files
 * directory, in which the PID file is stored.
 * 
 * @etymology  (Daemonise) the process!
 * 
 * @param   name   The name of the daemon. Use a hardcoded value,
 *                 not the process name. Must not be `NULL`.
 * @param   flags  Flags to modify the behaviour of the function.
 *                 A bitwise OR combination of the constants:
 *                 -  `DAEMONISE_NO_CLOSE`
 *                 -  `DAEMONISE_NO_SIG_DFL`
 *                 -  `DAEMONISE_KEEP_SIGMASK`
 *                 -  `DAEMONISE_KEEP_ENVIRON`
 *                 -  `DAEMONISE_KEEP_UMASK`
 *                 -  `DAEMONISE_NO_PID_FILE`
 *                 -  `DAEMONISE_KEEP_STDERR`
 *                 -  `DAEMONISE_CLOSE_STDERR`
 *                 -  `DAEMONISE_KEEP_STDIN`
 *                 -  `DAEMONISE_KEEP_STDOUT`
 * @return         Zero on success, -1 on error.
 * 
 * @throws  EEXIST  The PID file already exists on the system.
 *                  Unless your daemon supervisor removs old
 *                  PID files, this could mean that the daemon
 *                  has exited without removing the PID file.
 * @throws          Any error specified for signal(3).
 * @throws          Any error specified for sigemptyset(3).
 * @throws          Any error specified for sigprocmask(3).
 * @throws          Any error specified for chdir(3).
 * @throws          Any error specified for pipe(3).
 * @throws          Any error specified for dup2(3).
 * @throws          Any error specified for fork(3).
 * @throws          Any error specified for setsid(3).
 * @throws          Any error specified for open(3).
 * 
 * @since  Always.
 */
int daemonise(const char* name, int flags);

