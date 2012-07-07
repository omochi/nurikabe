#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

//エラー処理はめんどくさいので後回し。多分やらない。
//ASSERTだけ書いておく。
//配列操作は基本的にサイズ検査してない。

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


typedef struct{
	int x;
	int y;
}Point;

typedef enum{
	CellColorGray = 1,
	CellColorWhite = 2,
	CellColorBlack = 4
}CellColor;

typedef enum{
	BoardPrintChain = 1,
}BoardPrintFlag;

struct Cell_t;
typedef struct Cell_t Cell;

struct Cell_t{
	Point pos;//あーやっぱ必要になった。
	
	CellColor color;
	int group;
	
	Point head;
	int chainNum;
	
	int mark;
	int dist;
	Cell *markCell;
};

typedef struct{
	int width;
	int height;
	BoardData *data;
	Cell *cells;
	int nextGroup;
	
	int error;
	
	int depth;
	int grayCellNum;
	
	int printFlag;
}Board;

int silent = 0;
int solveStep= 0 ;
int solve();
void BoardInitWithBoard(Board *this,Board *src);
void BoardRelease(Board *this);
void BoardCellSetColor(Board *this,int x,int y,CellColor color);
void BoardIndent(Board *this);
void BoardPrint(Board *this);
void BoardPrintDist(Board *this);
void BoardPrintMark(Board *this);
void BoardPrintWithFlag(Board *this,int flags);
int BoardCellWhiteGroupRemain(Board *this,Cell *cell);
int BoardCellCheckThreeColor(Board *this,Point p0,Point p1,Point p2,int color);
void BoardGetCellAreaByColor(Board *this,Cell **result,int *resultLen,Cell *cell,int color);
void BoardCellAreaSetMarkByColor(Board *this,Cell *cell,int mark,int color);
int BoardGetCellsOfGroupAroundColor(Board *this,Cell **result,int *num,int group,int color);

int BoardGetCellsOfMarkAroundColor(Board *this,Cell **result,int *num,int mark,int color);

void BoardWhiteCellAreaMark(Board *this,Cell **result,int *num);

int BoardCellWhiteGroupRemain(Board *this,Cell *cell);
int BoardCalcGrayCellNum(Board *this);
int BoardGetScore(Board *this);
int BoardIsSame(Board *lhs,Board *rhs);
//ここまでヘッダ

Board *openNodes;
int openNodesLen=0;
int openNodesReserve=0;//メモリサイズ

Board *BoardArrayAlloc(int reserve){
	return malloc(sizeof(Board)*reserve);
}
/*
void BoardArrayReserve(Board *array,int *len,int *reserve){
}
 */
void BoardArrayInsert(Board *array,int *len,int *reserve,int idx,Board *add){
	ASSERT(*len+1<*reserve);
	int i;
	*len = *len+1;
	for(i=*len-1;i>idx;i--)array[i] = array[i-1];
	array[idx]=*add;
}
void BoardArrayRemove(Board *array,int *len,int *reserve,int idx){
	int i;
	*len=*len-1;
	for(i=idx;i<*len;i++)array[i]= array[i+1];
}
int BoardArrayFindByScore(Board *array,int *len,int score){
	int i;
	for(i=0;i<*len;i++){
		if(BoardGetScore(&array[i]) > score)return i;
	}
	return -1;
}
int BoardArrayFindByGrayNum(Board *array,int *len,int grayNum){
	int i;
	for(i=0;i<*len;i++){
		if(array[i].grayCellNum == grayNum)return i;
	}
	return -1;
}

int PointIsNull(Point *this){
	return this->x < 0;
}
void PointInit(Point *this){
	this->x = -1;
	this->y = 0;
}
int PointCmp(Point *this,Point *rhs){
	if(this->x < rhs->x){
		return -1;
	}else if(this->x > rhs->x){
		return 1;
	}else{
		
		if(this->y < rhs->y){
			return -1;
		}else if(this->y > rhs->y){
			return 1;
		}else{
			
			return 0;
		}
		
		
	}
	
}

Point PointMake(int x,int y){
	Point p;
	p.x = x;
	p.y = y;
	return p;
}


//nは更新される
void PointArrayAddUnique(Point *array,int *n,Point add){
	//同じ座標は追加しない
	int i;
	for(i=0;i<*n;i++){
		
		if(PointCmp(&array[i],&add)==0)return;
	}
	array[*n] = add;
	*n = *n + 1;
}

void GroupArrayAddUnique(int *array,int *n,int grp){
	int i;
	for(i=0;i<*n;i++){
		if(array[i] == grp)return;
	}
	array[*n] = grp;
	*n = *n +1;
}

void CellPtrArrayAddUniqueGroup(Cell **array,int *n,Cell *add){
	int i;
	for(i=0;i<*n;i++){
		if(array[i]->group == add->group)return;
	}
	array[*n] = add;
	*n = *n +1;
}

void CellPtrArrayAddUnique(Cell **array,int *n,Cell *add){
	int i;
	for(i=0;i<*n;i++)if(array[i] == add)return;
	array[*n]=add;
	*n = *n +1;
}

int CellPtrArrayFind(Cell **array,int *n,Cell *find){
	int i;
	for(i=0;i<*n;i++)if(array[i] == find)return i;
	return -1;
}

void CellPtrArrayRemove(Cell **array,int *n,int idx){
	int i;
	*n = *n -1;
	for(i=idx;i<*n;i++){
		array[i] = array[i+1];
	}
}

void CellPtrArrayInsert(Cell **array,int *n,int idx,Cell *val){
	int i;
	*n = *n +1;
	for(i=*n-1;i>idx;i--){
		array[i] = array[i-1];
	}
	array[idx] = val;
}

int CellPtrArrayFindByDist(Cell **array,int *n,int dist){
	int i;
	for(i=0;i<*n;i++)if(array[i]->dist >= dist)return i;
	return -1;
}

//caller should release
Point * BoardAllocPointArray(Board *this){
	Point *array = malloc(sizeof(Point) * this->width * this->height);
	return array;
}
int * BoardAllocGroupArray(Board *this){
	int *array = malloc(sizeof(int)*this->width*this->height);
	return array;
}

Cell **BoardAllocCellPtrArray(Board *this){
	Cell **array = malloc(sizeof(Cell*) * this->width * this->height);
	return array;
}



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
		//printf("[%s]\n",p);
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




void CellInit(Cell *this,int x,int y){
	this->pos = PointMake(x,y);
	this->head = PointMake(x,y);
	this->color = CellColorGray;

	this->group = 0;
	this->chainNum = 1;
	this->mark = 0;
	this->dist = 0;
	this->markCell = NULL;
}






Cell *BoardGetCellPtr(Board *this,int x,int y){
	if(x<0 || this->width<=x || y<0 || this->height<=y)return NULL;
	return &this->cells[y*this->width+x];
}

//変換
Cell *BoardGetThisCellPtr(Board *this,Cell *cell){
	if(cell==NULL)return NULL;
	return BoardGetCellPtr(this,cell->pos.x,cell->pos.y);
}

