
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
#include <sys/stat.h>
#include <dirent.h>

char* cgroup_dir = "/sys/fs/cgroup/memory/somename";

void
write_mem_limit() {
	int fd = open()
}

int
main(int argc, char *argv[]) {
	DIR* dir = opendir(cgroup_dir);
	if (dir)
	{
		closedir(dir);
	} else {
		int mkdir_result = mkdir(cgroup_dir, 0777);
		if (mkdir_result != 0) {
			perror("failed to makedir\n");
			return 0;
		}
	}



	char* parts[] = {"cgexec", "-g", "memory:somename", "./make_cgroup", NULL};
	printf("About to exec\n");
	execvp("cgexec", parts);
}
