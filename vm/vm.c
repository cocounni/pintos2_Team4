/* vm.c: Generic interface for virtual memory objects. vm.c: 가상 메모리 객체에 대한 일반적인 인터페이스.*/

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "userprog/process.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes.
 각 서브시스템의 초기화 코드를 호출하여 가상 메모리 서브시스템을 초기화합니다 */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* 프로젝트 4를 위한 부분 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES.
	상단의 코드를 수정하지 마십시오. */
	/* TODO: Your code goes here.
	TODO: 여기에 코드를 추가하세요. */
	list_init(&frame_table);
	lock_init(&frame_table_lock);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. 
 * 페이지의 타입을 가져옵니다. 페이지가 초기화된 후에 타입을 알고 싶을 때 이 함수가 유용합니다.현재 구현된 상태입니다.*/
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

/* Helpers 헬퍼 함수*/
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
page, do not create it directly and make it through this function or
`vm_alloc_page`. 
초기화 함수를 사용하여 보류 중인 페이지 개체를 생성합니다. 페이지를 생성하려면
직접 생성하지 말고 이 함수나 'vm_alloc_page'를 통해 만드세요. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,vm_initializer *init, void *aux) {		// type: 가상 메모리의 유형, upage: 할당할 페이지의 user 가상 주소, writable: 페이지 읽기/쓰기 가능 여부,
																														// init: 페이지 초기화 함수에 대한 포인터, aux: 보조 데이터 포인터
	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;														// 현재 스레드와 연결된 spt(보충 페이지 테이블) 가져옴

	/* Check wheter the upage is already occupied or not. 
	upage가 이미 점유되었는지 확인합니다*/
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. 
		 TODO: 페이지를 생성하고, VM 유형에 따라 초기화를 가져와서 "uninit" 페이지 구조체를 호출하여 생성합니다.
         호출 후에 필드를 수정해야 합니다.*/
		//!
		int t = VM_TYPE(type);
		struct page *p=(struct page*)malloc(sizeof(struct page)); 
		bool (*initializer)(struct page *, enum vm_type, void *);
		switch (t){																	// type에 따라 적절한 초기화 함수 가져옴
		case VM_ANON:
			initializer = anon_initializer;
			break;
		case VM_FILE:
			initializer = file_backed_initializer;
			break;
		}
		uninit_new(p, upage, init, type, aux, initializer);							// uninit_new 함수롤 호출하여 새로 생성된 struct page, upage, init, type, aux, initializer를 전달함 / 제공된 초기화 함수로 페이지를 초기화 하는 역할
		p->writable =writable;
		//!
		/* TODO: Insert the page into the spt.TODO: 페이지를 spt에 삽입합니다. */
		return spt_insert_page(spt, p);												// spt_insert_page 함수롤 호출하여 페이지를 보충 페이지 테이블에 삽입
	}
err:
	return false;					// 페이지 삽입 성공하면 true, 페이지 할당이나 삽입 과정 중에 오류가 발생하면 err 레이블로 이동하여 false 반환
}

/* Find VA from spt and return page. On error, return NULL.
spt에서 VA를 찾아서 해당 페이지를 반환합니다. 오류 시 NULL을 반환합니다. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* 
	TODO: Fill this function. */
	page = (struct page *)malloc(sizeof(struct page));
	struct hash_elem *e;

	page -> va =pg_round_down(va);
	e = hash_find(&spt->spt_hash, &page -> hash_elem); //해시 테이블 &spt 내에서 page가 hash_elem인 요소를 찾아 반환
	free(page);

	//hash_find한 값이 NULL이면 NULL, 아니면 e를 포인터로 가진 페이지 반환
	return e != NULL ? hash_entry(e,struct page, hash_elem) : NULL;	
}

/* Insert PAGE into spt with validation.페이지를 spt에 삽입하고 유효성을 검사합니다 */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/*
	TODO: Fill this function. */
	// struct page *p = spt_find_page(spt, page->va); // 중복검사, spt에 추가하려는 va를 가진 page가 있는지 확인
	// if (p != NULL)
	// 	return false;							// 같은게 없지 않으면(있으면 False)

	// hash_insert(&spt, &page->hash_elem); 		// 추가하려는 데이터가 존재하지 않을 경우에만 삽입
	// return true;
	return hash_insert(&spt->spt_hash, &page->hash_elem) == NULL ? true :false;
}