int BoardGetCellHeadData(Board *this,Cell *cell){
	return BoardDataGetData(this->data,cell->head.x,cell->head.y);	
}

int BoardCellIsCompletedWhite(Board *this,Cell *cell){
	int val = BoardGetCellHeadData(this,cell);
	if(val == 0)return 0;
	return cell->chainNum == val;
}

int BoardIsSame(Board *lhs,Board *rhs){
	int iy,ix;
	ASSERT(lhs->width == rhs->width && lhs->height == rhs->height);
	for(iy=0;iy<lhs->height;iy++){
		for(ix=0;ix<lhs->width;ix++){
			Cell *left = BoardGetCellPtr(lhs,ix,iy);
			Cell *right = BoardGetCellPtr(rhs,ix,iy);
			if(left->color != right->color)return 0;			
		}
	}
	return 1;
}

void BoardCellChangeGroup(Board *this,Cell *groupCell,int to,Point head, int chainNum){
	int group = groupCell->group;
	int iy,ix;
	
	
	if(group == 0){
		groupCell->group = to;
		groupCell ->head = head;
		groupCell -> chainNum = chainNum;
	//	return;
		group = to;
	}
	
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->group == group || cell->group == to){
				cell->group = to;
				cell->head = head;
				cell->chainNum = chainNum;
			}
		}
	}
}

void BoardCellGroupJoin(Board *this,Cell *lhs,Cell *rhs){
	ASSERT(lhs->color == rhs->color);
	ASSERT(lhs->color != CellColorGray);
	
	if(lhs->group == rhs->group){

		//BoardIndent(this);
		///printf("group same join(%d) grp=%d(%d,%d) x grp=%d(%d,%d)\n",lhs->color,
		//	   lhs->group,lhs->pos.x,lhs->pos.y,
		//	   rhs->group,rhs->pos.x,rhs->pos.y);
		//BoardPrint(this);

		return ;
	}
	
	int chainNum = lhs->chainNum + rhs->chainNum;
	
	if(lhs->color == CellColorBlack){
		
		
		BoardCellChangeGroup(this,lhs,rhs->group,rhs->head,chainNum);

	}else if(lhs->color == CellColorWhite){
		
		int leftValue = BoardGetCellHeadData(this,lhs);
		int rightValue = BoardGetCellHeadData(this,rhs);
		if(leftValue > 0 && rightValue ==0){
			
			BoardCellChangeGroup(this,rhs,lhs->group,lhs->head,chainNum);
		}else if(leftValue ==0 && rightValue > 0){
			
			BoardCellChangeGroup(this,lhs,rhs->group,rhs->head,chainNum);
			
		}else if(leftValue==0 && rightValue ==0){
			BoardCellChangeGroup(this,lhs,rhs->group,rhs->head,chainNum);
				
		}else{
			//違反連結
			this->error= 1;
			BoardIndent(this);
			printf("group join(%d) grp=%d(%d,%d) x grp=%d(%d,%d) error!\n",lhs->color,
				   lhs->group,lhs->head.x,lhs->head.y,
				   rhs->group,rhs->head.x,rhs->head.y);
			//BoardPrint(this);
		}
	}
	

}


void BoardCellSetBlackAroundWhite(Board *this,int group){
	//printf("black around white(grp=%d)\n",group);
	
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->group == group){
				
				Cell *blk;
				int x,y;
				
				x = ix - 1;
				y = iy;
				blk = BoardGetCellPtr(this,x,y);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,x,y,CellColorBlack);
				}

				x = ix + 1;
				y = iy;
				blk = BoardGetCellPtr(this,x,y);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,x,y,CellColorBlack);
				}
				
				x = ix;
				y = iy - 1;
				blk = BoardGetCellPtr(this,x,y);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,x,y,CellColorBlack);
				}
				
				x = ix;
				y = iy+1;
				blk = BoardGetCellPtr(this,x,y);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,x,y,CellColorBlack);
				}
				
				
			}
		}
	}
}


void BoardCellSetColor(Board *this,int x,int y,CellColor color){
	ASSERT(color!=CellColorGray);
	
	Cell *cell = BoardGetCellPtr(this,x,y);
	cell->color = color;

	int found = 0;
	
	Cell *chk = BoardGetCellPtr(this,x-1,y);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		found ++;
	}
	
	chk = BoardGetCellPtr(this,x,y-1);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		found ++;
	}
	chk = BoardGetCellPtr(this,x+1,y);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		found++;
	}
	chk = BoardGetCellPtr(this,x,y+1);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		found++;
	}
	
	
	if(found==0){
		cell->group = this->nextGroup;
		cell->chainNum = 1;
		
		this->nextGroup ++;
	}
			
	if(color == CellColorWhite){
		int value = BoardGetCellHeadData(this,cell);
		if(value > 0 ){
			
			if(cell->chainNum == value){
				//ホワイト塗り終わりの周囲黒確定
				BoardCellSetBlackAroundWhite(this,cell->group);
			}else if(cell->chainNum > value){
				this->error = 1;
				BoardIndent(this);
				printf("SetColor white chain over error! (%d,%d) chain=%d > %d\n",x,y,cell->chainNum,value);
				//BoardPrint(this);
				
			}
			
			
		}
		

	}else if(color==CellColorBlack){
		if(found>=2){
			//2x2を作ってないか
			if(BoardCellCheckThreeColor(this,PointMake(x-1,y),PointMake(x-1,y-1),PointMake(x,y-1),CellColorBlack) ||
			   BoardCellCheckThreeColor(this,PointMake(x,y-1),PointMake(x+1,y-1),PointMake(x+1,y),CellColorBlack) ||
			   BoardCellCheckThreeColor(this,PointMake(x-1,y),PointMake(x-1,y+1),PointMake(x,y+1),CellColorBlack) ||
			   BoardCellCheckThreeColor(this,PointMake(x,y+1),PointMake(x+1,y+1),PointMake(x+1,y),CellColorBlack)){
				
				this->error = 1;
				BoardIndent(this);
				printf("SetColor black 2x2 error! (%d,%d)\n",x,y);
				//BoardPrint(this);

			}
		}
	}

}

int BoardCellsAreSplitWhite(Board *this,Cell *lhs,Cell *rhs){
	//同じグループなら繋げて良い
	if(lhs->group == rhs->group)return 0;
	//違うグループでも片方親なしなら繋げて良い
	int leftValue = BoardGetCellHeadData(this,lhs);
	int rightValue = BoardGetCellHeadData(this,rhs);
	if(leftValue==0 || rightValue==0)return 0;
	return 1;
}

