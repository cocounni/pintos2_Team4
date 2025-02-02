/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	// project3 - Anonymous Page 추가 구현 (struct ~ , void *aux ~)
	struct uninit_page *uninit = &page->uninit;
	void *aux = uninit->aux;

	/* Set up the handler */
	page->operations = &file_ops;
	memset(uninit, 0, sizeof(struct uninit_page));		// project3 - Anonymous Page

	struct lazy_load_info *lazy_load_info = (struct lazy_load_info *)aux;			// project3 - Anonymous Page
	struct file_page *file_page = &page->file;

	/* project3 - Anonymous Page 추가 구현 */
	file_page->file = lazy_load_info->file;
	file_page->read_bytes = lazy_load_info->page_read_bytes;
	file_page->ofs = lazy_load_info->offset;
	file_page->zero_bytes = lazy_load_info->page_zero_bytes;
	return true;
	/****************** 끝 *****************/
}

/* Project3 - Supplemental Page Table 추가 구현 : lazy_load_segment_for_file */
bool
lazy_load_segment_for_file(struct page *page, void *aux)
{
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
	struct lazy_load_info * lazy_load_info = (struct lazy_load_info *)aux;
	struct file * file = lazy_load_info->file;
	size_t page_read_bytes = lazy_load_info->page_read_bytes;
	size_t page_zero_bytes = lazy_load_info->page_zero_bytes;
	off_t offset = lazy_load_info->offset;

	file_seek(file, offset);

	ASSERT (page->frame != NULL);
	void * kva = page->frame->kva;
	if (file_read(file, kva, page_read_bytes) != (int)page_read_bytes)
	{
		free(lazy_load_info);
		return false;
	}

	memset(kva + page_read_bytes, 0, page_zero_bytes);
	free(lazy_load_info);

	file_seek(file, offset);			// may read the file later - reset fileobj pos

	return true;
}


/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* Do the munmap */
void
do_munmap (void *addr) {
}
