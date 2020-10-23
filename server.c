#include "server.h"

huff_tree tree;
uint8_t ** dict;
int init_idarray(struct idarray* retrievetool) {
	retrievetool->length = 0;
	retrievetool->ids = NULL;
	return 0;
}

int if_idexist(uint32_t id,struct idarray* retrievetool) {
	for(int i = 0; i < retrievetool->length;i++) {
		if(id == retrievetool->ids[i].id) {
			return i;
		}
	}
	return -1;
}


uint8_t** read_compress_dict() {
	FILE* f = fopen("compression.dict","rb");
	uint8_t** b = (uint8_t**)malloc(sizeof(uint8_t*) * 256);
	memset(b,0,256*sizeof(uint8_t*));
	uint32_t cursor = 0;
	uint8_t buffer;
	fread(&buffer,1,1,f);
	uint8_t len;
    for(int i = 0;i<256;i++) {
        if(cursor == 0){
			len = buffer;
			fread(&buffer,1,1,f);
		}else{
			len = buffer<<cursor;//read the remain bits in last byte
			fread(&buffer,1,1,f);
			len = len + (buffer>>(8-cursor)); 
		}
        b[i] = (uint8_t*)malloc(5);
		memset(b[i],0,5);
		uint8_t* temp = b[i];
		temp[0] = len; // set the len
		int index = 1;
        //printf("\n");
		for(int current = 0;current+8 <= len;current+=8) {
            //printf("the current is %d and len is %d\n",current,len);
            //printf("%x",buffer);
			if(cursor != 0) {
				temp[index] = buffer<<cursor;
				fread(&buffer,1,1,f);
				temp[index] += (buffer>>(8-cursor));
			}else{
				temp[index] = buffer;
				fread(&buffer,1,1,f);
			}
			index ++;
        }
        //printf("\n");
		int remain =(int)len%8;

        //printf("%x\n",buffer);
        //printf("%d\n",remain);
        if(remain!=0) {

			if(cursor+remain <8){
                // printf("should be e :%x\n",(buffer<<cursor));
                // printf(" the index is %d\n",index);
				temp[index] = buffer<<cursor;
				cursor = cursor + remain;
            }else{
				temp[index] = buffer<<cursor;
				fread(&buffer,1,1,f);
				temp[index] += (buffer>>(8-cursor));
				cursor = (cursor + remain)%8;
			}
        }

        //printf("%x\n",(uint8_t)buffer << 4);

    }
	fclose(f);
    return b;
}

uint8_t getbit(uint8_t* string,int index) {
	int i = index/8;//0
	int move = index%8;//3
	return (string[i] >>(8-move-1)) & 1;
}
  
int init_tree() {
	tree.root.left = NULL;
	tree.root.right = NULL;
	for(unsigned int i = 0; i<256; i++) {
		//printf("%x",(content>>(32-len-1))&1);

		uint8_t len = dict[i][0];
		uint8_t* content = &dict[i][1];
		
		struct tree_node* current = &tree.root;
		for(int j = 0; j<len; j++) {
			if(getbit(content,j)) { 
				//go to right child when bit is 1
				if(current->right == NULL) {
					current->right = calloc(1,sizeof(struct tree_node));
					// current->right->left = NULL;
					// current->right->right= NULL;
				}
				current = current -> right;
			}else{
				//go to left child when bit is 0
				if(current->left == NULL) {
					current->left = calloc(1,sizeof(struct tree_node));
					// current->left->left = NULL;
					// current->left->right= NULL;
				}
				current = current->left;
			}
		}
		current->source = i;
		//printf("",current->source);
	}
	return 0;
}