//ぬれた数
int BoardCellSetBlackSplitWhite(Board *this){
	int iy,ix;
	int num = 0;
	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				//白の分割が必要な部分なら				
				Cell *grpCell=NULL;
				
				Cell *chk;
				
				int x,y;
				
				x=ix-1;
				y=iy;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == CellColorWhite){
					if(grpCell==NULL){
						grpCell = chk;
					}else if(BoardCellsAreSplitWhite(this,grpCell,chk)){
						//分割の必要あり
						BoardCellSetColor(this,ix,iy,CellColorBlack);
						num ++ ;
						continue;
					}
				}
				
				x=ix+1;
				y=iy;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == CellColorWhite){
					if(grpCell==NULL){
						grpCell = chk;
					}else if(BoardCellsAreSplitWhite(this,grpCell,chk)){
						BoardCellSetColor(this,ix,iy,CellColorBlack);
						num++;
						continue;
					}
				}
				
				x=ix;
				y=iy-1;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == CellColorWhite){
					if(grpCell==NULL){
						grpCell = chk;
					}else if(BoardCellsAreSplitWhite(this,grpCell,chk)){
						BoardCellSetColor(this,ix,iy,CellColorBlack);
						num++;
						continue;
					}
				}
				
				x=ix;
				y=iy+1;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == CellColorWhite){
					if(grpCell==NULL){
						grpCell = chk;
					}else if(BoardCellsAreSplitWhite(this,grpCell,chk)){
						BoardCellSetColor(this,ix,iy,CellColorBlack);
						num++;
						continue;
					}
				}
				
				
				
			}
		}
	}
	return num;
	
}
	
int BoardCellCheckThreeColor(Board *this,Point p0,Point p1,Point p2,int color){
	//3点が存在して全てcolor
	Cell *cell;
	cell = BoardGetCellPtr(this,p0.x,p0.y);
	if(cell==NULL || cell->color != color)return 0;
	cell = BoardGetCellPtr(this,p1.x,p1.y);
	if(cell==NULL || cell->color != color)return 0;
	cell = BoardGetCellPtr(this,p2.x,p2.y);
	if(cell==NULL || cell->color != color)return 0;
	return 1;
}

int BoardCellSetWhiteByBlack2x2(Board *this){
	int iy,ix;
	int num = 0;
	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				//右下の白確定
				if(BoardCellCheckThreeColor(this,PointMake(ix-1,iy),PointMake(ix-1,iy-1),PointMake(ix,iy-1),CellColorBlack)){
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				//右上の白
				if(BoardCellCheckThreeColor(this,PointMake(ix-1,iy),PointMake(ix-1,iy+1),PointMake(ix,iy+1),CellColorBlack)){
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				
				//左下の白
				if(BoardCellCheckThreeColor(this,PointMake(ix,iy-1),PointMake(ix+1,iy-1),PointMake(ix+1,iy),CellColorBlack)){
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				//左上の白
				if(BoardCellCheckThreeColor(this,PointMake(ix,iy+1),PointMake(ix+1,iy+1),PointMake(ix+1,iy),CellColorBlack)){
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				
			}
		}
	}
	return num;
}

//隣接する色を再帰的にmarkして塗っていく
void BoardCellAreaSetMarkByColor(Board *this,Cell *cell,int mark,int color){
	if(cell->mark != 0)return;//すでに塗られている
	
	cell->mark = mark;
	Cell *chk;
	chk=BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y);
	if(chk && chk->color & color){
		BoardCellAreaSetMarkByColor(this,chk,mark,color);
	}
	chk=BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y);
	if(chk && chk->color & color){
		BoardCellAreaSetMarkByColor(this,chk,mark,color);
	}
	chk=BoardGetCellPtr(this,cell->pos.x,cell->pos.y-1);
	if(chk && chk->color & color){
		BoardCellAreaSetMarkByColor(this,chk,mark,color);
	}
	chk=BoardGetCellPtr(this,cell->pos.x,cell->pos.y+1);
	if(chk && chk->color & color){
		BoardCellAreaSetMarkByColor(this,chk,mark,color);
	}
	
}

//隣接するセルを色で再帰的に取る。グループ値は見ない。
//色はビットパターン
void BoardGetCellAreaByColor(Board *this,Cell **result,int *resultLen,Cell *cell,int color){
	if(CellPtrArrayFind(result,resultLen,cell)>=0){
		//すでに発見している		
		return ; 
	}
	//まだなら追加
	result[*resultLen] = cell;
	*resultLen = *resultLen + 1;
	
	Cell *chk;
	int x,y;
	
	x = cell->pos.x -1 ; y = cell->pos.y;
	chk = BoardGetCellPtr(this,x,y);
	if(chk && chk->color & color){
		//同じ色なので追加
		BoardGetCellAreaByColor(this,result,resultLen,chk,color);
	}
	
	x = cell->pos.x  ; y = cell->pos.y - 1;
	chk = BoardGetCellPtr(this,x,y);
	if(chk && chk->color & color){
		BoardGetCellAreaByColor(this,result,resultLen,chk,color);
	}	
	
	x = cell->pos.x +1; y = cell->pos.y;
	chk = BoardGetCellPtr(this,x,y);
	if(chk && chk->color & color){
		BoardGetCellAreaByColor(this,result,resultLen,chk,color);
	}	
	
	x = cell->pos.x  ; y = cell->pos.y +1;
	chk = BoardGetCellPtr(this,x,y);
	if(chk && chk->color & color){
		BoardGetCellAreaByColor(this,result,resultLen,chk,color);
	}	
}

//あるグループに隣接するある色の広がりを全部取る
void BoardGetCellAreaOfGroupAroundColor(Board *this,Cell **result,int *resultLen,int group,int color){
	*resultLen = 0;
	
	Cell **arounds = BoardAllocCellPtrArray(this);
	int aroundsLen = 0;
	
	BoardGetCellsOfGroupAroundColor(this,arounds,&aroundsLen,group,color);
	int i;
	for(i=0;i<aroundsLen;i++){
		//広げる
		BoardGetCellAreaByColor(this,result,resultLen,arounds[i],color);
	}
	
	SAFE_FREE(arounds);
}

int BoardCellIsValueWhite(Board *this,Cell *cell){
	return cell->color == CellColorWhite &&
	BoardGetCellHeadData(this,cell) != 0 &&
	PointCmp(&cell->pos,&cell->head)==0 ; 
}


//到達可能な数値セルを引っ張る
void BoardGetReachableWhiteHeadCells(Board *this,Cell **array,int *n,Cell *from){
	*n = 0;
	Cell **area = BoardAllocCellPtrArray(this);
	int areaLen = 0;
	
	BoardGetCellAreaByColor(this,area,&areaLen,from,CellColorGray | CellColorWhite);
	int i;
	for(i=0;i<areaLen;i++){
		Cell *cell = area[i];
		if(BoardCellIsValueWhite(this,cell)){
			CellPtrArrayAddUnique(array,n,cell);
		}
	}

	SAFE_FREE(area);	
}


