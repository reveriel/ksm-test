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
unsigned const N_blob = 8;

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

// the write cursor
// .. not exposed
struct WriteP {
	struct MList *cur_ml;
	unsigned cur_page;
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
	// used only in write_pages()
	// init when first ml added
	struct WriteP wp;
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

	l->p = (char *) ROUND_UP(l->p_malloc, PAGESIZE);

	mlock(l->p, l->size); // prevent swap
	l->next = NULL;
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
		bk->tail = ml;
		bk->wp.cur_ml = ml;
		bk->wp.cur_page = 0;
		return;
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
	return bk;
}

static void BookKeeper_free(struct BookKeeper *bk) {
	free(bk);
}

// move 'wp' to point to the next page.
static void wrap_wp(struct BookKeeper *bk, struct WriteP *wp)
{
	wp->cur_page++;
	// if at end
	if (wp->cur_ml == bk->tail
			&& wp->cur_page == bk->allocated_size % N_blob) {
			wp->cur_ml = bk->head;
			wp->cur_page = 0;
	} else if (wp->cur_page == N_blob) {
		wp->cur_ml = wp->cur_ml->next;
		wp->cur_page = 0;
	}
}

// write one page at wp,
// move wp one page next, warp.
// @return how many pages has write.
// note: bk's size and capacity!
static int write_one_page_wp(struct BookKeeper *bk, struct WriteP *wp,
		char content)
{
	if (wp->cur_page >= N_blob)
		return 0;
	struct MList *ml = wp->cur_ml;
	char *write_p = ml->p;

	write_p += wp->cur_page * PAGESIZE;
	memset(write_p, content, PAGESIZE);
	wrap_wp(bk, wp);
	return 1;
}


// write one page at wp, write random data
static int write_one_page_wp_random(struct BookKeeper *bk, struct WriteP *wp)
{
	if (wp->cur_page >= N_blob)
		return 0;
	struct MList *ml = wp->cur_ml;
	char *write_p = ml->p;

	write_p += wp->cur_page * PAGESIZE;

	int *write_int_p = (int *)write_p;
	// there a 1024 ints, We don't need to write all
	// if write all,  pages' value space is 2^32768
	// a subsapce is enough to make pages have different contents
	for (int i = 0; i < PAGESIZE / 4; i += 16)
		write_int_p[i] = rand();
	
	wrap_wp(bk, wp);
	return 1;
}


// if wp->cur_ml is null, init to bk->head;
static void write_pages_wp(struct BookKeeper *bk, struct WriteP *wp, unsigned num,
		char content)
{
	if (!wp)
		return;
	if (wp->cur_ml == NULL) {
		wp->cur_ml = bk->head;
		wp->cur_page = 0;
	}

	for (unsigned i = 0; i < num; i++) {
		int n = write_one_page_wp(bk, wp, content);
		if (n == 0) {
			printf("write page error\n");
			return;
		}
	}
}

// if wp->cur_ml is null, init to bk->head;
static void write_pages_wp_random(struct BookKeeper *bk, struct WriteP *wp,
		unsigned num)
{
	if (!wp)
		return;
	if (wp->cur_ml == NULL) {
		wp->cur_ml = bk->head;
		wp->cur_page = 0;
	}

	for (unsigned i = 0; i < num; i++) {
		int n = write_one_page_wp_random(bk, wp);
		if (n == 0) {
			printf("write page error\n");
			return;
		}
	}
}

// check if the wp is a valid wp.
/* static int valid_wp(struct BookKeeper *bk, struct WriteP *wp); */

// only allocated
int alloc_pages(struct BookKeeper *bk, unsigned num)
{
	int num_pages_new = bk->allocated_size + num - bk->allocated_size_real;
	if (num_pages_new > 0) {
		// need allocate
		int num_ml_new = ROUND_UP(num_pages_new, N_blob) / N_blob;
		for (int i = 0; i < num_ml_new; i++) {
			struct MList *ml = MList_init();
			if (!ml)
				return -1;
			bk_add_ml(bk, ml);
		}
		bk->allocated_size_real += num_ml_new * N_blob;
	}
	bk->allocated_size += num;
	return 0;
}