uint8_t* decode_data(uint8_t* compressed,size_t length,uint64_t* size) {
	uint8_t* result = NULL;
	uint8_t padding = compressed[length-1];
	int totalbit = (length-1)*8-padding;
	uint64_t len = 0;
	struct tree_node* current = &tree.root;
	//printf(" the bits have %d\n",totalbit);
	for(int i = 0; i<totalbit; i++) {
		//printf("%x",getbit(compressed,i));
		if(getbit(compressed,i)) {
			//when the bit is 1
			//printf("\n%x",current->source);
			if(current->right == NULL) {
				len++;
				result = realloc(result,len);
				result[len-1] = current->source;
				//printf("%x",current->source);
				current = tree.root.right;
			}else{
				current = current->right;
			}
		}else{
			if(current->left == NULL) {
				len++;
				result = realloc(result,len);
				result[len-1] = current->source;
				current = tree.root.left;
			}else{
				current = current->left;
			}
		}
	
	}	

	if(current != &tree.root) {
		len++;
		result = realloc(result,len);
		result[len-1] = current->source;
	}
	//printf("%llu",len);
	free(compressed);
	(*size) = len;
	return result;
}

int send_error(int socketfd,char* response) {
	response[0] = 0xF0;
	send(socketfd,response,9,0);
	return 0;
}

uint8_t* compress_data(uint8_t* uncompress,size_t length,uint64_t* size) {
	//need to add padding
	uint8_t cursor = 0;
	uint8_t* result = NULL;
	int len = 0;
	int index = 0;
	int all_bits = 0;
	for(int i = 0;i <length;i++) {
        //compress one byte of data each time
		uint8_t* temp =  dict[uncompress[i]];
		// get how many bits it have;
		uint8_t lenoftemp_in_bit = temp[0];
		all_bits += lenoftemp_in_bit;
		uint8_t lenoftemp_in_byte = lenoftemp_in_bit/8;
		uint8_t remain = all_bits % 8;
		if(remain != 0) {
			len = all_bits/8 +1;
		}else{
			
			len = all_bits/8;
		}
		result = (uint8_t*)realloc(result,len);
		// push the main compressed data
		for (int j = 1; j <lenoftemp_in_byte+1; j++) {
			if(cursor == 0) {
				result[index] = temp[j];
				index++;
			}else{ 
				result[index] += temp[j]>>cursor;
				index++;
				result[index] = temp[j]<< (8-cursor);
			}

        }
		// push the remain compressed data
		if(lenoftemp_in_bit%8== 0) {continue;}

		if(remain == 0) {
			result[index] += temp[lenoftemp_in_byte+1] >> cursor;
			cursor = remain;
		}else if(cursor < remain) {
			if(cursor == 0){
				result[index] = temp[lenoftemp_in_byte+1] >> cursor;
			}else{
				result[index] += temp[lenoftemp_in_byte+1] >> cursor;
			}
			
			cursor = remain;
        }else{
			
			result[index] += temp[lenoftemp_in_byte+1] >> cursor;
			index++;
			result[index] = temp[lenoftemp_in_byte+1] << (8-cursor);
			cursor = remain;
			
		}
		if(cursor == 0) {
			index = len;
		}else{
			result[index] = (result[index] >>(8-cursor))<<(8-cursor);
		}
	}

	
	len=len+1;
	//to hold padding
	result = (uint8_t*)realloc(result,len);
	uint8_t padding;
	if(cursor != 0) {
		padding = 8-cursor;
	}else{
		padding = 0;
	}
	result[len-1] =padding;
	(*size) = len;
	return result;
}

uint64_t little_big_endian(uint8_t* little_endian){
	uint8_t big_indian[8];
	for(int i = 0; i<8; i++) {
		big_indian[i] = little_endian[7-i];
	} 
	uint64_t len = * (uint64_t*) & big_indian[0];
	return len;
}

int echo_content(int client_socket,uint64_t len,int r_compress) {
	// unsigned char buffer;
	unsigned char* temp = (unsigned char*)malloc(len);
	read(client_socket,temp,len);
	if(r_compress == 1) {
		uint64_t size;
		uint8_t* compressed_data = compress_data((uint8_t*)temp,len,&size);
		//size+= 1;
		uint64_t little_endian = little_big_endian((uint8_t*)&size);
		send(client_socket,&little_endian,8,0);
		//printf("%x",dict[0x68][1]);
		send(client_socket,compressed_data,size,0);
		//size = 0;
		//send(client_socket,&size,1,0);
		free(compressed_data);
	}else{
		send(client_socket,temp,len,0);
	}
	free(temp);
	return 0;
}