//セルリストにその色の隣接があるか
int BoardCellsHasArroundColor(Board *this,Cell **array,int *n,CellColor color){
	int i;
	for(i=0;i<*n;i++){
		Cell *cell = array[i];
		Cell *chk;
		chk = BoardGetCellPtr(this,cell->pos.x - 1,cell->pos.y);
		if(chk && chk->color & color)return 1;
		chk = BoardGetCellPtr(this,cell->pos.x + 1,cell->pos.y);
		if(chk && chk->color & color)return 1;
		chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y -1);
		if(chk && chk->color & color)return 1;
		chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y + 1);
		if(chk && chk->color & color)return 1;
	}
	
	return 0;	
}

void BoardClearCellDist(Board *this){
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			cell->dist = 0;
		}
	}
}
void BoardClearCellDistWithMask(Board *this,int markMask){
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			
			if(cell->mark & markMask)continue;

			cell->dist = 0;
			
		}
	}
}

void BoardClearCellMark(Board *this){
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			cell->mark = 0;
			cell->markCell = NULL;
		}
	}
}

void BoardSetCellMarkUniqueGroup(Board *this,Cell *cell,Cell *mark){
	if(cell->markCell!=NULL && cell->markCell->group == mark->group){
		//すでにマークされていて、グループが同じならマークしない
	}else{
		//そうでなければ
		cell->markCell = mark;
		cell->mark ++;
	}
}

int BoardCellIsAroundAnotherValueWhite(Board *this,Cell *cell,Cell *group){
	int num = 0;
	Cell *chk;
	chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y);
	if(chk && chk->color == CellColorWhite && 
	   BoardGetCellHeadData(this,chk) != 0 &&
	   chk->group != group->group){
		num += 1;
	}
	chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y);
	if(chk && chk->color == CellColorWhite && 
	   BoardGetCellHeadData(this,chk) != 0 &&
	   chk->group != group->group){
		num+=2;
	}
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y-1);
	if(chk && chk->color == CellColorWhite && 
	   BoardGetCellHeadData(this,chk) != 0 &&
	   chk->group != group->group){
		num+=4;
	}
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y+1);
	if(chk && chk->color == CellColorWhite && 
	   BoardGetCellHeadData(this,chk) != 0 &&
	   chk->group != group->group){
		num+=8;
	}
	return num;
}

//内部関数
void BoardCalcCellDistanceAddToOpen(Board *this,Cell *start,int threshold,
									Cell **open,int *openLen,Cell *from,Cell *next){
	if(next == NULL ||
	   (next->color & (CellColorGray | CellColorWhite)) == 0 ||
	   next->dist != 0)return;
	
	//グループが違う数持ちの白と接触するならアウト
	int dir = BoardCellIsAroundAnotherValueWhite(this,next,start);
	if( dir >0){
		//BoardIndent(this);
		//printf("Another Value White Hit Cell (%d,%d/grp=%d) -- > (%d,%d) / %d\n",
		//	   start->pos.x,start->pos.y,start->group,next->pos.x,next->pos.y,dir);
		//BoardPrint(this);
		//getchar();
		return;
	}
	
	next->dist = from->dist + 1;
	BoardSetCellMarkUniqueGroup(this,next,start);
	
	if(next->dist >= threshold)return;
	
	int idx = CellPtrArrayFindByDist(open,openLen,next->dist+1);
	if(idx==-1)idx = *openLen;
	CellPtrArrayInsert(open,openLen,idx,next);

}

//ダイクストラ法
//スレッショルドより先は調べない
//到達セルにmark++を付ける
//ただしマークはグループ数だけつける
void BoardCalcCellDistance(Board *this,Cell *from,int threshold){
	from->dist = 1;
	BoardSetCellMarkUniqueGroup(this,from,from);
	
	Cell **open = BoardAllocCellPtrArray(this);
	int openLen  =0;
	
	CellPtrArrayInsert(open,&openLen,0,from);
	
	Cell *cell = NULL;
	Cell *chk = NULL;
	
	while(openLen > 0){
		cell = open[0];
		CellPtrArrayRemove(open,&openLen,0);
		
		chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y+0);
		BoardCalcCellDistanceAddToOpen(this,from,threshold,open,&openLen,cell,chk);

		chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y+0);
		BoardCalcCellDistanceAddToOpen(this,from,threshold,open,&openLen,cell,chk);
		
		chk = BoardGetCellPtr(this,cell->pos.x+0,cell->pos.y-1);
		BoardCalcCellDistanceAddToOpen(this,from,threshold,open,&openLen,cell,chk);
		
		chk = BoardGetCellPtr(this,cell->pos.x+0,cell->pos.y+1);
		BoardCalcCellDistanceAddToOpen(this,from,threshold,open,&openLen,cell,chk);
	}
	
	SAFE_FREE(open);
}

int BoardCellSetBlackSurroundedGray(Board *this){
	int iy,ix;
	int num = 0;
	//グレーの連結中に白を含まないものがあれば、全て黒で塗る
	//サイズが1で無ければ、2x2則に引っかかってエラーリターン

	Board test;
	BoardInitWithBoard(&test,this);
	for(iy = 0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(&test,ix,iy);
			if(cell)cell->mark = 0;
		}
	}
	
	Cell **cells = BoardAllocCellPtrArray(&test);
	int cellsLen=0;
	
	for(iy = 0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(&test,ix,iy);
			if(cell==NULL)continue;
			if(cell->mark == 0 && cell->color == CellColorGray){
				//塊を抽出
				cellsLen = 0;
				BoardGetCellAreaByColor(&test,cells,&cellsLen,cell,CellColorGray);				
				//取った塊を塗りつぶしつつ・・・
				int i;
				for(i=0;i<cellsLen;i++)cells[i]->mark = 1;
				
				//隣接の白を探す
				if(!BoardCellsHasArroundColor(&test,cells,&cellsLen,CellColorWhite)){
					
					for(i=0;i<cellsLen;i++){
						//ここでthisの方を塗る
						BoardCellSetColor(this,cells[i]->pos.x,cells[i]->pos.y,CellColorBlack);
						num++;
						if(this->error)goto forend;
					}
					
					
				}
				
				
			}
			
		}
	}

forend:
	SAFE_FREE(cells);
	
	BoardRelease(&test);

	return num;
}




//ある色のグループ一覧を取る
int BoardGetGroupsWithColor(Board *this,int *result,int *retNum,int color){
	int iy,ix;
	int num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == color){
				GroupArrayAddUnique(result,&num,cell->group);
			}
		}		
	}
	*retNum = num;
	return num;
}

int BoardGetGroupsByCellWithColor(Board *this,Cell **result,int *retNum,int color){
	int iy,ix;
	int num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == color){
				CellPtrArrayAddUniqueGroup(result,&num,cell);
			}
		}		
	}
	*retNum = num;
	return num;
}

int BoardGetCellsOfMarkAroundColor(Board *this,Cell **result,int *num,int mark,int color){
	int iy,ix;
	*num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->mark == mark){
				Cell *chk;
				
				chk = BoardGetCellPtr(this,ix-1,iy);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
				chk = BoardGetCellPtr(this,ix+1,iy);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
				chk = BoardGetCellPtr(this,ix,iy-1);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
				chk = BoardGetCellPtr(this,ix,iy+1);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
			}
		}
	}
	return *num;
}

