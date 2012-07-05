#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//エラー処理はめんどくさいので後回し。多分やらない。
//ASSERTだけ書いておく。

#define ASSERT(e) {\
	if(!(e)){\
		fprintf(stderr,"ASSERT:%s:%s/%s/%d\n",#e,__FILE__,__FUNCTION__,__LINE__);\
		exit(EXIT_FAILURE);\
	}\
}
#define SAFE_FREE(v){\
	ASSERT((v)!=NULL);\
	free(v);\
	v = NULL;\
}

typedef struct{
	int width;
	int height;
	int *data;
}BoardData;

int *BoardDataGetDataPtr(BoardData *this,int x,int y){
	int i = y*this->width+x;
	ASSERT( 0<=i && i< this->width*this->height);
	return &this->data[i];
}
int BoardDataGetData(BoardData *this,int x,int y){
	int *p = BoardDataGetDataPtr(this,x,y);
	return *p;
}
void BoardDataSetData(BoardData *this,int x,int y,int v){
	int *p = BoardDataGetDataPtr(this,x,y);
	*p = v;
}

void BoardDataInit(BoardData *this,int w,int h){
	ASSERT(w>=1 && h>=1);
	this->width = w;
	this->height = h;
	this->data = malloc(sizeof(int) * w*h);
}
void BoardDataInitWithLoadFile(BoardData *this,char *path){
	char buf[1000];
	char *p;
	int num;
	int w, h;
	int iy,ix;
	FILE *fp = fopen(path,"rb");
	ASSERT(fp!=NULL);
	p=fgets(buf,sizeof(buf),fp);
	ASSERT(p!=NULL);
	num = sscanf(p,"#%d %d",&h,&w);
	ASSERT(num==2);
	BoardDataInit(this,w,h);
	for(iy = 0;iy < h ; iy++){
		p = fgets(buf,sizeof(buf),fp);
		ASSERT(p!=NULL && strlen(p)>w);
		for(ix = 0;ix<w;ix++){
			char c = p[ix];
			int v = 0;
			if('0' <= c && c<= '9'){
				v = c - '0';
			}else if('a'<=c && c<='z'){
				v = 10 + c - 'a';
			}else{
				ASSERT(0);
			}
			BoardDataSetData(this,ix,iy,v);
		}
	}
	fclose(fp);
}
void BoardDataRelease(BoardData *this){
	SAFE_FREE(this->data);
}
void BoardDataPrint(BoardData *this){
	int iy;
	int ix;
	printf("   ");
	for(ix=0;ix<this->width;ix++){
		printf("%2d",ix);
	}
	printf("\n");
	for(iy=0;iy<this->height;iy++){
		printf("%2d ",iy);
		for(ix=0;ix<this->width;ix++){
			printf("%2d",BoardDataGetData(this,ix,iy));
		}
		printf("\n");
	}
}

typedef struct{
	int x;
	int y;
}Point;

int PointIsNull(Point *this){
	return this->x < 0;
}
void PointInit(Point *this){
	this->x = -1;
	this->y = 0;
}

typedef enum{
	CellColorGray = 0,
	CellColorWhite = 1,
	CellColorBlack = 2
}CellColor;

typedef struct{
	CellColor color;
	int group;

	Point head;
	int chainNum;
}Cell;

void CellInit(Cell *this){
	this->color = CellColorGray;
	//PointInit(&this->head);
	this->group = 0;
	this->chainNum = 1;
}

void CellGroupCopy(Cell *this,Cell *src){
	this->group = src->group;
	this->head = src->head;
	this->chainNum = this->chainNum + src->chainNum;
	src->chainNum = this->chainNum;
}

typedef struct{
	int width;
	int height;
	BoardData *data;
	Cell *cells;
	int nextGroup;
	//Group *groups;
}Board;

Cell *BoardGetCellPtr(Board *this,int x,int y){
	if(x<0 || this->width<=x || y<0 || this->height<=y)return NULL;
	return &this->cells[y*this->width+x];
}