uint64_t get_send_filesize(FILE* file, int socketfd,int r_compress){
	fseek(file,0,SEEK_END);
	uint64_t len = ftell(file);
	fclose(file);
	if(r_compress == 1){
		uint64_t size= 8;
		unsigned char* temp = (unsigned char*)&size;
		for(int i = 8;i>0;i--) {
			send(socketfd,&temp[i-1],1,0);
		}
		temp = (unsigned char*)&len;
		for(int i = 8;i>0;i--) {
			send(socketfd,&temp[i-1],1,0);
		}
	}else if(r_compress == 0) {
		uint64_t size;
		len = little_big_endian((uint8_t*)&len);
		uint8_t* compressed = compress_data((uint8_t*)&len,8,&size);
		unsigned char* temp = (unsigned char*)&size;
		for(int i = 8;i>0;i--) {
			send(socketfd,&temp[i-1],1,0);
		}
		send(socketfd,compressed,size,0);
		free(compressed);
	}
	return len;
}

uint64_t read_send_files_in_dir(char* path,int socketfd,int r_compress) {
	DIR * dir;
	struct dirent * file;
	if((dir = opendir(path)) == 0) {
		printf("dir incorrect or edge\n");
		return -1;
	}
	uint64_t count = 0;
	// need to change names from table to a chain
	//char** names = NULL;
	char* names =NULL;
	uint64_t len = 0;
	while((file = readdir(dir)) != NULL) {
		if(strcmp(file->d_name,".")==0 || strcmp(file->d_name,"..")==0) {
            continue;
		}else if(file->d_type == 8 || file->d_type == 10) {
			//file
			//count++;
			if(count == 0){
				len = strlen(file->d_name)+1;
				names = (char*)realloc(names,len);
				strcpy(names,file->d_name);
				names[len-1] = '\0';
				count++;
			}else{
				uint64_t newlen =len + strlen(file->d_name)+1;
				names = (char*)realloc(names,newlen);
				strcpy(&names[len],file->d_name);
				len = newlen;
				names[len-1] = '\0';
			}
		}
	}
	//printf("\n%ld\n",count);
	closedir(dir);
	if(r_compress == 0) {
		char* temp = (char*)&len;
		for(int i = 8;i>0;i--) {
			send(socketfd,&temp[i-1],1,0);
		}
		send(socketfd,names,len,0);
	}else if(r_compress == 1) {
		uint8_t* compressed = compress_data((uint8_t*)names,len,&len);
		char* temp = (char*)&len;
		for(int i = 8;i>0;i--) {
			send(socketfd,&temp[i-1],1,0);
		}

		send(socketfd,compressed,len,0);
		free(compressed);
	}
	free(names);
	return count;
}

char* nodecode_getfilename(int len,int socketfd,struct connecter* c){
	char* filename =  (char*)malloc(len+2+(*c->path_len));
	memset(filename,0,len+2+(*c->path_len));
	strcpy(filename,c->path);


	char* temp =  (char*)malloc(len+1);
	memset(temp,0,len+1);

	read(socketfd,temp,len);

	strcat(filename,"/");
	//printf("/n%s",temp);
	strcat(filename,temp);
	
	free(temp);



	return filename;
}


