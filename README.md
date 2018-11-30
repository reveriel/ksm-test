# ksm-test
native programs for ksm testing

## page allocation

use `malloc` to allocate memory. It's difficult to allocate page-alligned memory.
So, just allocate a large enough memory is OK.

Note: small alloc will be handeld by `sbrk`, big alloc will be handeld by `mmap`

use `mlockall` to prevent page from swapping. (Did I enabled swap in the kernel ? Anyway.)


## mem test programs.

a program that can
* do simple allcate, free, write do a page, read a page.
* do it either in a random way or mechanically
  * can simulate a real app and do simple test.

* basic memory operation
  * allocate
  * free
  * write
  * read
see details in `bk.h`

**.. Damn, make things unnecessarily complex**

### test_one : `test_one.c`
one program, allocated many pages.
Then all the same page should be merged very soon.
**Expect to see**
* pages sharing go up.
* examine the merge process with gdb.


## make

Then I realized, that I don't know how to write a Makefile that works with
upper level Makefiles...

Then..
1. how to get upper CC, CFLAGS definition?

2. how to make upper make include this lower one?

3. can this makefile still be used standalone?


## note

malloc allocate pages. new pages will get to new list.
when free.  what happens ? 
pages are not returned to the OS. but keeped in malloc's buffer.
No! it is returned to OS and freed. since pages are deleted from new list.

fuck, How to test the function kernel!




