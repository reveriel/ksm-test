#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/* get, free, write pages
 */
#include "bk.h"

void print_help_message(char *progname)
{
	printf("Usage: \n");
	printf("     %s  [-s <num>][-h]\n", progname);
	printf("\n");
	printf("   -h         : print this help message\n");
	printf("   -s <num>   : choose which simulation to run\n");
	printf("      <num>\n");
	printf("      1 : get 100, 1; get 100, 0; sleep 10; free;\n");
	printf("      2 : get 10000, sleep 30;\n");
	printf("      3 : get 100, 1; sleep 10; fork{child: write 100, 2; sleep 10; parent: wait}; free;\n");
	printf("      4 : get 100, 1; sleep 10; fork 39 child;\n");
}

// I want to write a script language. to descripbe the simulation...

/* simulation 1 */
void simu1()
{
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages_write(bk, 100, 1);
	alloc_pages_write(bk, 100, 0);
	sleep(10);
	free_all_pages(bk);
}

// write page
// random write
void simu2()
{
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages_write(bk, 10000, 1);
	sleep(30);
	free_all_pages(bk);
}

// fork()
void simu3()
{
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages_write(bk, 100, 1);
	sleep(10);

	pid_t pid = fork();
	int r;
	if (pid == 0) {
		write_pages(bk, 100, 2);
		sleep(10);
	} else {
		write_pages(bk, 100, 3);
		wait(&r);
	}

	free_all_pages(bk);
}

// fork()
void simu4() {
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages_write(bk, 100, 1);
	sleep(10);

	int nproc = 10;

	pid_t pid;
	for (int i = 1; i < nproc; i++) {
		pid = fork();
		if (pid == 0) break;
		sleep(10);

	}
	if (pid == 0) {
		for (int i = 1; i < nproc; i++) {
			int r;
			wait(&r);
		}
	}

	free_all_pages(bk);
}


int main(int argc, char *argv[])
{
	int simu = 0;

	if (argc == 1 || strncmp(argv[1], "-h", 2) == 0) {
		print_help_message(argv[0]);
		exit(0);
	}

	char *a = argv[1];
	if (a[0] == '-' && a[1] == 's') {
		simu = atoi(argv[2]);
	}

	switch (simu) {
		case 1: simu1();
			break;
		case 2: simu2();
			break;
		case 3: simu3();
			break;
		case 4: simu4();
			break;
		default:
			print_help_message(argv[0]);
			exit(0);
	}

	return 0;
}


