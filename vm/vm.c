/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "threads/mmu.h"
#include "userprog/process.h"				// project3 추가

#define swap_out(page) (page)->operations->swap_out (page)		// project3 추가 - vm_evict_frame 구현 시

struct list frame_table;			// project3 - frame 변수 선언

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* 
	TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT);

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* 
		TODO: Create the page, fetch the initialier according to the VM type,
		TODO: and then create "uninit" page struct by calling uninit_new. You
		TODO: should modify the field after calling the uninit_new. */
		// project3 - Anonymous Page 추가
		bool (*initializer)(struct page *, enum vm_type, void *);
		switch(VM_TYPE(type)){
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
		}

		struct page *new_page = malloc(sizeof(struct page));
		uninit_new (new_page, upage, init, type, aux, initializer);

		new_page->writable = writable;
		// new_page->mapped_page_count = -1;			// only for file-mapped pages
		
		/*
		TODO: Insert the page into the spt. */

		spt_insert_page(spt, new_page);		// should always return true - checked that upage is not in stp
			return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {			// project3 추가 구현 - 주어진 보충 페이지 테이블에서 va에 해당하는 구조체 페이지를 찾는다. 실패하면 NULL 리턴
	// struct page *page = NULL;
	
	/* 
	TODO: Fill this function. 
	*/
	struct page* page = (struct page*)malloc(sizeof(struct page));						// 'struct page'의 구조체인 'page'를 동적으로 할당. 이 구조체는 페이지 정보를 저장하는 용도로 사용
	struct hash_elem *e;

	// va에 해당하는 hash_elem 찾기
	page->va = pg_round_down(va);	// page의 시작 주소 할당 / va가 가리키는 가상 페이지의 시작 포인트(오프셋이 0으로 설정된 va) 반환: va의 가장 하위 비트를 버림 처리 함

	/* e와 같은 해시값(va)을 가지는 원소를 e에 해당하는 bucket list 내에서
	찾아 리턴한다. 만약 못 찾으면 NULL을 리턴한다. */
	e = hash_find(&spt->pages, &page->hash_elem);
	free(page);														// 페이지를 찾은 후 동적으로 할당한 'page' 포인터 해제

	// 있으면 e에 해당하는 페이지 반환
	return e!=NULL ? hash_entry(e, struct page, hash_elem) : NULL;				// 'hash_elem'을 가진 페이지 구조체를 찾아 반환
}

/* Insert PAGE into spt with validation. */
bool																			// supplemental_page_table인 'spt'에 주어진 페이지 'page'를 삽입함 - 성공하면 True, 실패하면 False 반환
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* 
	TODO: Fill this function. */
	// project3 - Anonymous Page
	struct hash_elem *e = hash_find(&spt->pages, &page->hash_elem);				// hash_find 함수를 사용하여 'spt'에서 주어진 페이지 'page'의 'hash_elem'을 찾음 - 해당 페이지가 spt에 존재하는지 확인
	if(e != NULL)		// page already in SPT									// hash_find 함수의 결과인 e가 NULL이 아니라면, 이미 페이지가 spt에 존재하므로 false를 반환하여 삽입 실패한 것을 알림
		return succ;		// false, fail
	// page not in SPT
	return hash_insert(&spt->pages, page);										// page가 spt에 없는 경우, hash_insert 함수를 사용하여 spt에 page 삽입 - hash_insert 함수는 해시 테이블에 요소를 삽입하고, 성공하면 NULL 반환
																// hash_insert 함수가 해시 테이블에 요소 삽입을 성공하면 NULL 반환, 리턴값이 NULL이 아니라면 false 반환. 삽입 성공시 true 값 반환
	// return hash_insert(&spt->spt_hash, &page->hash_elem) == NULL ? true : false;		// 존재하지 않을 경우에만 삽입
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	/* 
	TODO: The policy for eviction is up to you. */
	// project3 추가 구현
	// victim = list_entry(list_pop_front(&frame_table), struct frame, elem);		// FIFO algorithm

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* 
	TODO: swap out the victim and return the evicted frame. */
	if (victim->page != NULL) {
		swap_out(victim->page);
	}
	// 주석 추가 - Manipulate swap table according to its design
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	void *kva = palloc_get_page(PAL_USER);			// PAL_USER를 사용하는 이유 - 커널 풀 대신 사용자 풀에서 메모리를 할당하게 하기 위해서
	/* 
	TODO: Fill this function. */

	//project3 추가
	if (kva == NULL) {			// NULL이면 (사용 가능한 페이지가 없으면)
		// frame = vm_evict_frame();			// 페이지 삭제 후 frame 리턴
		PANIC("to do");
	}
	else {					// 사용 가능한 페이지가 있으면
		frame = malloc(sizeof(struct frame));		// 페이지 사이즈만큼 메모리 할당
		frame->kva = kva;
	}

	ASSERT (frame != NULL);
	// ASSERT (frame->page == NULL);			// project3에서 주석처리
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
        bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
    struct page *page = NULL;
    /* TODO: Validate the fault */
    /* TODO: Your code goes here
    TODO: 오류를 검증합니다. /
    TODO: 여기에 코드를 추가하세요. */
    if (addr == NULL)
            return false;

    if (is_kernel_vaddr(addr))
        return false;

    if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
    {
        /* TODO: Validate the fault */
        page = spt_find_page(spt, addr);

        if (page == NULL)
            return false;
        if (write == 1 && page->writable == 0) // write 불가능한 페이지에 write 요청한 경우
            return false;
		// printf("21232232222222222222222");				// 218줄에서 page를 못 가져올수도 있고 바로 위 if문에서 - 자식의 spt hash에 page를 못 넣어준다.
        return vm_do_claim_page(page);
    }
    return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	// struct page *page = NULL;
	/* 
	TODO: Fill this function */
	// project3 추가 구현
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = spt_find_page(spt, va);
	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* 
	TODO: Insert page table entry to map page's VA to frame's PA. */
	// project3 추가 구현
	struct thread *cur = thread_current();
	bool writable = page->writable;		// [vm.h] struct page에 bool writable; 추가
	pml4_set_page(cur->pml4, page->va, frame->kva, writable);

	// project3 주석 추가 - add the mapping from the virtual address to the physical address in the page table.
	bool res = swap_in (page, frame->kva);

	return res;
	// return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {

	// ✅ TEST: supplemental_page_table_init
	// bool initialize_hash = hash_init(&spt->pages, page_hash, page_less, NULL);
	// ASSERT(initialize_hash != true);	// initialize_hash가 true라면 프로그램을 종료시킨다.

	hash_init(&spt->pages, page_hash, page_less, NULL);				// project3 - 추가

}

