# ksm-test
native programs for ksm testing

## page allocation

use `malloc` to allocate memory. It's difficult to allocate page-alligned memory.
So, just allocate a large enough memory is OK.

use `mlockall` to prevent page from swapping. (Did I enabled swap in the kernel ? Anyway.)


## mem test programs.

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