int BoardGetCellsOfGroupAroundColor(Board *this,Cell **result,int *num,int group,int color){
	int iy,ix;
	*num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			//4方向足す
			if(cell->group == group){
				
				Cell *chk;
				
				chk = BoardGetCellPtr(this,ix-1,iy);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
				chk = BoardGetCellPtr(this,ix+1,iy);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
				chk = BoardGetCellPtr(this,ix,iy-1);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
				chk = BoardGetCellPtr(this,ix,iy+1);
				if(chk && chk->color & color){
					CellPtrArrayAddUnique(result,num,chk);
				}
								
			}
		}		
	}	
	return *num;
}


int BoardCellExpandWhite(Board *this){
	int num = 0;

	Cell **arounds = BoardAllocCellPtrArray(this);
	int aroundsLen = 0;

	Cell **area = BoardAllocCellPtrArray(this);
	int areaLen = 0;
	//白をマークで塗り分ける
	BoardWhiteCellAreaMark(this,area,&areaLen);

	
	int i;
	for(i=0;i<areaLen;i++){
		Cell *cell = area[i];
		//完成していたら黒に囲まれるので未完成
		BoardGetCellsOfMarkAroundColor(this,arounds,&aroundsLen,cell->mark,CellColorGray);
		
		//このマークグループの周りのグレー
		if(aroundsLen == 1){
			//確定伸ばし
			int x = arounds[0]->pos.x;
			int y = arounds[0]->pos.y;
		//	BoardIndent(this);
		//	printf("one way white (%d,%d) -> (%d,%d)\n",cell->pos.x,cell->pos.y,x,y);
		
			BoardCellSetColor(this,x,y,CellColorWhite);
			num++;
		}
		
	}
	
	BoardClearCellMark(this);
									   
	SAFE_FREE(area);
	SAFE_FREE(arounds);

	 
	return num;
}

//黒の連結
int BoardCellExpandBlack(Board *this){
	int num = 0;
	Cell **groups = BoardAllocCellPtrArray(this);
	int groupsLen = 0;
	Cell **arounds = BoardAllocCellPtrArray(this);
	int aroundsLen = 0;
	
	BoardGetGroupsByCellWithColor(this,groups,&groupsLen,CellColorBlack);
	int i;
	
	
	if(groupsLen>1){
		//グループが1つの場合は伸ばすべきと限らない
		
		for(i = 0; i< groupsLen; i++){
			Cell *grpCell = groups[i];

			BoardGetCellsOfGroupAroundColor(this,arounds,&aroundsLen,grpCell->group,CellColorGray);

			if(aroundsLen == 1){
				int x = arounds[0]->pos.x;
				int y = arounds[0]->pos.y;
				
				BoardCellSetColor(this,x,y,CellColorBlack);
				num++;
			}else if(aroundsLen == 0){
				//伸ばせないグループがあったら詰んでる
				this->error = 1;
				BoardIndent(this);
				printf("black split error! grp=%d (%d,%d)\n",grpCell->group , grpCell->head.x,grpCell->head.y);
				//BoardPrint(this);
				break;
				
			}
		}
	}
	
	SAFE_FREE(arounds);
	SAFE_FREE(groups);
	
	return num;
}

void BoardCheckBlackSplit(Board *this){
	int iy,ix;
	Cell **cells = BoardAllocCellPtrArray(this);
	int cellsLen =0;
	
	BoardClearCellMark(this);
	
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorBlack){
				BoardGetCellAreaByColor(this,cells,&cellsLen,cell,CellColorBlack | CellColorGray);
				int i;
				for(i=0;i<cellsLen;i++){
					Cell *reach = cells[i];
					reach -> mark = 1;
				}
				goto markend;
			}
		}
	}
markend:
	
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorBlack && cell->mark == 0){
				BoardIndent(this);
				printf("black split error  ! (%d,%d)\n",ix,iy);
				//BoardPrint(this);
				this->error = 1;
				goto checkend;
			}
		}
	}
checkend:
	
	BoardClearCellMark(this);
	
	SAFE_FREE(cells);

}

void BoardCheckWhiteSplit(Board *this){
	//飛び地グルーピングの到達性確認
	int iy,ix;
	int jy,jx;

	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *headCell = BoardGetCellPtr(this,ix,iy);
			if(BoardCellIsValueWhite(this,headCell)){
				BoardClearCellMark(this);
				BoardClearCellDist(this);
				
				BoardCalcCellDistance(this,headCell,BoardGetCellHeadData(this,headCell));

				for(jy=0;jy<this->height;jy++){
					for(jx=0;jx<this->width;jx++){
						Cell *cell = BoardGetCellPtr(this,jx,jy);
						if(cell->group == headCell->group &&
						   cell->mark == 0){
							
							BoardIndent(this);
							printf("cant reach self head (%d,%d) -> (%d,%d) error\n",
								   cell->pos.x,cell->pos.y,headCell->pos.x,headCell->pos.y);
							//BoardPrint(this);
							//BoardPrintDist(this);
							
							
							//getchar();
							
							this->error = 1;
							goto exit;
							
		
						}
					}
				}

			}
		}
	}
exit:
	return;
	
}

//白を塗り分ける
//塗り分け開始点を配列で返す
void BoardWhiteCellAreaMark(Board *this,Cell **result,int *num){
	
	*num = 0;
	int mark = 1;
	BoardClearCellMark(this);
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->mark == 0 && cell->color == CellColorWhite){
				BoardCellAreaSetMarkByColor(this,cell,mark,CellColorWhite);
				CellPtrArrayAddUnique(result,num,cell);
				mark ++;
			}
		}
	}
}


int BoardCellAreaExpandWhite(Board *this){
	int num = 0;
	
	Cell **groups = BoardAllocCellPtrArray(this);
	int groupsLen = 0;
	
	Cell **cells = BoardAllocCellPtrArray(this);
	int cellsLen = 0;	
	
	BoardGetGroupsByCellWithColor(this,groups,&groupsLen,CellColorWhite);
	int i;
	for(i=0;i<groupsLen;i++){
		Cell *grpCell = groups[i];
		cellsLen = 0;
		int value = BoardGetCellHeadData(this,grpCell);
		if(value ==0)continue;
		
	
		
		int rem = value - grpCell->chainNum;
		if(rem == 0)continue;
		//グループの周りの白と灰色
		//白なのでグループ自身もカウントされる
		BoardGetCellAreaOfGroupAroundColor(this,cells,&cellsLen,grpCell->group,CellColorGray | CellColorWhite);
		
		if(cellsLen == value){
			//充填確定
			//BoardIndent(this);
			//printf("white cell area fixed! num=%d , grp=%d(%d,%d)\n",cellsLen,grpCell->group,grpCell->pos.x,grpCell->pos.y);
			//BoardPrint(this);

			int ci;
			for(ci = 0; ci<cellsLen;ci++){
				BoardCellSetColor(this,cells[ci]->pos.x,cells[ci]->pos.y,CellColorWhite);
			}
			num += cellsLen;
			
		}else if(cellsLen < value){
			//不足確定
			BoardIndent(this);
			printf("white cell area is lacking!! value=%d , area=%d , grp=%d(%d,%d)\n",
				   value,cellsLen,grpCell->group,grpCell->pos.x,grpCell->pos.y);
			//BoardPrint(this);
			
			this->error = 1;
			break;
			
		}
	
	}
	
	SAFE_FREE(cells);
	SAFE_FREE(groups);
	return num;
}

