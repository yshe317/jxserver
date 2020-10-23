#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
uint8_t ** dict;
huff_tree tree;

uint8_t** read_compress_dict() {
	FILE* f = fopen("compression.dict","rb");
	uint8_t** b = (uint8_t**)malloc(sizeof(uint8_t*) * 256);
	memset(b,0,256*sizeof(uint8_t*));
	int count = 0;
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

        b[i] = (uint8_t*)malloc(9);
		memset(b[i],0,9);
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
    return b;
}
uint8_t* compress_data(uint8_t* uncompress,size_t length,uint64_t* size) {
	//need to add padding
	uint8_t cursor = 0;
	uint8_t* result = NULL;
	int len = 0;
	int index = 0;
	int all_bits = 0;
	for(int i = 0;i <length;i++) {
        // // printf("so far the length is %d",i);
		// printf("so far the bits is %x\n",uncompress[i]);
		// printf("so far the bits is %x\n",dict[190][0]);
		uint8_t* temp =  dict[uncompress[i]];
        //printf("so far the bits is %d\n",temp[0]);
		// // how many bits it have;
		uint8_t lenoftemp_in_bit = temp[0];
		all_bits += lenoftemp_in_bit;
		uint8_t lenoftemp_in_byte = lenoftemp_in_bit/8;
		uint8_t remain = all_bits % 8;
		if(remain != 0) {
			len = all_bits/8 +1;
		}else{
			//remain == 0;
			len = all_bits/8;
		}


        //printf("so far the bits is %d\n",lenoftemp_in_bit);
        // len += lenoftemp_in_byte;
		result = (uint8_t*)realloc(result,len);

		// int remain = lenoftemp_in_bit%8;

		for (int j = 1; j <lenoftemp_in_byte+1; j++) {
			if(cursor == 0) {
				result[index] = temp[j];
				index++;
			}else{ // maybe will not attach 8
				result[index] += temp[j]>>cursor;
				index++;
				result[index] = temp[j]<< (8-cursor);
			}

        }
		// printf("the index is %d\n",index);
		// printf("the len is %d\n",len);
		//printf("the cursor and remain is %d and %d\n",cursor,remain);
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
			// printf("the half temp %x\n",temp[lenoftemp_in_byte+1]);
			// printf("the half result %x\n",result[index]);
			cursor = remain;
        }else{
			//printf("the cursor and remain is %d and %d\n",cursor,remain);
			result[index] += temp[lenoftemp_in_byte+1] >> cursor;
			index++;
			result[index] = temp[lenoftemp_in_byte+1] << (8-cursor);
			cursor = remain;
			//result[index] = (result[index] >>(8-cursor))<<(8-cursor);
		}
		if(cursor == 0) {
			index = len;
		}else{
			result[index] = (result[index] >>(8-cursor))<<(8-cursor);
		}
	}

	//printf("the len is %d\n",len);
	len=len+1;//to hold padding
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

int readfile() {
    FILE* f = fopen("compression.dict","rb");
    uint8_t buffer;
    for(int i = 0;i<10;i++) {
        fread(&buffer,1,1,f);
        printf("%x",buffer);
    }
    fclose(f);
    return 0;
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
		printf("%x",getbit(compressed,i));
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
	(*size) = len;
	return result;
}

int main() {
    unsigned int a;
    a = 8;
    dict = read_compress_dict();
    //readfile();
	init_tree();

	//printf("%x\n",tree.root.left->right->left->right->left->left->left->left->left->source);

    uint8_t* un = malloc(10);
    for(int i =0 ;i <10; i++) {
        un[i] = i;
    }

    uint64_t size = 0;
	uint8_t* s = compress_data(un,3,&size);
	printf("\n");
	for(int i = 0;i<size;i++) {
		printf("%x",s[i]);
	}
	printf("\n"); 
 	uint8_t* t = decode_data(s, size, &size);
	// for(int i = 0;i<size;i++) {
	// 	printf("%x",t[i]);
	// }
	//printf("\n%llu",size);
}