void BoardCellChangeGroup(Board *this,int from,int to){
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->group == from){
				cell->group = to;
			}
		}
	}
}


//検査無し
void BoardCellSetColor(Board *this,int x,int y,CellColor color){
	Cell *cell = BoardGetCellPtr(this,x,y);
	cell->color = color;
	
	int found = 0;
	
	Cell *chk = BoardGetCellPtr(this,x-1,y);
	if(chk && chk->color==color){
		CellGroupCopy(cell,chk);
		found = 1;
	}
	
	chk = BoardGetCellPtr(this,x,y-1);
	if(chk && chk->color==color){
		CellGroupCopy(cell,chk);
		if(found){
			BoardCellChangeGroup(this,chk->group,cell->group);
		}else{
			
			found = 1;
		}
	}
	chk = BoardGetCellPtr(this,x+1,y);
	if(chk && chk->color==color){
		CellGroupCopy(cell,chk);
		if(found){
			BoardCellChangeGroup(this,chk->group,cell->group);
		}else{
			found = 1;		
		}
	}
	chk = BoardGetCellPtr(this,x,y+1);
	if(chk && chk->color==color){
		CellGroupCopy(cell,chk);
		if(found){
			BoardCellChangeGroup(this,chk->group,cell->group);
		}else{
			found = 1;		
		}
	}
	
	if(found==0){
		
		cell->group = this->nextGroup;
		cell->head.x = x;
		cell->head.y = y;
		cell->chainNum = 1;
		
		this->nextGroup ++;
	}
	
}

void BoardInitWithData(Board *this,BoardData *data){
	int iy,ix;
	this->width = data->width;
	this->height = data ->height;
	this->data = data;
	this->nextGroup = 1;
	this->cells = malloc(sizeof(Cell) * this->width * this->height);

	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			CellInit(cell);
			
			//確定塗り
			int v = BoardDataGetData(this->data,ix,iy);
			if(v>0){
				
				BoardCellSetColor(this,ix,iy,CellColorWhite);
				
			}
		}
	}
}
void BoardInitWithBoard(Board *this,Board *src){
	int iy,ix;
	this->width = src->width;
	this->height = src->height;
	this->data = src->data;
	this->cells = malloc(sizeof(Cell) * this->width * this->height);
	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *thisCell = BoardGetCellPtr(this,ix,iy);
			Cell *srcCell = BoardGetCellPtr(src,ix,iy);
			*thisCell = *srcCell;
		}
	}
}

void BoardRelease(Board *this){
	SAFE_FREE(this->cells);
}

void BoardPrint(Board *this){
	int iy,ix;
	printf("    ");
	for(ix=0;ix<this->width;ix++){
		printf("    %2d  |",ix);
	}
	printf("\n");
	for(iy=0;iy<this->height;iy++){
		printf("%2d |",iy);
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				printf("  (%2d,%2d)",cell->group,cell->chainNum);
			}else if(cell->color == CellColorWhite){
				int v = BoardDataGetData(this->data,ix,iy);
				if(v == 0){
					printf(" .(%2d,%2d)",cell->group,cell->chainNum);
				}else{
					printf("%2d(%2d,%2d)",v,cell->group,cell->chainNum);
				}
			}else{
				printf(" X(%2d,%2d)",cell->group,cell->chainNum);
			}
		}
		printf("\n");
	}
}

int main(int argc,char *argv[]){
	if(argc<2){
		printf("usage:%s file\n",argv[0]);
		return EXIT_SUCCESS;
	}
	BoardData bData;
	BoardDataInitWithLoadFile(&bData,argv[1]);
	BoardDataPrint(&bData);
	printf("\n");
	
	Board board;
	BoardInitWithData(&board,&bData);
	BoardPrint(&board);
	
	return EXIT_SUCCESS;
}