//距離マップを戻りながら到達できるセルを全てマークする
//1のところに来たら終わり
void BoardCellSetMarkBackDist(Board *this,Cell *cell,int mark){
	if(cell->mark != 0)return;

	cell->mark |= mark;
	
	if(cell->dist == 1)return;
	
	Cell *chk;
	chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y);
	if(chk && chk->dist ==cell->dist - 1)BoardCellSetMarkBackDist(this,chk,mark);
	chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y);
	if(chk && chk->dist ==cell->dist - 1)BoardCellSetMarkBackDist(this,chk,mark);
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y-1);
	if(chk && chk->dist ==cell->dist - 1)BoardCellSetMarkBackDist(this,chk,mark);
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y+1);
	if(chk && chk->dist ==cell->dist - 1)BoardCellSetMarkBackDist(this,chk,mark);
}

int BoardCellSetWhiteByDistBack(Board *this,Cell *from,Cell *to){
	int num = 0;
	Board view;
	BoardInitWithBoard(&view,this);
	Cell *thisFrom = from;
	from = BoardGetThisCellPtr(&view,from);
	to = BoardGetThisCellPtr(&view,to);
	

	
	BoardClearCellDist(&view);
	BoardClearCellMark(&view);
	
	BoardCalcCellDistance(&view,from,BoardGetCellHeadData(&view,from));
	BoardClearCellMark(&view);
	ASSERT(to->dist>1 && from->dist==1);

	BoardCellSetMarkBackDist(&view,to,1);
	BoardClearCellDistWithMask(&view,1);
	
	int *distNum = calloc((to->dist+1) , sizeof(int));
	
	int iy,ix;
	for(iy=0;iy<view.height;iy++){
		for(ix=0;ix<view.width;ix++){
			Cell *cell = BoardGetCellPtr(&view,ix,iy);
			if(cell->dist > 1){
				//1のところは出発点なので不要
				distNum[cell->dist]++;
			}
		}
	}
	
	//1つしか出てない奴をぬる
	for(iy=0;iy<view.height;iy++){
		for(ix=0;ix<view.width;ix++){
			Cell *cell = BoardGetCellPtr(&view,ix,iy);
			if(cell->dist > 0 && distNum[cell->dist]==1){
				//確定セル
				Cell *thisCell = BoardGetThisCellPtr(this,cell);
		

				
				if(thisCell->color == CellColorGray){
					BoardCellSetColor(this,thisCell->pos.x,thisCell->pos.y,CellColorWhite);
				}else if(thisCell->color == CellColorWhite &&
						 thisCell->group != thisFrom->group){
					BoardCellChangeGroup(this,thisCell,thisFrom->group,thisFrom->head,thisCell->chainNum + thisFrom->chainNum);
				}else{
					continue;
				}
				
				//BoardIndent(this);
				//printf("fixed white by dist back (%d,%d/grp=%d/ch=%d) -> (%d,%d/dist=%d/ch=%d)\n",
			//		   thisFrom->pos.x,thisFrom->pos.y,thisFrom->group,thisFrom->chainNum,
		//			   ix,iy,thisCell->dist,thisCell->chainNum);	
				num++;
				
		
			}
		}
	}
	if(num>0){
		//BoardPrintDist(&view);
		//BoardPrintWithFlag(this,BoardPrintChain);
		//getchar();
	}


	SAFE_FREE(distNum);

	BoardRelease(&view);
	return num;
}

int BoardCellSetBlackUnreachable(Board *this){
	int num = 0;
	
	BoardClearCellDist(this);
	BoardClearCellMark(this);
	
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
						
			if(cell->color == CellColorWhite){
				int value = BoardGetCellHeadData(this,cell);
				if(value != 0){
					int rem = value - cell->chainNum;
					
					//数字と対応した白セル全てから距離を伸ばす
					//内部で到着先にマークを足す
					BoardClearCellDist(this);
					BoardCalcCellDistance(this,cell,1+rem);
				}
			}
			 
		}
	}
	
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray && cell->mark ==0){
				
				BoardCellSetColor(this,ix,iy,CellColorBlack);
				num ++ ;
			}else if(cell->color == CellColorWhite && BoardGetCellHeadData(this,cell)==0){
				if(cell->mark == 0){
					this->error = 1;
					BoardIndent(this);
					printf("unreachable no value white cell (%d,%d)\n",ix,iy);
					//BoardPrint(this);
					//getchar();
					goto endchk;
				}else if(cell->mark == 1){
					//到達グループが一つしかないなら！
					Cell *markCell = cell->markCell;
					ASSERT(markCell!=NULL);
					
					num += BoardCellSetWhiteByDistBack(this,markCell,cell);
					
					goto endchk;
				}
			}
		}
	}
endchk:
		
	BoardClearCellDist(this);
	BoardClearCellMark(this);
	
	return num;
}


void BoardInitWithData(Board *this,BoardData *data){
	int iy,ix;
	this->width = data->width;
	this->height = data ->height;
	this->data = data;
	this->nextGroup = 1;
	this->error = 0;
	this->depth = 0;
	this->cells = malloc(sizeof(Cell) * this->width * this->height);

	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			CellInit(cell,ix,iy);
		}
	}
	
	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			//数字セルは白確定する
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
	this->nextGroup = src->nextGroup;
	this->data = src->data;
	this->error = src->error;
	this->depth = src->depth;
	this->printFlag = src->printFlag;
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

void BoardIndent(Board *this){
	int i;
	for(i=0;i<this->depth;i++){
		if(i == this->depth -1 ){
			putchar('>');
		}else{
			putchar('-');
		}
			
	}
}