void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->spt_hash, &page->hash_elem);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted.대상이 될 구조체 frame을 가져옵니다 */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you.
	TODO: 대상 선정 정책은 여러분에게 달려 있습니다. */
	struct thread *curr = thread_current();

	lock_acquire(&frame_table_lock);
	struct list_elem *start = list_begin(&frame_table);
	for (start; start != list_end(&frame_table); start = list_next(start))
	{
		victim = list_entry(start, struct frame, frame_elem);
		if (victim->page == NULL) // frame에 할당된 페이지가 없는 경우 (page가 destroy된 경우 )
		{
			lock_release(&frame_table_lock);
			return victim;
		}
		if (pml4_is_accessed(curr->pml4, victim->page->va))
			pml4_set_accessed(curr->pml4, victim->page->va, 0);
		else
		{
			lock_release(&frame_table_lock);
			return victim;
		}
	}
	lock_release(&frame_table_lock);
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.
 페이지를 하나 제거하고 해당하는 프레임을 반환합니다. 오류 시 NULL을 반환합니다.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. 
	TODO: 대상을 스왑아웃하고 반환된 프레임을 반환합니다.*/

	if (victim->page)
		swap_out(victim->page);
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.
palloc()과 프레임 가져오기. 사용 가능한 페이지가 없으면 페이지를 스왑아웃하여
사용 가능한 메모리 공간을 확보한 후 반환합니다.
항상 유효한 주소를 반환합니다. 즉, 사용자 풀 메모리가 가득 차 있으면
이 함수는 사용 가능한 메모리 공간을 확보하기 위해 프레임을 스왑아웃합니다.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* 
	TODO: Fill this function.
	*/
	void *kva= palloc_get_page(PAL_USER); // user pool에서 새로운 physical page를 가져온다.

	if (kva == NULL)		// page 할당 실패
	{
		struct frame *victim = vm_evict_frame();
		victim->page = NULL;
		return victim;
	}


	frame = (struct frame *) malloc (sizeof(struct frame));	//프레임 할당
	frame -> kva = kva;
	frame -> page =NULL;

	lock_acquire(&frame_table_lock);
	list_push_back(&frame_table, &frame->frame_elem);
	lock_release(&frame_table_lock);
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. 스택 확장*/
static void
vm_stack_growth (void *addr UNUSED) {			// project3 추가 구현
	// todo: 스택 크기를 증가시키기 위해 anon page를 하나 이상 할당하여 주어진 주소(addr)가 더 이상 예외 주소(faulted address)가 되지 않도록 한다.
	// todo: 할당할 때 addr을 PGSIZE로 내림하여 처리
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page 페이지에서 발생한 오류 처리*/
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success
성공 시 true를 반환합니다.  */
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
		// todo: 페이지 폴트가 스택 확장에 대한 유효한 경우인지를 확인해야 한다.

		/****************** project3. Stack Growth 추가 구현 끝 ******************/
		void *rsp = f->rsp;			// user access인 경우 rsp는 유저 stack을 가리킨다.
		if (!user)
			rsp = thread_current()->rsp;

		// 스택 확장으로 처리할 수 있는 폴트인 경우, vm_stack_growth를 호출
		if ((USER_STACK - (1 << 20) <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK) || (USER_STACK - (1 << 20) <= rsp && rsp <= addr && addr <= USER_STACK))
			vm_stack_growth(addr);
		/*********************** Stack Growth 추가 구현 끝 ***********************/

		page = spt_find_page(spt, addr);
		if (page == NULL)
			return false;
		if (write == 1 && page->writable == 0) // write 불가능한 페이지에 write 요청한 경우
			return false;
		return vm_do_claim_page(page);
	}
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. 
 페이지를 해제합니다.

이 함수를 수정하지 마십시오.*/
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA.
VA에 할당된 페이지를 차지합니다. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function
	TODO: 이 함수를 구현하세요. */
	page = spt_find_page(&thread_current() -> spt, va);		//spt에서 VA를 찾아서 해당 페이지를 반환합니다. 오류 시 NULL을 반환합니다.
	if (page ==NULL)
		return false;
	return vm_do_claim_page (page);
}

