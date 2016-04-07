// Alex Pan
//
// lru.h - header file for LRU eviction policy.
//
// This basically is the placeholder for a user-defined eviction policy
// and the necessary struct and methods. This is implemented as a
// doubly-linked list that acts as a queue. For more abstraction, I've
// created a struct called evict that holds all the necessary pointers
// to functions and structs it will need. These can be redefined
// or modified by the user.
#include <stdint.h>
struct node{
	int htable_index;
	struct node *prev;
	struct node *next;
};
typedef struct node * node_t;


// Struct for key/value pair that will be stored in the cache.
// This keeps track of a key and value pair, the size of the
// value, and contains a pointer to an element in the LRU
// queue struct.
struct cache_keyval{
	// Key, value, size.
	uint8_t * key;
	void * val;
	int val_size;
	// Pointer to respective node in the queue
	node_t lru_node;
};


struct evict;
typedef void (*add_func)(struct evict * e, int hashtable_index);
typedef void (*update_func)(struct evict * e, node_t node);
typedef void (*remove_func)(struct evict * e, node_t node);

// I've created function pointers to the functions that are used by
// the evict struct. In particular, I've implemented lru_add,
// lru_update, and lru_delete to manipulate the queue. In theory,
// if a user wanted to use a different policy, (eg. MRU eviction),
// all they would have to do is define the necessary functions
// and then specify them when calling create_evict (see futher below).
typedef struct evict{
	// size of queue, head, and tail
	node_t head;
	node_t tail;
	// functions for adding, updating, removing, and destroying queue
	add_func add;
	update_func update;
	remove_func remove;
}* evict_t;


// Adds a node to the queue.
void lru_add(evict_t e, int hashtable_index);

// Moves a node that already exists in the queue to the front
// and properly readjusts the pointers in the rest of the queue
void lru_update(evict_t e, node_t node);

// Deletes a node from the queue.
void lru_remove(evict_t e, node_t node);


// I've put the create_evict and destroy_evict seperately from the lru
// specific functions because these are for the creation and destruction
// of the evict_struct. While these can be modified, I'm putting them in a
// "seperate layer" so that if you want to keep the overall structure of the
// eviction policy but modify the way in which we order things, these functions
// won't have to change.
// Creates an evict struct.
evict_t create_evict(add_func add, update_func update, remove_func remove);

// Destroys all resources connected to a queue.
void destroy_evict(evict_t e);