/* Copy supplemental page table from src to dst */
// project3 - Supplemental Page Table - Revisit : src에서 dst로 추가 페이지 테이블을 복사한다. 이것은 자식이 부모의 실행 컨텍스트를 상속해야 할 때 사용된다. 예) fork
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {								// project3 - Supplemental Page Table 구현 추가 - struct supplemental_page_table_copy
	// TODO: 보조 페이지 테이블을 src에서 dst로 복사합니다.
	// TODO: src의 각 페이지를 순회하고 dst에 해당 entry의 사본을 만듭니다.
	// TODO: uninit page를 할당하고 그것을 즉시 claim해야 합니다.
	struct hash_iterator i;

	hash_first(&i, &src->pages);
	while (hash_next(&i))
	{
		
		// src_page 정보
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type type = src_page->operations->type;										// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		void *upage = src_page->va;
		bool writable = src_page->writable;
		/* 1) type이 uninit이면 */
		if (type == VM_UNINIT)
		{ // uninit page 생성 & 초기화
			vm_initializer *init = src_page->uninit.init;
			void *aux = src_page->uninit.aux;
			vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);

			continue;
		}

		/* 2) type이 file이면 */
		// if (type == VM_FILE)
		// {
		// 	struct lazy_load_info *file_aux = malloc(sizeof(struct lazy_load_info));
		// 	file_aux->file = src_page->file.file;
		// 	file_aux->offset = src_page->file.ofs;
		// 	file_aux->page_read_bytes = src_page->file.read_bytes;
		// 	file_aux->page_zero_bytes = src_page->file.zero_bytes;
		// 	if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, file_aux))
		// 		return false;
		// 	struct page *file_page = spt_find_page(dst, upage);
		// 	file_backed_initializer(file_page, type, NULL);
		// 	file_page->frame = src_page->frame;
		// 	pml4_set_page(thread_current()->pml4, file_page->va, src_page->frame->kva, src_page->writable);
		// 	continue;
		// }

		/* 3) type이 anon이면 */
		if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
			return false;						   // init이랑 aux는 Lazy Loading에 필요. 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음

		// vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
		if (!vm_claim_page(upage))
			return false;

		// 매핑된 프레임에 내용 로딩
		struct page *dst_page = spt_find_page(dst, upage);
		memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
	}
	return true;
}

void hash_page_destroy(struct hash_elem *e, void *aux)
{
    struct page *page = hash_entry(e, struct page, hash_elem);
    destroy(page);
    free(page);
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/*
	TODO: Destroy all the supplemental_page_table hold by thread and
	TODO: writeback all the modified contents to the storage. */

	hash_clear(&spt->pages, hash_page_destroy); // 해시 테이블에서 모든 요소를 제거
}


/********************* project3 - 추가 구현 *********************/
// 해시 테이블 초기화할 때 해시 값을 구해주는 함수의 포인터 (page_hash)
unsigned 
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
	const struct page *p = hash_entry(p_, struct page, hash_elem);

	return hash_bytes(&p->va, sizeof p->va);
}

// 해시 테이블 초기화할 때 해시 요소를 비교하는 함수의 포인터 (page_less)
// a가 b보다 작으면 true, 반대면 false
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED) {
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}

bool
insert_page(struct hash *pages, struct page *p) {
	if (!hash_insert(pages, &p->hash_elem))
		return true;
	else
		return false;
}

bool
delete_page(struct hash *pages, struct page *p) {
	if (!hash_delete(pages, &p->hash_elem))
		return true;
	else
		return false;
}
/****************************** 끝 *****************************/
