#include <stdlib.h>
#include <stdio.h>
#include "lrutests.h"
#include "test_helper.h"

bool evictions_occur(){
    // adds a ton of elements to a cache with a small maxmem and sees if any elements are evicted
    const uint64_t max_elmts = 10;
    const uint64_t num_elmts_add = 36;
    val_type v = "val";
    cache_t c= create_cache(max_elmts*(strlen(v)+1),NULL);
    char k[1000];
    for(int i=0;i<num_elmts_add;++i){
        strcat(k,"i");
        cache_set(c,k,v,strlen(v)+1);
    }
    int size,i;
    val_type out;
    memset(k,'\0',36);
    for(i=0;i<num_elmts_add - max_elmts;++i){
        strcat(k,"i");
        out = cache_get(c,k,&size);
        if (out != NULL) return false;
    }
    for(;i<num_elmts_add;++i){
        strcat(k,"i");
        out = cache_get(c,k,&size);
        if (out == NULL) return false;
    }
    destroy_cache(c);
    return true;
}

bool basic_lru_test(){
    //adds A then B then adds C and expects A to be evicted
    val_type v = "val";
    const size_t max_mem = 2*(strlen(v)+1);
    cache_t cache = create_cache(max_mem,NULL);
    key_type k1 = "key1";
    key_type k2 = "key2";
    cache_set(cache,k1,v,strlen(v)+1);
    cache_set(cache,k2,v,strlen(v)+1);
    // add k3 to the cache
    key_type k3 = "key3";
    cache_set(cache,k3,v,strlen(v)+1);
    // check if k1 is evicted
    int size;
    val_type out2 = cache_get(cache,k1,&size);
    bool worked = out2==NULL;
    destroy_cache(cache);
    return worked;
}

bool lru_delete_test(){
    //adds A then B then C then gets A then deletes B adds D and expects C to be evicted
    //gets the element
    val_type v = "val";
    const size_t max_mem = 3*(strlen(v)+1);
    cache_t cache = create_cache(max_mem,NULL);
    key_type k1 = "key1";
    key_type k2 = "key2";
    key_type k3 = "key3";
    cache_set(cache,k1,v,strlen(v)+1);
    cache_set(cache,k2,v,strlen(v)+1);
    cache_set(cache,k3,v,strlen(v)+1);
    //gets k1
    int size;
    val_type out1 = cache_get(cache,k1,&size);  
    // deletes k2
    cache_delete(cache,k2);
    // adds k4
    key_type k4 = "key4";
    val_type v2 = "large";
    cache_set(cache,k4,v2,strlen(v2)+1);
    // check if k3 is evicted
    val_type out2 = cache_get(cache,k3,&size);
    bool worked = out2==NULL;
    destroy_cache(cache);
    return worked;
}

bool update_reordering(){
    // Adds A and B, then updates A, and adds a large val C, expecting
    // that B will be evicted and A will be held.
        // need to make this val big enough so we don't run into problems
        // with ambiguity regarding whether we evict >maxmem or ≥ maxmem.
    val_type v = "val";
    cache_t c = create_cache(4*(strlen(v)+1),NULL);
    key_type k1 = "key1";
    key_type k2 = "key2";
    cache_set(c,k1,v,strlen(v)+1);
    cache_set(c,k2,v,strlen(v)+1);
    // this should update the val stored under k1 and now
    // we expect (k2,v2) to be the LRU element.
    val_type v2 = "new";
    cache_set(c,k1,v2,strlen(v)+1);

    key_type k3 = "key3";
    val_type v_large = "largeval";
    //we expect this to evict (k2,v2)
    cache_set(c,k3,v_large,strlen(v_large)+1);

    int size1,size2;
    val_type out1 = cache_get(c,k1,&size1);
    if (out1 == NULL ) return false;
    val_type out2 = cache_get(c,k2,&size2);
    if (out2 != NULL) return false;
    // need to place ifs on sepearate lines there is an issue
    // with all cache_gets sharing the same pointer
    destroy_cache(c);
    return true;

}

bool evict_on_reset_old_val(){
    // Adds A and B, then updates B with a value that would overload
    // the cache if it was added but not if it replaced B. We expect B to be
    // replaced and A to not be evicted.
    val_type v = "val";
    cache_t c = create_cache(3*(strlen(v)+1),NULL);    
    key_type k1 = "key1";
    key_type k2 = "key2";
    cache_set(c,k1,v,strlen(v)+1);
    cache_set(c,k2,v,strlen(v)+1);

    // This should reassign the value under k2, but if the
    // LRU doesn't check the size of what an element would be
    // after reassigning then we would also end up evicting k1
    val_type v2 = "newv!";
    cache_set(c,k2,v2,strlen(v2)+1);

    // This should not be null
    int size;
    val_type out = cache_get(c,k1,&size);

    if(out!=NULL){
        destroy_cache(c);
        return false;
    }
    destroy_cache(c);
    return true;
}

