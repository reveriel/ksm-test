#include <stdio.h>

#include "bk.c"

#define P_GREEN(FMT, ...) printf("\033[32;1m" FMT  "\033[0m\n", ##__VA_ARGS__)
#define P_RED(FMT, ...) printf("\033[31;1m" FMT "\033[0m\n", ##__VA_ARGS__)

#define EXPECT_INT(A, B) do { \
	if ((A) == (B)) { \
		P_GREEN("PASS"); \
	} else { \
		P_RED("FAIL"); \
		printf("Expression: " #A "\n"); \
		printf("Expect: " " %d" "\n", (B)); \
		printf("Get:    " " %d" "\n", (A)); \
		return; \
	} } while (0)

#define EXPECT_PTR(A, B) do { \
	if ((A) == (B)) { \
		P_GREEN("PASS"); \
	} else { \
		P_RED("FAIL"); \
		printf("Expression: " #A "\n"); \
		printf("Expect: " " %p" "\n", (B)); \
		printf("Get:    " " %p" "\n", (A)); \
		return; \
	} } while (0)

// take 'bk' as an array of char.
// compare with 'ans' array of length 'len'
// return 0 if equal
// in 'ans', is an element is '(char)-1', it can match anything
int cmp_bk_array(struct BookKeeper *bk, char *ans, unsigned len) {
	if (!bk)
		return 1;
	unsigned size = bk->allocated_size;
	if (size != len)
		return 1;

	struct MList *ml = bk->head;
	char *p = ml->p;
	for (unsigned i = 0; i < size; i++) {
		if (*p != ans[i] && ans[i] != (char)-1)
			return 1;
		p += PAGESIZE;
		if (p == ml->p + PAGESIZE * N_blob 
				&& i != size - 1) { // bugfix
			ml = ml->next;
			p = ml->p;
		}
	}
	return 0;
}

void MList_print(struct MList *ml) {
	printf(" [");
	char *p = ml->p;
	for (unsigned i = 0; i < N_blob; i++) {
		printf("%4x ", *p);
		p += PAGESIZE;
	}
	printf(" ]\n");
}

void MList_init_test() {
	printf("Test: %s\n", __func__);
	struct MList *ml  = MList_init();
	MList_print(ml);
	MList_free(ml);
}


void BK_init_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	EXPECT_INT(bk->allocated_size, 0);
	EXPECT_INT(bk->allocated_size_real, 0);
	EXPECT_PTR(bk->head, NULL);
	EXPECT_PTR(bk->tail, NULL);
	BookKeeper_free(bk);
}

void BookKeeper_print(struct BookKeeper *bk) {
	struct MList *ml = bk->head;
	while (ml) {
		MList_print(ml);
		ml = ml->next;
	}
}

// consider size
void BookKeeper_prints(struct BookKeeper *bk) {
	unsigned size = bk->allocated_size;
	struct MList *ml = bk->head;
	while (size >= N_blob) {
		size -= N_blob;
		MList_print(ml);
		ml = ml->next;
	}
	if (size != 0) {
		printf(" [");
		char *p = ml->p;
		while (size--) {
			printf("%4x ", *p);
			p += PAGESIZE;
		}
		printf(" ]\n");
	}
}

void bk_add_ml_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	struct MList *ml  = MList_init();
	bk_add_ml(bk, ml);
	EXPECT_PTR(bk->head, ml);
	EXPECT_PTR(bk->tail, ml);
	EXPECT_PTR(ml->next, NULL);
	BookKeeper_print(bk);
	BookKeeper_free(bk);
	MList_free(ml);
}

void bk_add_ml_test2() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	struct MList *ml1  = MList_init();
	struct MList *ml2  = MList_init();
	struct MList *ml3  = MList_init();
	bk_add_ml(bk, ml1);
	bk_add_ml(bk, ml2);
	bk_add_ml(bk, ml3);
	BookKeeper_print(bk);
	EXPECT_PTR(bk->head, ml1);
	EXPECT_PTR(bk->head->next, ml2);
	EXPECT_PTR(bk->head->next->next, ml3);
	EXPECT_PTR(bk->tail, ml3);
	EXPECT_PTR(ml3->next, NULL);

	BookKeeper_free(bk);
	MList_free(ml1);
	MList_free(ml2);
}

