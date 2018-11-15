#pragma once
struct BookKeeper;

struct BookKeeper *BookKeeper_init();
int alloc_pages(struct BookKeeper *bk, unsigned num, char content);
void free_all_pages(struct BookKeeper *bk);

