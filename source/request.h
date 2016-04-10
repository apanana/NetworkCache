#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cache.h"

char * get_request(cache_t c, char *buff);
char * put_request(cache_t c, char * buff);
char * delete_request(cache_t c, char* buff);
char * head_request(cache_t c);
char * shutdown_request(cache_t c);
char * memsize_request(cache_t *c,char* buff);
char * process_request(cache_t *c,char * buff_in);