int getfilecontent(FILE* file,uint64_t starting,uint64_t len,int r_compress,int socketfd,int index, struct idarray* retrievetool) {
	fseek(file,starting,SEEK_SET);
	char* buffer = (char*)malloc(len);
	
	fread(buffer,len,1,file);

	if(r_compress == 1) {
		uint64_t size;
		uint8_t* pre_compress = malloc(len+20+1);
		//printf("the len is %ld\n",len);
		for(int i = 0;i<len;i++) {
			//printf("index is %i\n",i);
			pre_compress[20+i] = buffer[i];
		}
		*(uint32_t*)pre_compress = retrievetool->ids[index].id;
		starting = little_big_endian((uint8_t*)&starting);
		*(uint64_t*)&pre_compress[4] = starting;
		starting = little_big_endian((uint8_t*)&len);
		*(uint64_t*)&pre_compress[12] = starting;
		uint8_t* compressed = compress_data(pre_compress,len+20,&size);
		// uint64_t totallen = size+20;
		starting = little_big_endian((uint8_t*)&size);
		send(socketfd,&starting,8,0);
		send(socketfd,compressed,size,0);
		free(pre_compress);
		free(compressed);
	}else{
		uint64_t totallen = len+20;
		totallen = little_big_endian((uint8_t*)&totallen);
		send(socketfd,&totallen,8,0);
		send(socketfd,&retrievetool->ids[index].id,4,0);
		starting = little_big_endian((uint8_t*)&starting);
		send(socketfd,&starting,8,0);
		starting = little_big_endian((uint8_t*)&len);
		send(socketfd,&starting,8,0);
		send(socketfd,buffer,len,0);
	}
	
	free(buffer);
	return 0;
}

