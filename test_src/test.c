#include "test.h"
#include "test_helper.h"

bool cache_space_preserved(){
    // adds, deletes, updates, and maybe evicts and sees if the total size of
    // items of items in the cache is the size of all of the non-NULL elements
    const uint64_t maxmem = 200;
    cache_t c = create_cache(maxmem,NULL);
    add_elements(c,0,5,STR);
    delete_element(c,4);
    add_elements(c,0,2,STR);
    bool worked = cache_space_used(c) == space_of_elements(c,0,4,STR) + space_of_elements(c,5,5,STR);
    destroy_cache(c);
    return worked;
}

// we have a weird error with jcosel where it doubles its maxmem cap if
// we exceed maxmem. i dont know what kind of test i'd use to expose it though

bool create_init_correct_mem(){
    // cache space used should still be 0 because we shouldnt be
    // able to add an element greater than the size of the cache
    // fails jcosel because of resize on caches too small
    // fails zzhong because doesn't check if new val exceeds maxmem
    cache_t c = create_cache(65,NULL); //set over 64 for jcosel
    key_type k = "key";
    val_type v = "string too long! string too long! string too long! \
    string too long! string too long! string too long! string too long!";
    cache_set(c,k,v,strlen(v)+1);
    int space = cache_space_used(c);
    destroy_cache(c);
    if (space!=0) return false;
    return true;
}

bool add_single_item_over_memmax(){
    //adds a single item over maxmem and sees if it is not in the cache.
    cache_t c = create_cache(65,NULL); //set over 64 for jcosel
    key_type k = "key";
    val_type v = "string too long! string too long! string too long! \
    string too long! string too long! string too long! string too long!";
    cache_set(c,k,v,strlen(v)+1);
    uint32_t null_size = 0;
    val_type cache_big_val = cache_get(c,k,&null_size);
    destroy_cache(c);
    return cache_big_val == NULL;
}

/*
bool large_val_copied_correctly(){
    // Tests cache_set on an array containing two large values. If vals
    // were treated as strings, this would fail.
    cache_t cache = create_cache(1000,NULL);
    key_type key = "normal key";
    uint64_t val[] = {0xff00ff00ff00ffff,0xcc00cc00fe00ddcc};
    cache_set(cache,key,&val,sizeof(uint64_t)*2);
    uint32_t size = -1;
    val_type outval = cache_get(cache,key,&size);
    bool worked;
    if(outval != NULL){
        uint64_t * out_arr = (uint64_t *)(outval);
        worked = out_arr[0] == val[0] && out_arr[1] == val[1];
    }
    else{
        worked = false;
    }
    destroy_cache(cache);
    return worked;
}
*/
bool add_same_starting_char(){
    // adds vals under different keys that start with the same character.
    // if the cache doesn't copy keys by string then this will fail.
    cache_t c = create_cache(10000,NULL);
    char k[1000];
    val_type v = "val";
    int size;
    for(int i=0;i<20;++i){
        strcat(k,"i");
        cache_set(c,k,v,strlen(v)+1);
        size = cache_space_used(c);
        if (size!=((i+1)*(strlen(v)+1))){
            destroy_cache(c);
            return false;
        }
    }
    destroy_cache(c);
    return true;
}


bool add_over_memmax_eviction(){
    // jcosel gets a false pass because they resize their maxmem
    // adds small items to cache and then adds an item larger than maxmem
    // and sees if items have been evicted. (expect them to not be).
    val_type v = "v";
    uint64_t max_mem = 10;
    char rand_key[] = "random_key";
    char big_val[] = "string of length > max_mem asdfasdfasdfasdfasdfasdfasdf";

    cache_t c = create_cache(max_mem*(strlen(v)+1),NULL);
    key_type k1 = "key1";
    key_type k2 = "key2";
    key_type k3 = "key3";
    cache_set(c,k1,v,strlen(v)+1);
    cache_set(c,k2,v,strlen(v)+1);
    cache_set(c,k3,v,strlen(v)+1);
    cache_set(c,rand_key,big_val,strlen(big_val)+1);

    int size;
    if (cache_get(c,k1,&size)==NULL) return false;
    if (cache_get(c,k2,&size)==NULL) return false;
    if (cache_get(c,k3,&size)==NULL) return false;
    destroy_cache(c);
    return true;
}

bool add_resize_buckets_or_maxmem(){
    // adds small items up to half of maxmem then attemps to add
    // an item that would be too large for the cache (unless maxmem changed
    // after the resize). If new item appears, then maxmem was changed
    // in resize (which is a bug - maxmem should be constant).
    val_type v = "v";
    uint64_t max_mem = 6;
    char rand_key[] = "random_key";
    char big_val[] = "string of length > max_mem";

    cache_t c = create_cache(max_mem*(strlen(v)+1),NULL);

    key_type k1 = "key1";
    key_type k2 = "key2";
    key_type k3 = "key3";
    cache_set(c,k1,v,strlen(v)+1);
    cache_set(c,k2,v,strlen(v)+1);
    cache_set(c,k3,v,strlen(v)+1);

    int space1 = cache_space_used(c);

    cache_set(c,rand_key,big_val,strlen(big_val)+1);
    uint32_t null_size = 0;
    val_type cache_big_val = cache_get(c,rand_key,&null_size);

    int space2 = cache_space_used(c);

    if (cache_big_val != NULL){
        printf("%d\n",space1);
        printf("%d\n",space2);
        printf("%s\n",cache_big_val);
        printf("%p\n",cache_big_val);
        fflush(stdout);
        return false;
    }
    destroy_cache(c);
    return cache_big_val == NULL;
}

