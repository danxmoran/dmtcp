
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

int
main(int argc, char *argv[]) {
	// char* cgroup_dir = "/sys/fs/cgroup/memory/somename";

	// DIR* dir = opendir(cgroup_dir);
	// if (dir)
	// {
	// 	closedir(dir);
	// } else {
	// 	int mkdir_result = mkdir(cgroup_dir, 0777);
	// 	if (mkdir_result != 0) {
	// 		perror("failed to makedir\n");
	// 		return 0;
	// 	}
	// }


	// pid_t new_pid = fork();
	// if (new_pid == 0) {
		pid_t my_pid = getpid();
		char* stuff = "sfdasfdafdsafdsafds";
		printf("I'm a new process!\n");
		for (int i = 0; i > -1; i++) {
			void* mem = malloc(strlen(stuff) + 1);
			memcpy(mem, stuff, strlen(stuff) + 1);
			printf("%i: Makin' memory %p on %i\n", i, mem, my_pid);
			// sleep(1);
		}
		// for (int i = 0; i > -1; i++) {
		//   printf("%i\n", i);
		//   sleep(2);
		// }
		return 0;
	// } else {
	// 	// mount("/sys/fs/cgroup", , "cgroup")
	// 	char* filename = "/tasks";
	// 	char* cgroup_filename = malloc(strlen(cgroup_dir) + strlen(filename) + 1);
	// 	strcpy(cgroup_filename, cgroup_dir);
	// 	strcat(cgroup_filename, filename);
	// 	printf("File name %s\n", cgroup_filename);
	// 	FILE* f = fopen(cgroup_filename, "w+");
	// 	if (f == NULL) {
	// 		perror("failed to open\n");
	// 		return 0;
	// 	}

	// 	char* stuff = "fsfds";
	// 	int write_result = fwrite(&new_pid, sizeof(pid_t), 1, f);
	// 	if (write_result != sizeof(pid_t)) {
	// 		perror("failed to write");
	// 		return 0;
	// 	}
	// 	printf("Wrote %i\n", write_result);
	// }
}