void BoardPrintWithFlag(Board *this,int flags){
	int iy,ix;
	
	if(silent)return;
	
	
	BoardIndent(this);
	printf("    |");
	for(ix=0;ix<this->width;ix++){
		if(flags & BoardPrintChain){
			printf("     %2d   |",ix);
		}else{
			printf("   %2d |",ix);
		}
	}
	printf("\n");

	if(flags & BoardPrintChain){

		for(iy=0;iy<this->height;iy++){
			BoardIndent(this);
			printf(" %2d |",iy);
			for(ix=0;ix<this->width;ix++){
				Cell *cell = BoardGetCellPtr(this,ix,iy);
				if(cell->color == CellColorGray){
					printf("  (%3d,%3d)",cell->group,cell->chainNum);
				}else if(cell->color == CellColorWhite){
				//	if(BoardCellIsCompletedWhite(this,cell)){
				//		printf("%2d(%3d,%3d)",cell->chainNum,cell->group,cell->chainNum);
				//	}else{
						int data = BoardDataGetData(this->data,ix,iy);
						int value = BoardGetCellHeadData(this,cell);
						if(data != 0){
							printf("%2d(%3d,%3d)",data,cell->group,cell->chainNum);
							
						}else if(value != 0){
							printf(" +(%3d,%3d)",cell->group,cell->chainNum);
							
						}else{
							printf(" .(%3d,%3d)",cell->group,cell->chainNum);
						}

					//}
				}else{
					printf(" X(%3d,%3d)",cell->group,cell->chainNum);
				}
			}
			printf("\n");
		}
	
	}else{
	
		for(iy=0;iy<this->height;iy++){
			BoardIndent(this);
			printf(" %2d |",iy);
			for(ix=0;ix<this->width;ix++){
				Cell *cell = BoardGetCellPtr(this,ix,iy);
				if(cell->color == CellColorGray){
					printf("  (%3d)",cell->group);
				}else if(cell->color == CellColorWhite){
					/*
					if(BoardCellIsCompletedWhite(this,cell)){
						printf("%2d(%3d)",cell->chainNum,cell->group);
					}else{
					*/
					 int data = BoardDataGetData(this->data,ix,iy);
						int value = BoardGetCellHeadData(this,cell);
						if(data != 0){
							printf("%2d(%3d)",data,cell->group);
							
						}else if(value != 0){
							printf(" +(%3d)",cell->group);
							
						}else{
							printf(" .(%3d)",cell->group);
						}
					//}
				}else{
					printf(" X(%3d)",cell->group);
				}
			}
			printf("\n");
		}
	}
	
	BoardIndent(this);
	printf("--[%d]--\n",solveStep);
}

void BoardPrint(Board *this){
	BoardPrintWithFlag(this,this->printFlag);
}

void BoardPrintDist(Board *this){
	int iy,ix;

	BoardIndent(this);
	printf("    |");
	for(ix=0;ix<this->width;ix++){
		printf("   %2d |",ix);
	}
	printf("\n");
	
	
	for(iy=0;iy<this->height;iy++){
		BoardIndent(this);
		printf(" %2d |",iy);
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				printf("  (%3d)",cell->dist);
			}else if(cell->color == CellColorWhite){

				int v = BoardDataGetData(this->data,ix,iy);
				if(v == 0){
					printf(" .(%3d)",cell->dist);
				}else{
					printf("%2d(%3d)",v,cell->dist);
				}
			
			}else{
				printf(" X(   )");
			}
		}
		printf("\n");
	}

}

void BoardPrintMark(Board *this){
	int iy,ix;
	
	BoardIndent(this);
	printf("    |");
	for(ix=0;ix<this->width;ix++){
		printf("   %2d |",ix);
	}
	printf("\n");
	
	
	for(iy=0;iy<this->height;iy++){
		BoardIndent(this);
		printf(" %2d |",iy);
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				printf("  (%3d)",cell->mark);
			}else if(cell->color == CellColorWhite){
				
				int v = BoardDataGetData(this->data,ix,iy);
				if(v == 0){
					printf(" .(%3d)",cell->mark);
				}else{
					printf("%2d(%3d)",v,cell->mark);
				}
				
			}else{
				printf(" X(   )");
			}
		}
		printf("\n");
	}
}

int BoardIsSolved(Board *this){
	int iy,ix;
	
	int blackGroup = 0;
	
	//BoardPrint(this);
	
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			int value = BoardGetCellHeadData(this,cell);
			
			if(cell->color ==CellColorGray){
				//BoardIndent(this);
				//printf("solved check : there is gray(%d,%d)\n",ix,iy);
				//塗り終わっていない
				return 0;
			}else if(cell->color == CellColorWhite){
				//白の長さが正しい
				if(cell->chainNum != value){
					//BoardIndent(this);
					//printf("solved check : white num chain=%d != %d (%d,%d)=>(%d,%d)\n",cell->chainNum,value,ix,iy,
					//	   cell->head.x,cell->head.y);
					return 0;
				}
			}else if(cell->color ==CellColorBlack){
				//黒が1連結
				if(blackGroup == 0){
					blackGroup = cell->group;
				}else{
					if(blackGroup != cell->group){
					//	BoardIndent(this);
					//	printf("solved check : black group over\n");
						return 0;
					}
				}
			}
		}
	}
	
	//BoardPrint(this);
	
	return 1;
}

int BoardCalcGrayCellNum(Board *this){
	int num = 0;
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				num++;	
			}
		}
	}
	this->grayCellNum = num;
	return num;
}

int BoardCellWhiteGroupRemain(Board *this,Cell *cell){
	return BoardGetCellHeadData(this,cell) - cell->chainNum;
}

int BoardGetScore(Board *this){
	if(this->grayCellNum == 0)return 0;
	//return this->grayCellNum * this->depth;
	return this->grayCellNum + this->depth * 5;
}

int BoardFindNextBranchGroup(Board *this){
	//次に探索するとよさそうなグループを選ぶ
	//残り必要長の小さいグループにする。
	//ただし、数字なし部分からは伸ばさない
	int remain = INT_MAX;
	Cell *choice = NULL;
	

	Cell **groups = BoardAllocCellPtrArray(this);
	int groupsLen = 0;

	BoardGetGroupsByCellWithColor(this,groups,&groupsLen,CellColorWhite);

	int i;
	for(i=0;i<groupsLen;i++){
			
		Cell *grpCell = groups[i];
		int value = BoardGetCellHeadData(this,grpCell);
		if(value == 0)continue;//数字なしグループは拒否
		
		int cellRemain = value - grpCell->chainNum;
		//ASSERT(0<=cellRemain);
		if(cellRemain == 0)continue;//完成しているところは伸ばさない
		
		if(cellRemain < remain){
			remain = cellRemain;
			choice = grpCell;
		}		
	}
	
	SAFE_FREE(groups);
	
	if(choice==NULL){
		if(groupsLen>0){
			//ここには来ないはず・・・
			BoardIndent(this);
			printf("no number only\n");
			BoardPrint(this);
			getchar();
		}
		
		this->error = 1;
		return 0;
	}
	
	return choice->group;
}


