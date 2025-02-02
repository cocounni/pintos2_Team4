/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

const int SECTORS_IN_PAGE = 8;			// 4KB / 512 (DISK_SECTOR_SIZE)

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	/*********** project3 - Anonymous Page 추가 구현 ***********/
	swap_disk = disk_get(1, 1);

#ifdef DBG
	printf("disk size : %d\n", disk_size(swap_disk));
#endif
	bitcnt = disk_size(swap_disk)/SECTORS_IN_PAGE;		// #ifdef Q. disk size decided by swap-size option?
	swap_table = bitmap_create(bitcnt);		// each bit = swap slot for a frame
}
	/************************** 끝 *************************/

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	// project3 - Anonymous Page 추가 구현 (struct ~ , memset ~)
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));

	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	// project3 - Anonymous Page 추가 구현
	anon_page->swap_slot_no = -1;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
