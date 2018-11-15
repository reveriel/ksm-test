#include <stdio.h>
#include <unistd.h>

/**
 * allocate many zero pages. with malloc.
 */

#include "bk.h"



int main()
{
	struct BookKeeper *bk;
	bk = BookKeeper_init();
	int err = alloc_pages(bk, 10, 0);
	if (!err) {
		sleep(10);
		free_all_pages(bk);
	} else {
		fprintf(stderr, "alloc_failed\n");
	}
	return 0;
}


