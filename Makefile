
CFLAGS = -Wall -Wextra
all: ksm-test

ksm-test: bk.c bk.h test_one.c
	$(CC) $(CFLAGS) bk.c test_one.c -o test_one