void* connection_handler(void* arg) {
	struct connecter *c = (struct connecter*) arg;
	int socketfd = c->clientsocket;
	char buffer[10];
	memset(buffer, 0, 10);
	while(recv(socketfd,buffer,9,0) > 0) {
		uint8_t temp = buffer[0];
		uint8_t type = temp >> 4;
		int compressed = (temp>>3)&0b00000001;
		int requirec = (temp>>2)&0b00000001;

		// printf("the origin is %x\n",temp);
		// printf("the type is %x\n",type);

		uint64_t len =little_big_endian((uint8_t*)&buffer[1]) ;//* (uint64_t*) & big_indian[0];
		char response[9];
		memset(response,0,9);
		struct idarray* retrievetool = c->ret; 
		if(type == 0x0) {
				//echo
				if(compressed == 1) {
					response[0] = 0b00001000;
				}
				if(requirec == 1) {
					response[0] = 0b00001000;
				}
				response[0] += 0x10;
				// send header
				send(socketfd,response,1,0); 
				
				if(requirec == 1 && compressed == 0){
					echo_content(socketfd,len,1); 
				}else{
					// send length of payload
					send(socketfd,&buffer[1],8,0); 
					// send payload
					echo_content(socketfd,len,0); 
				}
		}else if(type == 0x2) {

				if(requirec == 1) {
					response[0] = 0b00001000;
					response[0] += 0x30;
					send(socketfd,response,1,0);
					//printf("%s",c->path);
					read_send_files_in_dir(c->path,socketfd,1);
				}else{
					response[0] = 0x30;
					send(socketfd,response,1,0);
					//printf("%s",c->path);
					read_send_files_in_dir(c->path,socketfd,0);
				}
				
		}else if(type == 0x4) {
				char* filename;
				if(compressed) {
					uint8_t* reading_data = malloc(len);
					read(socketfd,reading_data,len);
					uint64_t size;
					reading_data = decode_data(reading_data,len,&size);
					char* filename =  (char*)malloc(size+2+(*c->path_len));
					memset(filename,0,size+2+(*c->path_len));
					strcpy(filename,c->path);
					strcat(filename,"/");
					for(int i = 0;i<size;i++) {
						filename[(*c->path_len)+i+1] = reading_data[i]; 
					}
					
					free(reading_data);
				}else{
					filename = nodecode_getfilename(len,socketfd,c);
				}
				FILE* target = fopen(filename,"r");
				//printf("\n%s\n",filename);
				if(target == NULL) {
					send_error(socketfd,response);
				}else{
					response[0] += 0x50;
					if(requirec == 1) {
						response[0] += 0b00001000;
						send(socketfd,response,1,0);
						get_send_filesize(target,socketfd,0);
					}else{
						send(socketfd,response,1,0);
						get_send_filesize(target,socketfd,1);
					}
				}
				free(filename);
		}else if(type == 0x6) {

				uint32_t id;
				uint64_t starting;
				uint64_t size;
				char* filename;
				if(compressed){
					uint8_t * reading_data = malloc(len);
					read(socketfd,reading_data,len);
					uint64_t size_of_uncompress;
					reading_data = decode_data(reading_data,len,&size_of_uncompress);
					id = *(uint32_t*)&reading_data[0];
					// uint8_t temp[8];
					starting = *(uint64_t*)&reading_data[4];
					//printf("the starting and length is %lu and %lu",starting,size);
					starting = little_big_endian((uint8_t*)&starting);
					size = *(uint64_t*)&reading_data[12];
					size = little_big_endian((uint8_t*)&size);
					int filename_len = size_of_uncompress - 20;
					filename =(char*)calloc(1,filename_len+2+(*c->path_len));
					strcpy(filename,c->path);
					strcat(filename,"/");
					for(int i = 0;i<filename_len;i++) {
						filename[(*c->path_len)+i+1] = reading_data[20+i]; 
					}
					free(reading_data);
				}else{
					read(socketfd,&id,4);
					uint8_t temp[8];
					read(socketfd,temp,8);
					starting = little_big_endian(temp);
					read(socketfd,temp,8);
					size = little_big_endian(temp);
					filename = nodecode_getfilename(len-20,socketfd,c);
				}
				//get filename
				FILE* file = fopen(filename,"r");
				
				if(file == NULL) {
					free(filename);
					send_error(socketfd,response);
				}else{
					fseek(file,0,SEEK_END);
					uint64_t filesize = ftell(file);
					if(filesize - starting < size) {
						free(filename);
						send_error(socketfd,response);
						continue;
					}
					int index = if_idexist(id,retrievetool);
					if(index == -1) {
						retrievetool->length++;
						retrievetool->ids = (struct fileid*)realloc(retrievetool->ids,retrievetool->length*sizeof(struct fileid));
						index = retrievetool->length-1;
						// printf("the len is %d\n",retrievetool.length);
						// printf("the index is %d\n",index);
						retrievetool->ids[index].filename = filename;
						retrievetool->ids[index].cursor = starting;
						retrievetool->ids[index].id = id;
						retrievetool->ids[index].use = 1;
						retrievetool->ids[index].length = size;
						retrievetool->ids[index].starting = starting;
						pthread_mutex_init(&retrievetool->ids[index].lock,NULL);
					}else{
						if(retrievetool->ids[index].use != 0) {
							if(strcmp(retrievetool->ids[index].filename,filename)!=0) {
								send_error(socketfd,response);
								continue;
							}
							free(filename);
							if(retrievetool->ids[index].length != size) {
								send_error(socketfd,response);
								continue;
							}
							if(retrievetool->ids[index].starting != starting) {
								send_error(socketfd,response);
								continue;
							}
						}else{
							if(strcmp(filename,retrievetool->ids[index].filename)== 0) {
								free(filename);
								if(retrievetool->ids[index].length == size && retrievetool->ids[index].starting == starting) {
									response[0] = 0x70;
									if(requirec == 1) {
										response[0]+= 0b00001000;
									}
									send(socketfd,response,9,0);
									continue;
								}
							}
							free(retrievetool->ids[index].filename);
							retrievetool->ids[index].filename = filename;
							retrievetool->ids[index].cursor = starting;
							retrievetool->ids[index].id = id;
							retrievetool->ids[index].length = size;
							retrievetool->ids[index].starting = starting;
							
						}
					}
					
					response[0] = 0x70;
					if(requirec == 1) {
						response[0]+= 0b00001000;
					}
					uint64_t ending = starting + size;
					uint64_t cursor = starting;

					//the data transfer each time
					uint64_t len_nexttime = size/5;
					while(1) {
						pthread_mutex_lock(&retrievetool->ids[index].lock);
						if(retrievetool->ids[index].cursor >= ending) {
							pthread_mutex_unlock(&retrievetool->ids[index].lock);
							break;
						}
						if(retrievetool->ids[index].cursor+len_nexttime<=ending) {
							cursor = retrievetool->ids[index].cursor;
							retrievetool->ids[index].cursor+=len_nexttime;
							pthread_mutex_unlock(&retrievetool->ids[index].lock);
						}else{
							cursor = retrievetool->ids[index].cursor;
							len_nexttime = ending - cursor;
							retrievetool->ids[index].cursor = ending;
							pthread_mutex_unlock(&retrievetool->ids[index].lock);
						}
						send(socketfd,response,1,0);
						if(requirec == 0){
							getfilecontent(file,cursor,len_nexttime,0,socketfd,index,retrievetool);
						}else{
							getfilecontent(file,cursor,len_nexttime,1,socketfd,index,retrievetool);
						}
						
					}		
					//free the tool and resize it;
					pthread_mutex_lock(&retrievetool->ids[index].lock);
					retrievetool->ids[index].use = 0;
					pthread_mutex_unlock(&retrievetool->ids[index].lock);
					fclose(file);

				}
				
				// break;
		}else if(type == 0x8) {		
			response[0] = 0x80;			
			exit(0);
		}else{
			send_error(socketfd,response);
			break;
		}
	}
	pthread_mutex_lock(c->qlock);
	enqueue(c->t_queue, c->used);
	pthread_mutex_unlock(c->qlock);
	close(socketfd);
	return NULL;
}