void alloc_pages_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, 4);
	char ans[] = {-1,-1,-1,-1};
	EXPECT_INT(cmp_bk_array(bk, ans, 4), 0);
}
void alloc_pages_test2() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, N_blob + 4);
	char *ans = (char *)malloc(N_blob + 4);
	memset(ans, -1, N_blob + 4);
	EXPECT_INT(cmp_bk_array(bk, ans, N_blob + 4), 0);
	free(ans);
}

void write_one_page_wp_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, 2);

	char *ans = (char *)malloc(N_blob);
	memset(ans, -1, 2);
	ans[0] = 1;

	struct WriteP wp;
	wp.cur_ml = bk->head;
	wp.cur_page = 0;
	write_one_page_wp(bk, &wp, 1);
	EXPECT_PTR(wp.cur_ml, bk->head);
	EXPECT_INT(wp.cur_page, 1);
	EXPECT_INT(cmp_bk_array(bk, ans, 2), 0);

	BookKeeper_free(bk);
	free(ans);
}

void write_one_page_wp_test2() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, N_blob + 2);

	char *ans = (char *)malloc(N_blob + 2);
	memset(ans, -1, N_blob + 2);
	ans[N_blob + 1] = 1;
	ans[0] = 1;


	struct WriteP wp;
	wp.cur_ml = bk->tail;
	wp.cur_page = 1;
	write_one_page_wp(bk, &wp, 1);
	write_one_page_wp(bk, &wp, 1);
	EXPECT_PTR(wp.cur_ml, bk->head);
	EXPECT_INT(wp.cur_page, 1);
	EXPECT_INT(cmp_bk_array(bk, ans, N_blob + 2), 0);

	BookKeeper_free(bk);
	free(ans);
}

void write_pages_wp_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, N_blob + 2);

	char *ans = (char *)malloc(N_blob + 2);
	memset(ans, -1, N_blob + 2);
	ans[N_blob + 1] = 1;
	memset(ans, 1, N_blob);


	struct WriteP wp;
	wp.cur_ml = bk->tail;
	wp.cur_page = 1;
	write_pages_wp(bk, &wp, N_blob + 1, 1);
	EXPECT_PTR(wp.cur_ml, bk->tail);
	EXPECT_INT(wp.cur_page, 0);
	EXPECT_INT(cmp_bk_array(bk, ans, N_blob + 2), 0);

	BookKeeper_free(bk);
	free(ans);
}

void alloc_pages_write_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages_write(bk, 12, 5);
	alloc_pages_write(bk, 20, 6);

	char *ans = (char *)malloc(12 + 20);
	memset(ans, 5, 12);
	memset(ans + 12, 6, 20);

	EXPECT_INT(bk->allocated_size, 12 + 20);
	EXPECT_INT(bk->allocated_size_real, (int)ROUND_UP(32, N_blob));
	EXPECT_INT(cmp_bk_array(bk, ans, 12 + 20), 0);

	MList_free(bk->head);
	BookKeeper_free(bk);
}

void write_pages_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, N_blob * 2);
	write_pages(bk, N_blob, 1);
	write_pages(bk, N_blob, 2);
	write_pages(bk, N_blob, 3); // overwrite
	
	char *ans = (char*)malloc(N_blob * 2);
	memset(ans, 3, N_blob);
	memset(ans + N_blob, 2, N_blob);

	EXPECT_INT(cmp_bk_array(bk, ans, N_blob * 2), 0);

	MList_free(bk->head);
	BookKeeper_free(bk);
}


int main() {
	MList_init_test();
	BK_init_test();
	bk_add_ml_test();
	bk_add_ml_test2();
	alloc_pages_test();
	alloc_pages_test2();
	write_one_page_wp_test();
	write_one_page_wp_test2();
	write_pages_wp_test();
	alloc_pages_write_test();
	write_pages_test();
}

