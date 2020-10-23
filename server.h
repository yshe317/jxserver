#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <pthread.h>
#include "queue.h"
#define MAXTHREAD 16

struct tree_node{
	uint8_t source;
	struct tree_node* left;
	struct tree_node* right;
};


typedef struct huffman_tree{
	struct tree_node root;

}huff_tree;
struct fileid{
	uint32_t id;
	char* filename; 
	uint64_t starting;
	uint64_t length;
	uint64_t cursor;
	pthread_mutex_t lock;
	int use;
};

struct idarray{
	struct fileid* ids;
	int length;
};
struct connecter{
	huff_tree* tree;
	uint8_t ** dict;
	pthread_mutex_t* qlock;
	struct threadqueue* t_queue;
	
	int clientsocket;
	struct sockaddr_in client;
	int shutdown;
	int used;
	struct idarray* ret;
	int local_socket;
	pthread_t* threadIDs;
	char* path;
    int* path_len;
};


