// Alex Pan
//
// lru.c - implementation of the LRU eviction policy.
//
// This is my implementation of the methods for an LRU eviction
// policy. lru_add, lru_update, lru_delete all pertain to maintaining
// a queue. The last two functions, create_evict and destroy_evict, 
// are for initailizing and destroying an evict struct. 


#include <stdlib.h>
#include <stdio.h>
#include "lru.h"

void lru_add(evict_t e, int hashtable_index){
	node_t n = malloc(sizeof(struct node));
	n->htable_index = hashtable_index;
	// If queue is empty
	if (e->head == NULL){
		e->head = n;
		n->prev = NULL;
		n->next = NULL;
	}
	// If queue is of only one element
	else if (e->tail == NULL){
		e->tail = e->head;
		e->tail->prev = n;
		n->next = e->tail;
		n->prev = NULL;
		e->head = n;
	}
	else{
		e->head->prev = n;
		n->next = e->head;
		n->prev = NULL;
		e->head = n;
	}
}

void lru_update(evict_t e, node_t node){
	// If node doesn't exist
	if (node == NULL) return;
	// If node is the head of the queue, do nothing
	else if (e->head == node) return;
	// If node is the tail of the queue
	else if (e->tail == node){
		e->tail = node->prev;
		e->tail->next = NULL;
		node->prev = NULL;
		node->next = e->head;
		e->head->prev = node;
		e->head = node;
	}
	else{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		node->prev = NULL;
		e->head->prev = node;
		node->next = e->head;
		e->head = node;
	}
}

void lru_delete(evict_t e, node_t node){
	// If node doesn't exist
	if (node == NULL) return;
	// If the queue is empty
	else if (e->head==NULL){
		printf("Error: Attempting to delete from an empty queue!\n");
	}
	// If the queue is of only one element
	else if (e->tail == NULL){
		free(node);
		e->head=NULL;
	}
	// If the queue is of only two elements
	else if (e->head->next == e->tail){
		if(e->head == node){
			free(node);
			e->head = e->tail;
			e->head->prev = NULL;
			e->tail = NULL;
		}
		else{
			free(node);
			e->tail = NULL;
			e->head->next = NULL;
		}
	}
	// If the node is the head of the queue
	else if (e->head==node){
		e->head = e->head->next;
		free(node);
		e->head->prev = NULL;
	}
	// If the node is the tail of the queue
	else if (e->tail==node){
		e->tail = e->tail->prev;
		free(node);
		e->tail->next = NULL;	
	}
	else{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		free(node);
	}
}


// This initalizes an evict struct and returns a pointer to it. It allows
// the user to define what functions they want to use in their struct.
// If the user does not define anything, then we default to the LRU functions.
evict_t create_evict(add_func add, update_func update, remove_func remove){
	evict_t e = calloc(1,sizeof(struct evict));
	if (e->head!=NULL) printf("calloc failed: head not null\n");
	if (e->tail!=NULL) printf("calloc failed: tail not null\n");
	if (add==NULL) e->add = lru_add;
	if (update==NULL) e->update = lru_update;
	if (remove==NULL) e->remove = lru_delete;
	return e;
}

// This destroys an evict struct and all connected resources.
void destroy_evict(evict_t e){
	while (e->head!=NULL) lru_delete(e,e->head);
	if (e->head!=NULL) printf("Head not null\n");
	if (e->tail!=NULL) printf("Tail not null\n");
	free(e);
	e = NULL;
}
