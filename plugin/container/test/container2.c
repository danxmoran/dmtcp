
#define _GNU_SOURCE
#include <sys/mount.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/* A simple error-handling function: print an error message based
   on the value in 'errno' and terminate the calling process */

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

struct child_args {
    char **argv;        /* Command to be executed by child, with args */
    int    pipe_fd[2];  /* Pipe used to synchronize parent and child */
};

static int verbose;

static void
usage(char *pname)
{
    fprintf(stderr, "Usage: %s [options] cmd [arg...]\n\n", pname);
    fprintf(stderr, "Create a child process that executes a shell "
            "command in a new user namespace,\n"
            "and possibly also other new namespace(s).\n\n");
    fprintf(stderr, "Options can be:\n\n");
#define fpe(str) fprintf(stderr, "    %s", str);
    fpe("-i          New IPC namespace\n");
    fpe("-m          New mount namespace\n");
    fpe("-n          New network namespace\n");
    fpe("-p          New PID namespace\n");
    fpe("-u          New UTS namespace\n");
    fpe("-U          New user namespace\n");
    fpe("-M uid_map  Specify UID map for user namespace\n");
    fpe("-G gid_map  Specify GID map for user namespace\n");
    fpe("-z          Map user's UID and GID to 0 in user namespace\n");
    fpe("            (equivalent to: -M '0 <uid> 1' -G '0 <gid> 1')\n");
    fpe("-v          Display verbose messages\n");
    fpe("\n");
    fpe("If -z, -M, or -G is specified, -U is required.\n");
    fpe("It is not permitted to specify both -z and either -M or -G.\n");
    fpe("\n");
    fpe("Map strings for -M and -G consist of records of the form:\n");
    fpe("\n");
    fpe("    ID-inside-ns   ID-outside-ns   len\n");
    fpe("\n");
    fpe("A map string can contain multiple records, separated"
        " by commas;\n");
    fpe("the commas are replaced by newlines before writing"
        " to map files.\n");

    exit(EXIT_FAILURE);
}

static int              /* Start function for cloned child */
childFunc(void *arg)
{
    char ch;

    /* Wait until the parent has updated the UID and GID mappings.
       See the comment in main(). We wait for end of file on a
       pipe that will be closed by the parent process once it has
       updated the mappings. */

    /* Execute a shell command */

    // if (umount("/proc") != 0) {
    //         fprintf(stderr, "failed unmount /proc %s\n",
    //                 strerror(errno));
    //         exit(-1);
    // }

    // if (mount("proc", "/proc", "proc", 0, "") != 0) {
    //         fprintf(stderr, "failed mount /proc %s\n",
    //                 strerror(errno));
    //         exit(-1);
    // }

    printf("PID: %i\n", getpid());
    // for (int i = 0; i > -1; i++) {
    //   printf("%i\n", i);
    //   sleep(2);
    // }

    char* args[] = { "bash" };
    execvp("bash", args);
    // errExit("execvp");
}

#define STACK_SIZE (1024 * 1024)

static char child_stack[STACK_SIZE];    /* Space for child's stack */

int
main(int argc, char *argv[])
{
    int flags, opt, map_zero;
    pid_t child_pid;
    struct child_args args;
    char *uid_map, *gid_map;
    const int MAP_BUF_SIZE = 100;
    char map_buf[MAP_BUF_SIZE];
    char map_path[PATH_MAX];

    /* Parse command-line options. The initial '+' character in
       the final getopt() argument prevents GNU-style permutation
       of command-line options. That's useful, since sometimes
       the 'command' to be executed by this program itself
       has command-line options. We don't want getopt() to treat
       those as options to this program. */

    flags = 0;
    verbose = 0;
    gid_map = NULL;
    uid_map = NULL;
    map_zero = 0;
    // while ((opt = getopt(argc, argv, "+imnpuUM:G:zv")) != -1) {
    //     switch (opt) {
    //     case 'i': flags |= CLONE_NEWIPC;        break;
    //     case 'm': flags |= CLONE_NEWNS;         break;
    //     case 'n': flags |= CLONE_NEWNET;        break;
    //     case 'p': flags |= CLONE_NEWPID;        break;
    //     case 'u': flags |= CLONE_NEWUTS;        break;
    //     case 'v': verbose = 1;                  break;
    //     case 'z': map_zero = 1;                 break;
    //     case 'U': flags |= CLONE_NEWUSER;       break;
    //     default:  usage(argv[0]);
    //     }
    // }

    /* Create the child in new namespace(s) */

    child_pid = clone(childFunc, child_stack + STACK_SIZE,
                      flags | CLONE_NEWPID | SIGCHLD, NULL);
    if (child_pid == -1)
        errExit("clone");

    /* Parent falls through to here */

    if (verbose)
        printf("%s: PID of child created by clone() is %ld\n",
                argv[0], (long) child_pid);

    if (verbose)
        printf("%s: terminating\n", argv[0]);

    exit(EXIT_SUCCESS);
}
