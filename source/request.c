#include "request.h"

char * get_request(cache_t c, char *buff){
	uint32_t size;
	uint64_t space;
	key_type k = strtok(NULL,"/");
	val_type out = cache_get(c,k,&size);
	space = cache_space_used(c);
	if (out!=NULL){
		char * json = calloc(1024,1);
		strcat(json,"{key: ");
		strcat(json,(const char*)k);
		strcat(json,", value: ");
		strcat(json,(char *)out);
		strcat(json,"}");
		return json;
	}
	else return "404 Not Found!\n";
}

char * put_request(cache_t c, char * buff){
	key_type k = strtok(NULL,"/");
	val_type v = strtok(NULL,"/");
	// printf("KEY: %s\n",k);
	// printf("VAL???: %s\n",v);
	// printf("LENGTH OF VAL? %d\n",strlen(v));
	cache_set(c,k,v,strlen(v)+1);
	return "201 Created\n";
}

char * delete_request(cache_t c, char* buff){
	key_type k = strtok(NULL,"/");
	cache_delete(c,k);
	return "204 No Content\n";
}
// HTTP version, Date, Accept and Accept and Content-Type
char * head_request(cache_t c){
	// Time
	time_t t = time(NULL);
	struct tm present = * localtime(&t);
	//Space-used
	uint64_t space = cache_space_used(c);
	char * out = calloc(1024,1);
	sprintf(out,"200 OK\nHTTP1.1\nDate: %sAccept: text/plain\nContent-Type: application/json\nSpace-Used: %llu\n",asctime(&present),space);
	return out;
}

char * shutdown_request(cache_t c){
	cache_destroy(c);
	return "204 No Content: Shutting down";
}

char * memsize_request(cache_t *c,char* buff){
	char *size[strlen(buff)+1];
	strcpy(size,strtok(NULL,"/"));
	int memsize = atoi(size);
	cache_destroy(*c);
	cache_t new_c = create_cache(memsize,NULL);
	*c = new_c;
	return "204 No Content: Memory Resized\n";
}

char * process_request(cache_t *c,char * buff_in,int len){
	char * buffer[strlen(buff_in)];
	memset(buffer, '\0', sizeof(buffer));
	strncpy(buffer,buff_in,(strlen(buff_in)-1));
	char *token = strtok(buffer,"/");
	// GET /k
	if(strcmp("GET ",token) == 0){
		return get_request(*c,buffer);
	}
	// PUT /k/v
	else if(strcmp("PUT ",token) == 0){
		return put_request(*c,buffer);
	}
	// DELETE /k
	else if(strcmp("DELETE ",token) == 0){
		return delete_request(*c,buffer);
	}
	// HEAD /k
	else if(strcmp("HEAD ",token) == 0){
		return head_request(*c);
	}
	else if(strcmp("POST ",token) == 0){
		token = (strtok(NULL,"/"));
		// POST /shutdown
		if(strcmp("shutdown",token)==0) 
			return shutdown_request(*c);
		// POST /memsize/value
		else if (strcmp("memsize",token)==0){
			char * out[1000];
			memset(out, '\0', sizeof(out));
			strcpy(out,memsize_request(c,buffer));
			return out;
		}

		else
			return "400 Bad Request: Invalid POST request\n";
	}
	else{
		return "400 Bad Request: Invalid request!\n";
	}
}