bool get_null_empty(){
    // adds things to our cache and then attempts to get one that
    // doesn't exist
    val_type v = "v";
    uint64_t max_mem = 100;
    cache_t c = create_cache(max_mem*(strlen(v)+1),NULL);
    char k[1000];
    for(int i=0;i<75;++i){
        strcat(k,"i");
        cache_set(c,k,v,strlen(v)+1);
    }
    key_type unk_key = "key";
    int size;
    void * out = cache_get(c,unk_key,&size);
    destroy_cache(c);
    if (out != NULL) return false;
    return true;
}

bool get_nonexist(){
    // attempts to get an elements that doesn't exist in an empty cache
    cache_t c = create_cache(1000,NULL);
    key_type k = "nonexist";
    int size;
    val_type out = cache_get(c,k,&size);
    destroy_cache(c);
    if(out != NULL) return false;
    return true;
}


bool get_size_after_reassign_test(){
    // Tests if space from cache_get remains the same after reassigning a val
    cache_t c = create_cache(1000,NULL);
    key_type k = "key";
    val_type v1 = "val1";
    int size1,size2;
    cache_set(c,k,v1,strlen(v1)+1);
    val_type out1 = cache_get(c,k,&size1);

    val_type v2 = "newval";
    cache_set(c,k,v2,strlen(v2)+1);
    val_type out2 = cache_get(c,k,&size2);
    destroy_cache(c);
    if(strcmp(out1,out2)==0){
        printf("%s\n",out1);
        printf("%s\n",out2);
        return false;
    }
    return true;
}

// exposes some problems with the val that cache_get returns.
// if we don't copy out the value to something totally new,
// we end up reassigning the same pointer over and over.
// so updating it raises an error because we also update
// old outs.
bool get_val_after_reassign_test(){
    // Tests if the val from cache_get remains the same after reassigning a val
    cache_t c = create_cache(1000,NULL);
    char * k = "key";
    char *v1 = "stringval1";
    int size1,size2;
    cache_set(c,k,v1,strlen(v1)+1);
    void * out1 = cache_get(c,k,&size1);
    char *v2 = "stringval2";
    cache_set(c,k,v2,strlen(v2)+1);
    void * out2 = cache_get(c,k,&size2);
    if(strcmp(out1,out2)==0 ||
        strcmp(out1,v1)!=0 ||
        strcmp(out2,v2)!=0){
        printf("%s\n",out1);
        printf("%s\n",out2);
        printf("%s\n",v1);
        printf("%s\n",v2);
        destroy_cache(c);
        return false;
    }
    destroy_cache(c);
    return true;
}


bool get_with_null_term_strs_test(){
    // Tests keys cache_set on two different keys that contain a null termination in
    // the middle: "a\0b" and "a\0c". We expect cache_set to overwrite the first val
    // with the second val because both keys 'look the same' (ie "a\0").
    cache_t cache = create_cache(100,NULL);
    key_type key1 = "a\0b";
    key_type key2 = "a\0c";
    val_type val1 = "val1";
    val_type val2 = "val2";
    cache_set(cache,key1,val1,strlen(val1)+1);
    cache_set(cache,key2,val2,strlen(val2)+1);
    uint32_t size = -1;
    val_type outval = cache_get(cache,key1,&size);
    bool worked = (strcmp(outval,val2) == 0);
    destroy_cache(cache);
    return worked;
}

bool delete_not_in(){
    // Tests to see if something that is set and then deleted returns NULL
    // when cache_get is called.
    cache_t cache = create_cache(max_str_len+1,NULL);
    const uint64_t item = 10;
    add_element(cache,item,STR);
    delete_element(cache,item);
    bool worked = !element_exists(cache,item);

    destroy_cache(cache);
    return worked;
}

bool delete_affect_get_out(){
    // A bug was raised with the outputed vals of cache_get being affected
    // by updates. This tests whether we have the same problem on the outputs
    // of cache_get after deletes.
    cache_t c = create_cache(1000,NULL);
    char * k = "key";
    char *v1 = "stringval1";
    int size1,size2;
    cache_set(c,k,v1,strlen(v1)+1);
    void * out1 = cache_get(c,k,&size1);
    cache_delete(c,k);
    void * out2 = cache_get(c,k,&size1);
    if (out1 == NULL){
        destroy_cache(c);
        return false;
    }
    destroy_cache(c);
    return true;
}
