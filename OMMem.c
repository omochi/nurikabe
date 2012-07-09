#include "OMMem.h"


OMMem *OMMemChainHead=NULL;
OMMem *OMMemChainTail=NULL;

OMMem *OMMemAlloc(int size){
	OMMem *mem = calloc(1,sizeof(OMMem) + size);
	return mem;
}
OMMem *OMMemFromPtr(void *ptr){
	char *p = (char *)ptr;
	return (OMMem*)(p - offsetof(OMMem,buf));
}
void *OMMemGetPtr(OMMem *this){
	return (void *)(this->buf);
}

void OMMemChainAdd(OMMem *this){
	this->prev = OMMemChainTail;
	this->next = NULL;
	if(OMMemChainTail!=NULL){
		OMMemChainTail->next = this;
	}
	if(OMMemChainHead==NULL)OMMemChainHead = this;
	OMMemChainTail = this;
}
void OMMemChainRemove(OMMem *this){
	if(this->prev != NULL)this->prev->next = this->next;
	if(this->next != NULL)this->next->prev = this->prev;
	
	if(OMMemChainHead == this)OMMemChainHead = this->next;
	if(OMMemChainTail == this)OMMemChainTail = this->prev;
}

void OMMemChainPrint(){
	int i = 0;
	int size = 0;
	printf("-->>OMMemChainPrint\n");
	OMMem *mem = OMMemChainHead;
	while(mem!=NULL){
		printf("OMMem[%3d]:ptr=%p,size=%4d,tag=%s\n",
			   i,
			   mem->buf,
			   mem->size,
			   mem->tag);
		size += mem->size;
		mem = mem->next;
		i++;
	}
	printf("--<<OMMemChainPrint,size = %d\n",size);
}

void *OMAlloc(int size,char *tag){
	
	if(size<=0){
		fprintf(stderr,"OMAlloc failed . size = %d\n",size);
		exit(EXIT_FAILURE);
	}
	OMMem *mem = OMMemAlloc(size);
	if(mem==NULL){
		fprintf(stderr,"OMAlloc failed . size = %d\n",size);
		exit(EXIT_FAILURE);
	}
	strncat(mem->tag,tag,sizeof(mem->tag));
	mem->size = size;
	OMMemChainAdd(mem);
	return OMMemGetPtr(mem);
}

void OMFree(void *ptr){
	if(ptr==NULL){
		fprintf(stderr,"OMFree ptr is null\n");
		exit(EXIT_FAILURE);
	}
	OMMem *mem = OMMemFromPtr(ptr);
	OMMemChainRemove(mem);
	free(mem);
}
