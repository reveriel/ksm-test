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

void write_one_page_wp_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	struct MList *ml1  = MList_init();
	bk_add_ml(bk, ml1);
	BookKeeper_print(bk);
	struct WriteP wp;
	wp.cur_ml = bk->head;
	wp.cur_page = 0;
	write_one_page_wp(bk, &wp, 1);
	EXPECT_PTR(wp.cur_ml, ml1);
	EXPECT_INT(wp.cur_page, 1);
	BookKeeper_print(bk);

	wp.cur_page = N_blob - 1;
	write_one_page_wp(bk, &wp, 1);
	EXPECT_PTR(wp.cur_ml, ml1);
	EXPECT_INT(wp.cur_page, 0);
	BookKeeper_print(bk);
	write_one_page_wp(bk, &wp, 1);
	write_one_page_wp(bk, &wp, 1);
	BookKeeper_print(bk);
	EXPECT_PTR(wp.cur_ml, ml1);
	EXPECT_INT(wp.cur_page, 2);
	
	BookKeeper_free(bk);
	MList_free(ml1);
}

void write_one_page_wp_test2() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	struct MList *ml1  = MList_init();
	struct MList *ml2  = MList_init();
	bk_add_ml(bk, ml1);
	bk_add_ml(bk, ml2);
	printf("before\n");
	BookKeeper_print(bk);
	struct WriteP wp;
	wp.cur_ml = bk->head;
	wp.cur_page = N_blob - 1;
	write_one_page_wp(bk, &wp, 2);
	write_one_page_wp(bk, &wp, 3);
	EXPECT_PTR(wp.cur_ml, ml2);
	EXPECT_INT(wp.cur_page, 1);
	printf("after\n");
	BookKeeper_print(bk);
	
	BookKeeper_free(bk);
	MList_free(ml1);
	MList_free(ml2);
}

void write_page_wp_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	struct MList *ml1  = MList_init();
	struct MList *ml2  = MList_init();
	bk_add_ml(bk, ml1);
	bk_add_ml(bk, ml2);
	printf("before\n");
	BookKeeper_print(bk);
	struct WriteP wp;
	wp.cur_ml = bk->head;
	wp.cur_page = 0;
	write_page_wp(bk, &wp, N_blob * 3, 4);
	printf("after\n");
	BookKeeper_print(bk);
	
	BookKeeper_free(bk);
	MList_free(ml1);
	MList_free(ml2);
}

void alloc_pages_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, 2, 5);
	printf("allocated\n");
	BookKeeper_print(bk);

	MList_free(bk->head);
	BookKeeper_free(bk);
}

void alloc_pages_test2() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, N_blob + 2, 6);
	printf("allocated\n");
	BookKeeper_print(bk);

	MList_free(bk->head);
	BookKeeper_free(bk);
}

void write_pages_test() {
	printf("Test: %s\n", __func__);
	struct BookKeeper *bk = BookKeeper_init();
	alloc_pages(bk, N_blob + 2, 0);
	/* write_pages(bk, N_blob + 3); */
}


int main() {
	MList_init_test();
	BK_init_test();
	bk_add_ml_test();
	bk_add_ml_test2();
	write_one_page_wp_test();
	write_one_page_wp_test2();
	write_page_wp_test();
	alloc_pages_test();
	alloc_pages_test2();
	write_pages_test();
}

