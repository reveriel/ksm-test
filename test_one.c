#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

/**
 * allocate many zero pages. with malloc.
 */

#include "bk.h"


void print_help_message(char *progname)
{
	printf("Usage: \n");
	printf("     %s  [-r][-h]\n", progname);
	printf("\n");
	printf("   -r : random\n");
	printf("   -h : print this help message\n");
}

/* simulation 1 */
void simu1()
{
	struct BookKeeper *bk;
	bk = BookKeeper_init();

	alloc_pages(bk, 50, 1);

	// ramdom free some pages 
	free_pages(bk, 50);
	alloc_pages(bk, 50, 1);

	sleep(10);
	free_all_pages(bk);
}

int main(int argc, char *argv[])
{
	bool op_random = false;
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			char *a = argv[i];
			if (a[0] == '-' && a[1] == 'r') {
				op_random = true;
			} else if (a[0] == '-' && a[1] == 'h') {

				print_help_message(argv[0]);
			} else  {
				print_help_message(argv[0]);
			}
		}
	}

	simu1();

	return 0;

}


