#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _OMMem OMMem;
struct _OMMem{
	OMMem *next;
	OMMem *prev;
	char tag[16];
	int size;
	char buf[0];
};

extern OMMem *OMMemChainHead;
extern OMMem *OMMemChainTail;

#define OMSAFE_FREE(ptr){\
OMFree(ptr);\
ptr=NULL;\
}


OMMem *OMMemAlloc(int size);
OMMem *OMMemFromPtr(void *ptr);
void *OMMemGetPtr(OMMem *this);
void OMMemChainAdd(OMMem *this);
void OMMemChainRemove(OMMem *this);
void OMMemChainPrint();

void *OMAlloc(int size,char *tag);
void OMFree(void *ptr);

