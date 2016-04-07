#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "cache.h"
#include "lru.h"

struct cache_obj{
	hash_func hashf;
	uint64_t maxmemory;
	uint64_t occupiedmemory;
	uint64_t buckets;
	uint64_t occupiedbuckets;
	struct cache_keyval * keyvals; // Defined in lru.h
	evict_t lru; // Defined in lru.h
};

uint64_t modified_jenkins(key_type key)
{	
	// Modified jenkins hash function by Alex Ledger. 
    // https://en.wikipedia.org/wiki/Jenkins_hash_function
    uint32_t hash = *key;
    hash += (hash << 10);
    hash ^= (hash >> 6);
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return (uint64_t) hash;
}

// Create a new cache object with a given maximum memory capacity
// and a hash function. This requires that the user has defined
// a struct for keyvals and a struct for lru. 
cache_t create_cache(uint64_t maxmem,hash_func hash){
	cache_t cache = malloc(sizeof(struct cache_obj));
	if (NULL == cache){
		printf("malloc failed for create_cache");
		exit(1);
	}
	
	cache->maxmemory = maxmem;
	cache->occupiedmemory = 0;
	cache->buckets = maxmem;
	cache->occupiedbuckets = 0;

	if(hash == NULL) cache->hashf = modified_jenkins;
	else cache->hashf = hash;

	cache->keyvals = calloc(maxmem,sizeof(struct cache_keyval));

	//Create an evict struct with a constructor from its API.
	cache->lru = create_evict(NULL,NULL,NULL);

	return cache; 
}

// Linear probing of the cache searching for a specified key. 
// The specific way we treat cases depends on whether or not
// we are trying to set an element, so this takes a third argument
// that lets us know whether we are probing to set a val or not.
int linear_probe(cache_t cache, key_type key, bool set){
	int max = 0;
	int index;
	while (max < cache->buckets){
		index = (cache->hashf(key) + max) % cache->buckets;
		if (cache->keyvals[index].key == NULL){
			if (set) return index;
			else ++max;
		}
		else if (strcmp(cache->keyvals[index].key,key)==0) return index;
		else ++ max;
	}
	return -1;
}

// Function that sets a <key,value> pair in the hash table at a specified index.
void cache_set_new(cache_t cache, key_type key, val_type val, uint32_t val_size,uint64_t index){
	cache->keyvals[index].key = calloc(strlen(key)+1,sizeof(uint8_t *));
	strcpy(cache->keyvals[index].key,key);
	cache->keyvals[index].val = malloc(val_size);
	memcpy(cache->keyvals[index].val,val,val_size);
	cache->keyvals[index].val_size = val_size;
	// Update our evict struct with its own function.
	cache->lru->add(cache->lru,index);
	cache->keyvals[index].lru_node = cache->lru->head;

	cache->occupiedbuckets = cache->occupiedbuckets + 1;
	cache->occupiedmemory = cache->occupiedmemory + val_size;
}

// Function that updates a <key,value> pair in the hash table at a specified index.
void cache_update(cache_t cache, key_type key, val_type val, uint32_t val_size,uint64_t index){
	cache->keyvals[index].val = realloc(cache->keyvals[index].val,val_size);
	memcpy(cache->keyvals[index].val,val,val_size);
	// Update our evict struct with its own function.
	cache->lru->update(cache->lru,cache->keyvals[index].lru_node);
	cache->occupiedmemory =
		cache->occupiedmemory-cache->keyvals[index].val_size + val_size;
	cache->keyvals[index].val_size = val_size;
}

// Add a <key, value> pair to the cache.
// If key already exists, it will overwrite the old value.
// If maxmem capacity is exceeded, sufficient values will be removed
// from the cache to accomodate the new value.
void cache_set(cache_t cache, key_type key, val_type val, uint32_t val_size)
{
	// Reject if entry is too large for the cache.
	if (val_size > cache->maxmemory){
		printf("Object is too large for cache\n");
		return;
	}

	// Resize if load factor is exceeded.
	double load = (double)(cache->occupiedbuckets+1.0)/(double)cache->buckets;
	if(load>0.5){
		cache->buckets = cache->buckets*2;
		cache->keyvals = 
			realloc(cache->keyvals,sizeof(struct cache_keyval)*cache->buckets);
	}

	// LRU eviction if maxmem is exceeded.
	int lru;
	uint64_t usedmem = cache->occupiedmemory+val_size;
	while(usedmem>cache->maxmemory){
		if (cache->lru->tail == NULL) lru = cache->lru->head->htable_index;
		else (lru = cache->lru->tail->htable_index);
		cache_delete(cache,cache->keyvals[lru].key);
		usedmem = cache->occupiedmemory+val_size;
	}

	// Linear probing for adding new element.
	uint64_t index = linear_probe(cache,key,true);
	if (index == -1){
		// This should never happen if our cache is properly resizing, which 
		// is why we return an error if no open slots are found.
		printf("Error on linear probing when trying to set cache. Found no slots\n");
		return;
	}
	if (cache->keyvals[index].key == NULL) cache_set_new(cache,key,val,val_size,index);
	else cache_update(cache,key,val,val_size,index);
}


// Retrieve the value associated with key in the cache, or NULL if not found.
// The size of the returned buffer will be assigned to *val_size.
val_type cache_get(cache_t cache, key_type key, uint32_t *val_size){
	// Linear probing to find our key.
	int index = linear_probe(cache,key,false);
	if (index!=-1){
		cache->lru->update(cache->lru,cache->keyvals[index].lru_node);
		*val_size = cache->keyvals[index].val_size;
		return cache->keyvals[index].val;
	}
	// Return NULL if key isnt in our hash table. 
	return NULL;
}


// Delete an object from the cache, if it's still there.
void cache_delete(cache_t cache, key_type key){
	uint64_t index = linear_probe(cache,key,false);
	if (index!=-1){
		free(cache->keyvals[index].key);
		free(cache->keyvals[index].val);
		cache->keyvals[index].key = NULL;
		cache->keyvals[index].val = NULL;
		cache->occupiedmemory = 
			cache->occupiedmemory - cache->keyvals[index].val_size;
		cache->keyvals[index].val_size = 0;
		// Update our evict struct with its own function.
		cache->lru->remove(cache->lru,cache->keyvals[index].lru_node);
		cache->keyvals[index].lru_node = NULL;
		cache->occupiedbuckets = cache->occupiedbuckets -1;	
	}
}


// Compute the total amount of memory used up by all cache values (not keys)
uint64_t cache_space_used(cache_t cache){
	return cache->occupiedmemory;
}

// Destroy all resource connected to a cache object
void destroy_cache(cache_t cache){
	for(int i = 0;i < cache->buckets;++i){
		free(cache->keyvals[i].key);
		free(cache->keyvals[i].val);
	}
	free(cache->keyvals);
	cache->keyvals = NULL;

	//Get rid of the evict_struct with a destructor from its API
	destroy_evict(cache->lru);

	free(cache);
	cache=NULL;

}
