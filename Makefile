CC = gcc

# CFLAGS = -Wall -pedantic -Werror
# CFLAGS = -Wall
CFLAGS  = -D NDEBUG -O0 -g -w -std=c99

SOURCE = source/

TEST_FILES = test_src/basic_test.c test_src/lrutests.c test_src/main.c test_src/test.c test_src/test_helper.c test_src/testing_client.c
# TEST_FILES = test_src/testing_client.c

RM = rm -rf

# the build target executable:
all:
	make makeserver
	make makedirectclient
	make maketestclient

makeserver: 
	$(CC) $(CFLAGS) $(SOURCE)lru.c $(SOURCE)cache.c $(SOURCE)server.c -o out/server

makedirectclient:
	$(CC) $(CFLAGS) $(SOURCE)direct_client.c -o out/dir_client

maketestclient:
	$(CC) $(CFLAGS) $(TEST_FILES) -o out/test_client
	# $(CC) $(CFLAGS) $(SOURCE)testing_client.c -o out/test_client
	# $(CC) $(CFLAGS) $(TEST_FILES) $(SOURCE)testing_client.c -o out/test_client

runserver: out/server
	./out/server $(ARG)

run_direct_client: out/dir_client
	./out/dir_client

run_test_client: out/test_client
	./out/test_client

debug_server: out/server
	valgrind -v --leak-check=full ./out/server

clean:
	$(RM) out/*