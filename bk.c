#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "bk.h"

#define PAGESIZE 4096
struct MList {
	char *p;
	int size;
	struct MList *next;
};

struct BookKeeper {
	struct MList *next;
};

static struct MList *MList_init(char *pmem, int size) {
	struct MList *l = (struct MList *)malloc(sizeof(struct MList));
	if (l == NULL)
		return NULL;
	l->p = pmem;
	l->size = size;
	l->next = 0;
	return l;
}

static void MList_free(struct MList *ml) {
	free(ml->p);
	free(ml);
}

static void bk_add_mem(struct BookKeeper *bk, struct MList *ml) {
	ml->next = bk->next;
	bk->next = ml;
}

struct BookKeeper *BookKeeper_init() {
	struct BookKeeper *bk = (struct BookKeeper *)malloc(sizeof(struct BookKeeper));
	if (bk == NULL)
		return NULL;
	bk->next = NULL;
	return bk;
}

static void BookKeeper_free(struct BookKeeper *bk) {
	free(bk);
}

int alloc_pages(struct BookKeeper *bk, unsigned num, char content)
{
	int size = PAGESIZE * (num + 1);
	char *p = (char *)malloc(size);
	if (p == NULL)
		return errno;
	mlock(p, size);
	memset(p, content, size);
	struct MList *ml = MList_init(p, size);
	if (!ml)
		return -1;
	bk_add_mem(bk, ml);
	return 0;
}

void free_all_pages(struct BookKeeper *bk)
{
	struct MList *next = bk->next;
	while (next) {
		MList_free(next);
		next = next->next;
	}
	BookKeeper_free(bk);
}