bool evict_on_failed_reset_old_val(){
    // Adds A and B, then updates B with a value that is larger than the entire
    // maxmem. We expect both A and B to not be evicted, but this is ambiguous in 
    // the spec, so we will make more mention of this later in the writeup
    val_type v = "val";
    cache_t c = create_cache(3*(strlen(v)+1),NULL);  
    key_type k1 = "key1";
    key_type k2 = "key2";
    cache_set(c,k1,v,strlen(v)+1);
    cache_set(c,k2,v,strlen(v)+1);

    // This should fail to reassign the value of key2, since the new
    // value is too large for the cache. We want to make sure that
    // none of the old values were evicted.
    val_type v2 = "value too large for cache!";
    cache_set(c,k2,v2,strlen(v2)+1);

    // This should not be null
    int size1,size2;
    val_type out1 = cache_get(c,k1,&size1);
    if(out1==NULL){
        destroy_cache(c);
        return false;
    }
    val_type out2 = cache_get(c,k2,&size1);
    if(out2==NULL){
        destroy_cache(c);
        return false;
    }
    if(strcmp(out2,v2)!=0){
        destroy_cache(c);
        return false;
    }
    destroy_cache(c);
    return true;
}

bool get_reordering(){
    //adds A then B then gets A then adds C and expects A to be evicted
    val_type v = "val";
    const size_t max_mem = 2*(strlen(v)+1);
    cache_t cache = create_cache(max_mem,NULL);
    key_type k1 = "key1";
    key_type k2 = "key2";
    cache_set(cache,k1,v,strlen(v)+1);
    cache_set(cache,k2,v,strlen(v)+1);
    //gets the element
    int size;
    val_type out1 = cache_get(cache,k1,&size);
    // add k3 to the cache
    key_type k3 = "key3";
    cache_set(cache,k3,v,strlen(v)+1);
    // check if k2 is evicted
    val_type out2 = cache_get(cache,k2,&size);
    bool worked = out2==NULL;
    destroy_cache(cache);
    return worked;
}
/*
bool maxmem_not_excceeded(){
    //adds too many elements, checks if the size of values with non-null keys is > maxmem, deletes some, adds more, overwrites some, checks again
    const uint64_t max_emts = 100;
    const uint64_t max_mem = max_emts*sizeof(int_ty);
    cache_t cache = create_cache(max_mem,NULL);

    add_elements(cache,0,max_emts+1,INT);
    bool exceeded = space_of_elements(cache,0,max_emts+1,INT) > max_mem;
    delete_elements(cache,0,max_emts+1);
    add_elements(cache,max_emts*2,max_emts*3,INT);
    exceeded = exceeded || space_of_elements(cache,max_emts*2,max_emts*3,INT) > max_mem;

    destroy_cache(cache);
    return !exceeded;
}
bool elements_not_evicted_early(){
    //adds some elements, deletes some, and replaces some, and then checks if all the elements are still in the cache
    const uint64_t max_emts = 100;
    const uint64_t max_add_emts = max_emts-1;//due to ambiguity about whether the cache can store maxmem or up to, but not including maxmem
    cache_t cache = create_cache(max_emts*sizeof(int_ty),NULL);

    add_elements(cache,0,max_add_emts/2,INT);
    delete_elements(cache,0,max_add_emts/4);
    add_elements(cache,max_add_emts/2, max_add_emts/4 + max_add_emts,INT);
    bool passed = elements_exist(cache,max_add_emts/4,max_add_emts/4 + max_add_emts);
    destroy_cache(cache);
    return passed;
}
*/
bool var_len_evictions(){
    //basic lru_test for variable length strings
    const size_t max_mem = val_size(0,STR)+val_size(1,STR)+val_size(2,STR)-1;
    cache_t cache = create_cache(max_mem,NULL);
    add_element(cache,0,STR);
    add_element(cache,1,STR);
    //gets the element
    element_exists(cache,0);
    add_element(cache,2,STR);
    bool worked = !element_exists(cache,1);
    destroy_cache(cache);
    return worked;
}
