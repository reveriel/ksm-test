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
	// the pointer used to write data
	// this one is page aligned
	char *p; 
	// this pointer is returned from malloc
	// used to free memory
	char *p_malloc;
	int size; // N_blob * PAGESIZE
	struct MList *next;
};

struct BookKeeper {
	// the sum of 'num' of all alloc_page() invocation
	unsigned allocated_size;
	// allocated_size_real = ROUND_UP(allocated_size, N_blob);
	unsigned allocated_size_real;
	// the singlly linked ml list' head
	struct MList *head;
	// used in bk_add_ml(), make insert easier
	struct MList *tail;
	// write cursor,
	// record the write position
	// when write to Memory list, do it in loop
	// after the first bk_add_ml, cur_ml will not be null
	struct MList *cur_ml;
	// assert(cur_page >= 0 && cur_page < N_blob)
	unsigned cur_page;
};


/* allocate the memory blob */
/* make pointer alligned to page */
static struct MList *MList_init() {
	struct MList *l = (struct MList *)malloc(sizeof(struct MList));
	if (l == NULL)
		return NULL;
	l->size = PAGESIZE * N_blob;

	l->p_malloc = (char *)malloc(l->size + PAGESIZE);
	if (l->p_malloc == NULL) {
		free(l);
		return NULL;
	}

	l->p = ROUND_UP(l->p_malloc, PAGESIZE);

	mlock(l->p, l->size); // prevent swap
	l->next = 0;
	return l;
}

static void MList_free(struct MList *ml) {
	free(ml->p_malloc);
	free(ml);
}

// note, the order!
// to make write_pages work correctly
// new 'ml' should be appened at the tail;
static void bk_add_ml(struct BookKeeper *bk, struct MList *ml) {
	if (ml == NULL)
		return;
	// if it is the first ml added, init cur_ml and head
	if (bk->head == NULL) {
		bk->head = ml;
		bk->cur_ml = ml;
		bk->cur_page = 0;
	}
	bk->tail->next = ml;
	bk->tail = ml;
}

struct BookKeeper *BookKeeper_init() {
	struct BookKeeper *bk = (struct BookKeeper *)malloc(sizeof(struct BookKeeper));
	if (bk == NULL)
		return NULL;
	bk->head = NULL;
	bk->allocated_size = 0;
	bk->allocated_size_real = 0;
	bk->tail = NULL;
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

static write_one_page(struct BookKeeper *bk, char content)
{
	struct MList *ml = bk->cur_ml;
	char *write_p = ml->p;
	
	write_p += bk->cur_page * PAGESIZE;
	memset(write_p, content, PAGESIZE);
	
	// how many pages is allocated in this ml
	unsigned left_free = bk->allocated_size_real - bk->allocated_size;
	left_free = (left_free + N_blob - 1) % N_blob + 1;
	// if tail, wrap back
	if (ml == bk->tail && left_free == bk->cur_page) {
		bk->cur_ml = bk->head;
		bk->cur_page = 0;
	} else if (left_free == bk->cur_page) {
		bk->cur_ml = ml->next;
		bk->cur_page = 0;
	} else {
		bk->cur_page++;
	}
}


// need a cursor in bk as a write header
void write_pages(struct BookKeeper *bk, unsigned num, char content)
{
	for (int i = 0; i < num; i++)
		write_one_page(bk, content);
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
	struct MList *next = bk->head;
	while (next) {
		MList_free(next);
		next = next->next;
	}
	BookKeeper_free(bk);
}


void free_pages(struct BookKeeper *bk, unsigned num)
{
	bk = bk;
	num = num;
}



