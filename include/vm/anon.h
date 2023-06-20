#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
struct page;
enum vm_type;
struct bitmap *swap_table;          // project3 - Anonymous Page 추가 / 0 - empty, 1 - filled
int bitcnt;

struct anon_page {
    uint32_t swap_slot_no;          // project3 - Anonymous Page 추가 구현 / swap out될 때 이 페이지가 저장된 slot의 번호
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
