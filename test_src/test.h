#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define failed_code 0x92
typedef bool (*test_fn_ty)();


bool create_init_correct_mem();


bool add_single_item_over_memmax();
bool large_val_copied_correctly();
bool add_same_starting_char();
bool add_over_memmax_eviction();
bool add_resize_buckets_or_maxmem();


bool get_null_empty();
bool get_nonexist();
bool get_size_after_reassign_test();
bool get_val_after_reassign_test();
bool get_with_null_term_strs_test();


bool delete_not_in();
bool delete_affect_get_out();