char* addr_from_file(struct sockaddr_in * local, FILE* config,int* length) {
    if(config == NULL) {
        return NULL;
    }
	//get len of file
	fseek(config,0,SEEK_END);
	unsigned int len = ftell(config);
	fseek(config,0,SEEK_SET);
    //read four byte of ip address
    uint32_t ip_address;
    fread(&ip_address,4,1,config);
    //read the port
    uint16_t port_net_order = 0;
	//read the remian part which is path
    fread(&port_net_order,2,1,config);
	char* path = (char*)malloc(len-6+1);
	memset(path,0,len-5);
	fread(path,len-6,1,config);
	// printf("%s",path);
	(*length) = len-6;

    fclose(config);
    local->sin_addr.s_addr = ip_address;
    //ip is ok now
    local->sin_family = AF_INET;
    //input port
    local->sin_port = port_net_order;

    return path;
}

int main(int argc, char** argv) {
	int serversocket_fd = -1;

	int clientsocket_fd = -1;
	struct threadqueue* q;
	struct idarray retrievetool;
	init_idarray(&retrievetool);
	//thread pool
	pthread_t threadIDs[MAXTHREAD]; 
	pthread_mutex_t qlock;
	q = init_queue(MAXTHREAD);
	pthread_mutex_init(&qlock,NULL);
	//memory pool
	struct connecter* input = (struct connecter*)malloc(sizeof(struct connecter)*MAXTHREAD); 
	memset(input,0,MAXTHREAD*sizeof(struct connecter));
	struct sockaddr_in address;
	int option = 1; 
	serversocket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(serversocket_fd < 0) {
		puts("This failed!");
		exit(1);
	}
    FILE* config = fopen(argv[1],"rb");
	int len;
	//init compress dict,init tree and read config
	
	dict = read_compress_dict();
	init_tree();
    char* path = addr_from_file(&address,config,&len);
	setsockopt(serversocket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(int));

	if(bind(serversocket_fd, (struct sockaddr*) &address, sizeof(struct sockaddr_in))) {
		puts("something wrong: port? ip-address?");
		exit(1);
	}
	listen(serversocket_fd, 128);
	int x;

	while(1) {
		uint32_t addrlen = sizeof(struct sockaddr_in);
		clientsocket_fd = accept(serversocket_fd, (struct sockaddr*) &(input[x].client), &addrlen);
		//create the shared memory
		x = -1;
		//stuck x;

		while(x == -1) {
			pthread_mutex_lock(&qlock);
			x = dequeue(q);
			pthread_mutex_unlock(&qlock);
		}
		input[x].qlock = &qlock;
		input[x].t_queue = q;
		input[x].used = x;
		input[x].ret = &retrievetool;
		input[x].clientsocket = clientsocket_fd;
		input[x].local_socket =serversocket_fd;
		input[x].path = path;
		input[x].path_len = &len;
		pthread_create(&threadIDs[x], NULL, connection_handler, &input[x]);
	}
	
	close(serversocket_fd);
	free(path);
	free(retrievetool.ids);
	free(input);
	return 0;
}