//0:fail 1:ok
int solveSingle(Board *board){
	//固定計算を進める
	while(1){
		int num= 0;
		num = BoardCellSetBlackSplitWhite(board);
		if(board->error)return 0;
		if(num > 0)continue;
		
		
		num = BoardCellSetWhiteByBlack2x2(board);
		if(board->error)return 0;
		if(num > 0)continue;
		
		num = BoardCellExpandWhite(board);
		if(board->error)return 0;
		if(num > 0)continue;
		
		
		num = BoardCellExpandBlack(board);
		if(board->error)return 0;
		if(num > 0)continue;
		
		num = BoardCellAreaExpandWhite(board);
		if(board->error)return 0;
		if(num>0)continue;
		
		
		num = BoardCellSetBlackUnreachable(board);
		if(board->error)return 0;
		if(num>0)continue;
		
		
		BoardCheckBlackSplit(board);
		if(board->error)return 0;
		
		BoardCheckWhiteSplit(board);
		if(board->error)return 0;
		
		BoardCalcGrayCellNum(board);
		
		break;
	}
	return 1;
}

void solveAddToOpen(Board *node){
	int idx;
	int score = BoardGetScore(node);
	idx= BoardArrayFindByGrayNum(openNodes,&openNodesLen,node->grayCellNum);
	if(idx>=0){
		//同じ盤面か？
		if(BoardIsSame(node,&openNodes[idx])){
			
			int openScore = BoardGetScore(&openNodes[idx]);
			//printf("same board ! new=%d , open=%d\n",score,openScore);	
			if(openScore <= score){
				BoardRelease(node);
				//すでにある奴の方がいいなら追加しない
				return;
			}else{
				//こっちのほうがいいなら今のを消す
				Board opened = openNodes[idx];
				BoardArrayRemove(openNodes,&openNodesLen,&openNodesReserve,idx);
				BoardRelease(&opened);
			}
			
		}
	}
	
	
	idx = BoardArrayFindByScore(openNodes,&openNodesLen,score);
	if(idx==-1){
		idx = openNodesLen;
	}
	BoardArrayInsert(openNodes,&openNodesLen,&openNodesReserve,idx,node);
	
}

void solveBranch(Board *board){
	int ret = 0;

	int group = BoardFindNextBranchGroup(board);
	if(group == 0){
		BoardIndent(board);
		printf("no next branch group\n");
		return;	
	}

	Cell **cells = BoardAllocCellPtrArray(board);
	int cellsLen = 0;
		
	BoardGetCellsOfGroupAroundColor(board,cells,&cellsLen,group,CellColorGray);

	BoardIndent(board);
	printf("branch group=%d:branch num:%d\n",group,cellsLen);
	BoardPrint(board);
	
	ASSERT(cellsLen != 1);//1なら事前にぬったはず
	if(cellsLen==0){
		//詰んでる
		SAFE_FREE(cells);
		return;
	}
	
	//ここで選択肢が出なかった場合はそのままret = 0を返却する
	int i;
	for(i=0;i<cellsLen;i++){
		//この中のどれかが正解であるはず。全部だめなら問題が解無し。
		
		//複製
		Board next;
		BoardInitWithBoard(&next,board);
		next.depth++;
		
		int x = cells[i]->pos.x;
		int y = cells[i]->pos.y;
		
		BoardIndent(board);
		printf("branch[%d/%d]  group=%d -> (%d,%d)\n",i,cellsLen,group,x,y);
		BoardCellSetColor(&next,x,y,CellColorWhite);
		//BoardPrint(&next);
		//ret = solve(&next);
		
		ret = solveSingle(&next);
		if(ret == 0){
			//すでにおかしいノードは切り捨て
		}else{
			//追加
			
			solveAddToOpen(&next);
				
		}
		
		//BoardRelease(&next);
		
		/*
		if(ret == 0){
			//白が違ったなら黒。確定して次を試す。
			BoardIndent(board);
			printf("white fail -> black (%d,%d)\n",x,y);
			BoardCellSetColor(board,x,y,CellColorBlack);
			
		}else{
			//成功を返却する
			SAFE_FREE(cells);
			return 1;
		}
		 */
	}
	
	/*
	//黒を伸ばしてみる
	Cell **groups = BoardAllocCellPtrArray(board);
	int groupsLen = 0;
	BoardGetGroupsByCellWithColor(board,groups,&groupsLen,CellColorBlack);
	for(i=0;i<groupsLen;i++){
		Cell *grpCell = groups[i];
		
		BoardGetGroupAroundColor(board,points,&pointsLen,grpCell->group,CellColorGray);
		printf("depth %d:black expand branch:group=%d\n",depth,grpCell->group);
		
		int j;
		for(j=0;j<pointsLen;j++){
			Board next;
			BoardInitWithBoard(&next,board);
	 		next.depth++;
			int x = points[j].x;
			int y = points[j].y;
			BoardCellSetColor(&next,x,y,CellColorBlack);
			ret = solve(&next,depth+1);
			BoardRelease(&next);
			
			if(ret == 0){
				
			}else{
				SAFE_FREE(points);
				SAFE_FREE(groups);
				return 1;
			}
			
			
		}
		
	
	}
	SAFE_FREE(groups);
	 */
	SAFE_FREE(cells);
}



//0:fail 1:solved
int solve(){
	int solved = 0;
	
	while(openNodesLen>0){
		solveStep ++ ;
		
		int i;
		
		printf("step:%d , openNodeList(%d):",solveStep,openNodesLen);
		for(i=0;i<openNodesLen;i++){
			printf("%d+%d,",openNodes[i].grayCellNum,openNodes[i].depth);
			if(i>100){
				printf("...");
				break;
			}
		}
		printf("\n");
		//getchar();
	
		Board board = openNodes[0];
		BoardArrayRemove(openNodes,&openNodesLen,&openNodesReserve,0);
		
		if(BoardIsSolved(&board)){
			BoardIndent(&board);
			printf("solved\n");
			BoardPrint(&board);
			BoardRelease(&board);
			solved = 1;
			break;
		}

		
		

		
		//以下分岐処理
		solveBranch(&board);
		
		BoardRelease(&board);
	}
	
	while(openNodesLen>0){
		//解放
		Board board = openNodes[0];
		BoardArrayRemove(openNodes,&openNodesLen,&openNodesReserve,0);
		BoardRelease(&board);
	}
	
	
	return solved;
	
}

int main(int argc,char *argv[]){
	if(argc<2){
		printf("usage:%s file\n",argv[0]);
		return EXIT_SUCCESS;
	}
	
	int opt;
	while ((opt = getopt(argc, argv, "s")) != -1) {
        switch (opt) {
			case 's':
				silent = 1;
				break;
        }
    }
	
	openNodesReserve=100000;
	openNodes = BoardArrayAlloc(openNodesReserve);
	openNodesLen = 0;
	
	BoardData bData;
	BoardDataInitWithLoadFile(&bData,argv[optind]);

	printf("\n");
	
	Board board;
	BoardInitWithData(&board,&bData);
	board.printFlag = 0;

	solveStep = 0;
	
	int ret = solveSingle(&board);
	if(ret){
		BoardArrayInsert(openNodes,&openNodesLen,&openNodesReserve,0,&board);
	}
	
	ret = solve(&board);
	
	printf("solve step : %d\n",solveStep);

	if(ret){

	}else{
		printf("solve failed\n");
	}


	SAFE_FREE(openNodes);
	
	return EXIT_SUCCESS;
}

