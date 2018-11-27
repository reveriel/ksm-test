
CFLAGS = -Wall -Wextra
all: test_one bk_test

test_one: bk.c bk.h test_one.c
	$(CC) $(CFLAGS) bk.c test_one.c -o test_one

bk_test: bk.c bk.h bk_test.c
	$(CC) $(CFLAGS) bk_test.c -o $@
