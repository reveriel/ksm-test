#ifndef _BK_H
#define _BK_H

#define ROUND_UP(num, mod) ((((unsigned long long)num) + (mod) - 1) / (mod) * (mod))

extern unsigned const N_blob;

struct BookKeeper;

struct BookKeeper *BookKeeper_init();

/* allocate 'num' of pages. pad with 'content'
 * the real alloc is done in multiple of 'N_blob'('N')
 * for example, if 'num' = N + 1, we will allocate  2 * 'N'
 * number of ten-pages.
 * follow that, if you allocate N - 1, we don't need to do any real allocation,
 * since we already have 2 * 'N' free pages.
 *
 * return -1 if failed.
 * return 0 if success
 * */
int alloc_pages_write(struct BookKeeper *bk, unsigned num, char content);

/*
 * alloc pages, like 'alloc_pages_write()', but don't write 
 */
int alloc_pages(struct BookKeeper *bk, unsigned num);

/**
 * alloc pages, like 'alloc_pages_write()', but write random data
 */
void alloc_pages_write_random(struct BookKeeper *bk, unsigned num);

/* return the total number of allocated pages.
 * its is the sum of 'num' of all alloc_page() invocation
 */
unsigned num_allocated_pages(struct BookKeeper *bk);

/*
 * return the real number of pages allocated
 */
inline unsigned num_allocated_pages_real(struct BookKeeper *bk) {
	return ROUND_UP(num_allocated_pages(bk), N_blob);
}

/*
 * write pages
 * witie 'num' pages,
 * start from where stopped the last time.
 * think of pages as a round buffer.
 *
 * pad with char 'content'
 */
void write_pages(struct BookKeeper *bk, unsigned num, char content);

/* write 'num' pages,
 * random choosen
 * pad with char 'content'
 */
void write_random_pages(struct BookKeeper *bk, unsigned num, char content);

/* write 'num' pages,
 * start from where stopped the last time.
 * think of all pages in 'bk' as a round buffer
 *
 * write random data
 */
void write_pages_random(struct BookKeeper *bk, unsigned num);


/* not sure
 * yet
 */
void read_pages(struct BookKeeper *bk, unsigned num);


/*
 * simmilar to alloc_pages
 * free is done in multiple of 'N_blob'('N'),
 * if the the 'num_allocated_pages_real()' is N.
 * free 'num' < N, won't actually free any memory.
 */
void free_pages(struct BookKeeper *bk, unsigned num);

/*
 * free all memory allocated
 * and free 'bk'
 */
void free_all_pages(struct BookKeeper *bk);



#endif