void reset_write_pos(struct BookKeeper *bk)
{
	bk->wp.cur_ml = bk->head;
	bk->wp.cur_page = 0;
}

/* then I realized.. my poor design */
/* I will split the cursor out */
int alloc_pages_write(struct BookKeeper *bk, unsigned num, char content)
{
	struct WriteP wp;
	// I should have used an empty node as a list head!
	wp.cur_ml = bk->tail; // might be null
	wp.cur_page = bk->allocated_size % N_blob;

	alloc_pages(bk, num);
	write_pages_wp(bk, &wp, num, content);

	return 0;
}

unsigned num_allocated_pages(struct BookKeeper *bk)
{
	return bk->allocated_size;
}

// continue from where left last time
// if pages are freed, and the wp points to invalid ml, error.
//  add a wp to bk. used for this function.
// note! size!
void write_pages(struct BookKeeper *bk, unsigned num, char content)
{
	write_pages_wp(bk, &bk->wp, num, content);
}


// alloc pages, like 'alloc_pages_write()', but write random data
void alloc_pages_write_random(struct BookKeeper *bk, unsigned num)
{
	struct WriteP wp;
	wp.cur_ml = bk->tail; // might be null
	wp.cur_page = bk->allocated_size % N_blob;

	alloc_pages(bk, num);
	write_pages_wp_random(bk, &wp, num);
}

// not implemented yet
void write_random_pages(struct BookKeeper *bk, unsigned num, char content)
{
	bk = bk;
	num = num;
	content = content;
}

// similar to write_pages, but write random data
void write_pages_random(struct BookKeeper *bk, unsigned num)
{
	write_pages_wp_random(bk, &bk->wp, num);
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

// how many ml needs to be freed
static unsigned num_ml_needs_free(struct BookKeeper *bk, unsigned num) {
	// assume num >= size % N_blob
	// so return value must > 0
	unsigned size = bk->allocated_size;
	return (num - size % N_blob) / N_blob + 1;
}

// free, start from tail.
// I want double linked list !
// Ok, fine. not a problem..
void free_pages(struct BookKeeper *bk, unsigned num)
{
	if (!bk)
		return;
	// no need to free
	if (num < bk->allocated_size % N_blob) {
		bk->allocated_size -= num;
		return;
	}

	// how many ml needs to be freed
	unsigned num_free = num_ml_needs_free(bk, num);
	unsigned num_total = bk->allocated_size_real / N_blob;
	if (num_free > num_total)
		num_free = num_total;
	/* if total = 1, free = 1 */
	/* if total = 2, free = 1 */

	struct MList *ml = bk->head;
	if (num_total == num_free) {
		// that means we will free all mls
		bk->tail = NULL;
		bk->head = NULL;
	} else {
		// before the first to be freed, we need to reset tail pointer.
		for (int i = 0; i < (int)(num_total - num_free) - 1; i++) {
			ml = ml->next;
		}
		bk->tail = ml;
		ml = ml->next;
		bk->tail->next = NULL;
	}

	for (unsigned i = 0; i < num_free; i++) {
		struct MList *tmp = ml;
		MList_free(tmp);
		ml = ml->next;
	}

	bk->allocated_size -= num;
	bk->allocated_size_real -= num_free * N_blob;
}

void print_pages(struct BookKeeper *bk)
{
	if (!bk || !bk->head || !bk->head->p)
		return;
	struct MList *ml = bk->head;
	char *read_p = ml->p;
	for (unsigned i = 0; i < bk->allocated_size; i++) {
		printf(" %4x ", *read_p);
		read_p += PAGESIZE;
		if (i % N_blob == 0 && ml->next){
			ml = ml->next;
			read_p = ml->p;
			printf("\n");
		}
	}
}



