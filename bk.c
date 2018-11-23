#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>

#include "bk.h"

/*
 *
 * alloc and free pages to simulate real situation
 * a test program for ksm
 * 
 * because we use malloc to allocate memory,
 * this is not pages aligned
 * and allocated memory has block_header stuff that saves meta data.
 * we have to use bigger granularity..
 * 
 * malloc aggregate many small malloc() into big brk() 
 * (or mmap() for larger request)
 * requests. to improve performance.
 * when free, pages are also not returned immediately
 *
 * see man mallopt(3)
 * 
 * allocation larger than M_MMAP_THRESHOLD(128*1024, dynamic)
 * are served by mmap
 *
 */ 

#define PAGESIZE 4096
int N_blob = 8;

/* this is a node that points to a memory blob 
 * which is of size 'N_blob * PAGESIZE'
 */
struct MList {
	char *p;
	int size; // N_blob * PAGESIZE
	struct MList *next;
};

struct BookKeeper {
	// the sum of 'num' of all alloc_page() invocation
	unsigned allocated_size;
	// allocated_size_real = ROUND_UP(allocated_size, N_blob);
	unsigned allocated_size_real;
	struct MList *next;
	// write cursor,
	// record the write position
	// when write to Memory list, do it in loop
	struct MList *cur_ml;
	// assert(cur_page >= 0 && cur_page < N_blob)
	unsigned cur_page;
};


/* allocate the memory blob */
static struct MList *MList_init() {
	struct MList *l = (struct MList *)malloc(sizeof(struct MList));
	if (l == NULL)
		return NULL;
	l->size = PAGESIZE * N_blob;
	l->p = (char *)malloc(l->size);
	if (l->p == NULL) {
		free(l);
		return NULL;
	}
	mlock(l->p, l->size); // prevent swap
	l->next = 0;
	return l;
}

static void MList_free(struct MList *ml) {
	free(ml->p);
	free(ml);
}

static void bk_add_ml(struct BookKeeper *bk, struct MList *ml) {
	ml->next = bk->next;
	bk->next = ml;
}

struct BookKeeper *BookKeeper_init() {
	struct BookKeeper *bk = (struct BookKeeper *)malloc(sizeof(struct BookKeeper));
	if (bk == NULL)
		return NULL;
	bk->next = NULL;
	bk->allocated_size = 0;
	bk->allocated_size_real = 0;
	bk->cur_ml = NULL;
	bk->cur_page = 0;
	return bk;
}

static void BookKeeper_free(struct BookKeeper *bk) {
	free(bk);
}


int alloc_pages(struct BookKeeper *bk, unsigned num, char content)
{
	bk->allocated_size += num;
	bool need_alloc = (bk->allocated_size > bk->allocated_size_real);
	if (!need_alloc) {
		// TODO: write content
		return 0;
	}
	int num_pages_needed = bk->allocated_size - bk->allocated_size_real;
	int num_pages_alloc_real = ROUND_UP(num_pages_needed, N_blob);
	bk->allocated_size_real +=  num_pages_alloc_real;

	int num_ml = num_pages_alloc_real / N_blob ;
	for (int i = 0; i < num_ml; i++) {
		struct MList *ml= MList_init();
		if (!ml)
			return -1;
		// TODO: write content
		bk_add_ml(bk, ml);
	}
	
	struct MList *ml = MList_init();
	if (!ml)
		return -1;
	return 0;
}

unsigned nr_allocated_pages(struct BookKeeper *bk)
{
	return bk->allocated_size;
}

// need a cursor in bk as a write header
void write_pages(struct BookKeeper *bk, unsigned num, char content)
{
	
	

}

void write_random_pages(struct BookKeeper *bk, unsigned num, char content)
{
	bk = bk;
	num = num;
	content = content;
}

void write_pages_random(struct BookKeeper *bk, unsigned num)
{
	bk = bk;
	num = num;
}






/* also free bk */
void free_all_pages(struct BookKeeper *bk)
{
	struct MList *next = bk->next;
	while (next) {
		MList_free(next);
		next = next->next;
	}
	BookKeeper_free(bk);
}


void free_pages(struct BookKeeper *bk, unsigned num)
{
	
}