/** Claim the PAGE and set up the mmu.
 * 페이지를 차지하고 MMU를 설정합니다.  
 * @param page 할당할 페이지를 가리키는 포인터
 * */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();	//함수를 통해 얻은 프레임을 가리키는 포인터

	/* Set links 링크 설정  */
	frame->page = page;		//frame 구조체의 page에는 할당된 페이지를
	page->frame = frame;	//page 구조체의 frame에는 할당된 프레임을 설정

	/* TODO: Insert page table entry to map page's VA to frame's PA. 
	TODO: 페이지 테이블 엔트리를 삽입하여 페이지의 VA를 프레임의 PA로 매핑합니다.*/
	struct thread *current =thread_current();
	pml4_set_page(current->pml4, page->va, frame ->kva, page->writable);

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table
새로운 보조 페이지 테이블을 초기화합니다. */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {

	/* 보조 데이터 AUX를 사용하여 해시 테이블 H를 초기화하고
	해시 값을 계산하기 위해 HASH를 사용하고 해시 요소를 비교하기 위해 LESS를 사용합니다.
	*/
	hash_init(&spt->spt_hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst 
src에서 dst로 보조 페이지 테이블을 복사합니다.*/
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED) 
{																		// dst는 복사될 보조 페이지 테이블 / src는 복사할 보조 페이지 테이블 : src -> dst로 복사
	// TODO: 보조 페이지 테이블을 src에서 dst로 복사합니다.
	// TODO: src의 각 페이지를 순회하고 dst에 해당 entry의 사본을 만듭니다.
	// TODO: uninit page를 할당하고 그것을 즉시 claim해야 합니다.
	struct hash_iterator i;

	hash_first (&i, &src->spt_hash);
	while (hash_next (&i))
	{
		// src_page 정보
		struct page *src_page = hash_entry (hash_cur (&i), struct page, hash_elem);
		//...f를 사용하여 작업 수행...
		enum vm_type type = src_page->operations->type;					// type: 페이지의 가상 메모리 타입
		void *upage = src_page->va;										// upage: 페이지의 가상 주소
		bool writable = src_page->writable;								// writable: 페이지의 쓰기 가능 여부

		/* 1) type이 uninit이면 */
		if (type == VM_UNINIT)											// 페이지 타입이 VM_UNINIT이면 초기화되지 않은 페이지를 할당하고 초기화한다.
		{	// uninit page 생성 & 초기화
			vm_initializer *init = src_page -> uninit.init;
			void *aux =src_page ->uninit.aux;
			vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);
			continue;
		}
		
		/* 2) type이 file이면 */
		if (type == VM_FILE)
		{
			struct for_lazy *file_aux = malloc(sizeof(struct for_lazy));
			file_aux->file = src_page->file.file;
			file_aux->ofs = src_page->file.ofs;
			file_aux->read_bytes = src_page->file.read_bytes;
			file_aux->zero_bytes = src_page->file.zero_bytes;
			if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, file_aux))
				return false;
			struct page *file_page = spt_find_page(dst, upage);
			file_backed_initializer(file_page, type, NULL);
			file_page->frame = src_page->frame;
			pml4_set_page(thread_current()->pml4, file_page->va, src_page->frame->kva, src_page->writable);
			continue;
		}

		/* 3) type이 anon이면 */
		if (!vm_alloc_page(type, upage, writable))			// uninit page 생성 & 초기화
			return false;									// init이랑 aux는 Lazy Loading에 필요. 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음
		
		// vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
		if (!vm_claim_page(upage))
			return false;

		struct page *dst_page = spt_find_page(dst,upage);
		memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);		// 페이지를 요구한 후 src_page의 데이터를 dst_page로 복사함 이때 memcpy를 이용하여 페이지의 프레임(KVA)을 복사함0
	}
	return true;														// 모든 페이지 복사한 후 true 반환

}


void hash_page_destroy(struct hash_elem *e, void *aux)
{
    struct page *page = hash_entry(e, struct page, hash_elem);
    destroy(page);
    free(page);
}
/* Free the resource hold by the supplemental page table  
보조 페이지 테이블이 점유한 리소스를 해제합니다.  */


void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {				// struct supplemental_page_table 포인터인 spt를 매개변수로 받음. -> spt: 파괴할 보조 페이지 테이블을 나타냄
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage.
	 TODO: 스레드가 보유한 모든 보조 페이지 테이블을 제거하고
     TODO: 수정된 모든 내용을 스토리지에 기록하세요.
	 TODO: 페이지 항목들을 순회하며 테이블 내의 페이지들에 대해 destroy(page)를 호출  */
	 hash_clear(&spt->spt_hash, hash_page_destroy);				// 해시 테이블의 모든 요소를 제거

	 //!!!!!!!!!!!! hash_destroy가 아닌 hash_clear를 사용해야 하는 이유
	  /* 여기서 hash_destroy 함수를 사용하면 해시 테이블 자체가 소멸되어 hash가 사용하던 메모리(hash->bucket) 자체도 반환한다.
	  * process가 실행될 때 hash table을 생성한 이후에 process_clean()이 호출되는데,
	  * 이때는 hash table은 남겨두고 안의 요소들만 제거되어야 한다.
	  * 따라서, hash의 요소들만 제거하는 hash_clear를 사용해야 한다.
	 */
}
// 해시 테이블을 초기화할 때 해시 값을 구해주는 함수의 포인터(page_hash)
unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED){
	const struct page *p =hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va,sizeof p->va);
}

// 해시 테이블을 초기화할 때 해시 요소를 비교하는 함수의 포인터(page_less)
// a가 b보다 작으면 true, 반대면 false
unsigned
page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
	const struct page *a = hash_entry(a_, struct page, hash_elem); // hash_elem 구조체의 a_포인터를 page구조체의 포인터로 바꾸고 해시 요소의 멤버로 hash_elem을 집어넣겠다
	const struct page *b = hash_entry(b_, struct page, hash_elem);
	return a->va < b->va;
}