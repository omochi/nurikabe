#include "main.h"

int silent = 0;
int solveStep= 0 ;
//ここまでヘッダ

Board *openNodes;
int openNodesLen=0;
int openNodesReserve=0;//メモリサイズ

Board *BoardArrayAlloc(int reserve){
	return OMAlloc(sizeof(Board)*reserve,"BoardArray");
}

void BoardArrayInsert(Board *array,int *len,int *reserve,int idx,Board *add){
	ASSERT(*len+1<*reserve);
	int i;
	*len = *len+1;
	for(i=*len-1;i>idx;i--)array[i] = array[i-1];
	array[idx]=*add;
}
void BoardArrayRemove(Board *array,int *len,int *reserve,int idx){
	(void)(reserve);
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
int BoardArrayFindSame(Board *array,int *len,Board *find){
	int i;
	for(i=0;i<*len;i++){
		if(BoardIsSame(find,&array[i]))return i;
	}
	return -1;
}

Point PointMake(int x,int y){
	Point p;
	p.x = x;
	p.y = y;
	return p;
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

Cell **BoardAllocCellPtrArray(Board *this){
	Cell **array = OMAlloc(sizeof(Cell*) * this->width * this->height,"CellPtrArray");
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
	this->data = OMAlloc(sizeof(int) * w*h,"BoardData.data");
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
		ASSERT(p!=NULL && (int)(strlen(p))>=w);
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

int BoardCellGetHeadData(Board *this,Cell *cell){
	return BoardDataGetData(this->data,cell->head.x,cell->head.y);	
}

int BoardIsSame(Board *lhs,Board *rhs){
	int iy,ix;
	if(lhs->grayCellNum != rhs->grayCellNum)return 0;
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
		groupCell->head = head;
		groupCell->chainNum = chainNum;
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
	if(lhs->group == rhs->group)return ;
	int chainNum = lhs->chainNum + rhs->chainNum;

	if(lhs->color == CellColorWhite){
		int leftValue = BoardCellGetHeadData(this,lhs);
		int rightValue = BoardCellGetHeadData(this,rhs);
		
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
		}
	}else if(lhs->color == CellColorBlack){
		BoardCellChangeGroup(this,lhs,rhs->group,rhs->head,chainNum);
	}

}


void BoardCellSetBlackAroundWhite(Board *this,int group){	
	int iy,ix;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->group == group){
				Cell *blk;
				blk = BoardGetCellPtr(this,ix-1,iy);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,blk,CellColorBlack);
				}

				blk = BoardGetCellPtr(this,ix+1,iy);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,blk,CellColorBlack);
				}
				
				blk = BoardGetCellPtr(this,ix,iy-1);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,blk,CellColorBlack);
				}
				
				blk = BoardGetCellPtr(this,ix,iy+1);
				if(blk && blk->color==CellColorGray){
					BoardCellSetColor(this,blk,CellColorBlack);
				}
				
			}
		}
	}
}

void BoardCellWhiteChainOnChanged(Board *this,Cell *cell){
	ASSERT(cell->color == CellColorWhite);
	int value = BoardCellGetHeadData(this,cell);
	if(value>0){
		if(cell->chainNum == value){
			//ホワイト塗り終わりの周囲黒確定
			BoardCellSetBlackAroundWhite(this,cell->group);
		}else if(cell->chainNum > value){
			this->error = 1;
			BoardIndent(this);
			printf("white size over error! (%d,%d) chain=%d > %d\n",cell->pos.x,cell->pos.y,cell->chainNum,value);
		}
	}	
}

void BoardCellSetColor(Board *this,Cell *cell,CellColor color){
	ASSERT(color!=CellColorGray);
	ASSERT(cell!=NULL);
	
	cell->color = color;
	
	int found = 0;
	
	Cell *chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		if(this->error)return;
		found ++;
	}
	
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y-1);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		if(this->error)return;
		found ++;
	}
	chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		if(this->error)return;
		found++;
	}
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y+1);
	if(chk && chk->color==color){
		BoardCellGroupJoin(this,cell,chk);
		if(this->error)return;
		found++;
	}
	
	
	if(found==0){
		cell->group = this->nextGroup;
		cell->chainNum = 1;
		
		this->nextGroup ++;
	}
	
	if(color == CellColorWhite){
		BoardCellWhiteChainOnChanged(this,cell);
	}else if(color == CellColorBlack){
		Cell *cells[8];

		BoardGetCellsOfAroundCell(this,cells,cell);
		if(CellPtrArrayCheckAroundBlackL(cells)){
			
			this->error = 1;
			BoardIndent(this);
			printf("set color black 2x2 error! (%d,%d)\n",cell->pos.x,cell->pos.y);
			return;
		}
	}

}

//飛び地を作る場合はグループは同じにならない
void BoardCellAddToWhiteGroup(Board *this,Cell *add,Cell *group){
	if(add->group == group->group){
		ASSERT(add->color == group->color);
		return;
	}
	
	if(add->color == CellColorWhite){
		BoardCellGroupJoin(this,add,group);
		BoardCellWhiteChainOnChanged(this,group);
	}else if(add->color == CellColorGray){
		BoardCellSetColor(this,add,CellColorWhite);
	}else{
		BoardIndent(this);
		printf("add black to white error (%d,%d)\n",
			   add->pos.x,add->pos.y);
		//BoardPrint(this);
		//ASSERT(0);//この塗り替えはできない
		this->error = 1;
	}
}

//繋げられない白
int BoardCellsCantChainWhite(Board *this,Cell *lhs,Cell *rhs){
	//同じグループなら繋げて良い
	if(lhs->group == rhs->group)return 0;
	//違うグループでも片方親なしなら繋げて良い
	int leftValue = BoardCellGetHeadData(this,lhs);
	int rightValue = BoardCellGetHeadData(this,rhs);
	if(leftValue==0 || rightValue==0)return 0;
	return 1;
}

int BoardCell4CantChainWhite(Board *this,Cell *c0,Cell *c1,Cell *c2,Cell *c3){
	Cell *cell = NULL;
	if(c0 && c0->color==CellColorWhite && BoardCellGetHeadData(this,c0)>0){
		cell = c0;
	}
	if(c1 && c1->color==CellColorWhite &&BoardCellGetHeadData(this,c1)>0){
		if(cell && BoardCellsCantChainWhite(this,cell,c1))return 1;
		cell = c1;
	}
	if(c2 && c2->color==CellColorWhite &&BoardCellGetHeadData(this,c2)>0){
		if(cell && BoardCellsCantChainWhite(this,cell,c2))return 1;
		cell = c2;
	}
	if(c3 && c3->color==CellColorWhite &&BoardCellGetHeadData(this,c3)>0){
		if(cell && BoardCellsCantChainWhite(this,cell,c3))return 1;
	}
	return 0;
}

/*
 0 1 2
 3 C 4
 5 6 7
 へのポインタを返す
 端ならnullを入れる
 */
void BoardGetCellsOfAroundCell(Board *this,Cell *result[8],Cell *cell){
	result[0] = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y-1);
	result[1] = BoardGetCellPtr(this,cell->pos.x  ,cell->pos.y-1);
	result[2] = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y-1);
	
	result[3] = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y  );
	result[4] = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y  );
	
	result[5] = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y+1);
	result[6] = BoardGetCellPtr(this,cell->pos.x  ,cell->pos.y+1);
	result[7] = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y+1);
}

int CellPtrArrayCheck3Color(Cell **cells,int i0,int i1,int i2,int color){
	if(cells[i0]==NULL || (cells[i0]->color & color) == 0)return 0;
	if(cells[i1]==NULL || (cells[i1]->color & color) == 0)return 0;
	if(cells[i2]==NULL || (cells[i2]->color & color) == 0)return 0;
	return 1;
}

int CellPtrArrayCheckAroundBlackL(Cell *cells[8]){
	return
		CellPtrArrayCheck3Color(cells,0,1,3,CellColorBlack) ||
		CellPtrArrayCheck3Color(cells,1,2,4,CellColorBlack) ||
		CellPtrArrayCheck3Color(cells,3,5,6,CellColorBlack) ||
		CellPtrArrayCheck3Color(cells,4,6,7,CellColorBlack);
}

int BoardCellSetBy3x3(Board *this){
	int iy,ix;
	int num = 0;
	
	Cell *cells[8];
	
	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				BoardGetCellsOfAroundCell(this,cells,cell);
				if(CellPtrArrayCheckAroundBlackL(cells)){
					//2x2則で白確定
					BoardCellSetColor(this,cell,CellColorWhite);
					num++;
				}else if(BoardCell4CantChainWhite(this,cells[1],cells[3],cells[4],cells[6])){
					//白グループ分割則で黒確定
					BoardCellSetColor(this,cell,CellColorBlack);
					num++;
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

int BoardCellIsValueWhite(Board *this,Cell *cell){
	return cell->color == CellColorWhite &&
	BoardDataGetData(this->data,cell->pos.x,cell->pos.y) != 0; //数字が書かれてる
	
//	BoardCellGetHeadData(this,cell) != 0 &&
//	PointCmp(&cell->pos,&cell->head)==0 ; 
}

//数字の書いてあるセルのリスト
int BoardGetCellsOfValueWhite(Board *this,Cell **result,int *resultLen){
	int iy,ix;
	
	*resultLen = 0;
	
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(BoardCellIsValueWhite(this,cell)){
				result[*resultLen] = cell;
				*resultLen = *resultLen +1;
			}
		}
	}
	return *resultLen;
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
			cell->markDist = 0;
		}
	}
}

void BoardSetCellMarkUniqueGroup(Board *this,Cell *cell,Cell *mark){
	(void)this;
	if(cell->markCell!=NULL && cell->markCell->group == mark->group){
		//すでにマークされていて、グループが同じならマークしない
	}else{
		//そうでなければ
		cell->markCell = mark;
		cell->mark ++;
		cell->markDist = cell->dist;//マークした時のdistを保存
	}
}

int BoardCellIsAroundAnotherValueWhite(Board *this,Cell *cell,int group){
	int num = 0;
	Cell *chk;
	chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y);
	if(chk && chk->color == CellColorWhite && 
	   BoardCellGetHeadData(this,chk) != 0 &&
	   chk->group != group){
		num += 1;
	}
	chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y);
	if(chk && chk->color == CellColorWhite && 
	   BoardCellGetHeadData(this,chk) != 0 &&
	   chk->group != group){
		num+=2;
	}
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y-1);
	if(chk && chk->color == CellColorWhite && 
	   BoardCellGetHeadData(this,chk) != 0 &&
	   chk->group != group){
		num+=4;
	}
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y+1);
	if(chk && chk->color == CellColorWhite && 
	   BoardCellGetHeadData(this,chk) != 0 &&
	   chk->group != group){
		num+=8;
	}
	return num;
}

void BoardCalcGroupDistanceAddToOpen(Board *this,Cell **open,int *openLen,Cell *start,Cell *prev,Cell *next){
	if(next == NULL ||
	   (next->color & (CellColorGray | CellColorWhite))==0 ||
	   next->dist != 0)return; //既に距離を調べたセルは無視
	
	//違うグループとつながる部分は進まない
	int dir = BoardCellIsAroundAnotherValueWhite(this,next,start->group);
	if(dir>0)return;
	
	int dist = 0;
	
	if(next->group == start->group){
		dist = prev->dist;//グループが同じなら移動量を消費しない
	}else{
		dist = prev->dist+1;
	}
	int threshold = BoardCellGetWhiteGroupRemain(this,start) + 1;//残余0なら1(=自身)までは塗る 
	if(threshold < dist)return;
	
	//追加できる
	next->dist = dist;
	BoardSetCellMarkUniqueGroup(this,next,start);
	int idx = CellPtrArrayFindByDist(open,openLen,dist+1);
	if(idx==-1)idx = *openLen;
	CellPtrArrayInsert(open,openLen,idx,next);
}

//グループ拡大域を抽出
int BoardGetCellsOfWhiteExpansionFromDist(Board *this,Cell **result,int *resultLen,int group){
	int iy,ix;
	int num= 0;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->dist > 0 &&
			   cell->group != group){
				if(result!=NULL)result[num] = cell;
				num++;
			}
		}
	}
	if(resultLen!=NULL)*resultLen = num;
	return num;
}

int BoardGetCellsOfGroup(Board *this,Cell **result,int *resultLen,int group){
	int iy,ix;
	int num = 0;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->group == group){
				if(result!=NULL)result[num] = cell;
				num++;
			}
		}
	}
	if(resultLen!=NULL)*resultLen = num;
	return num;
}

//ダイクストラ法2
//同一グループ間は距離を消耗せずに移動する
void BoardCalcGroupDistance(Board *this,Cell *start){
	BoardClearCellDist(this);
	start->dist = 1;
	BoardSetCellMarkUniqueGroup(this,start,start);
	Cell **open = BoardAllocCellPtrArray(this);
	int openLen = 0;
	
	CellPtrArrayInsert(open,&openLen,0,start);
	while(openLen > 0){
		Cell *chk = NULL;
		Cell *cell = open[0];
		CellPtrArrayRemove(open,&openLen,0);
		
		chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y+0);
		BoardCalcGroupDistanceAddToOpen(this,open,&openLen,start,cell,chk);
		
		chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y+0);
		BoardCalcGroupDistanceAddToOpen(this,open,&openLen,start,cell,chk);		
		
		chk = BoardGetCellPtr(this,cell->pos.x+0,cell->pos.y-1);
		BoardCalcGroupDistanceAddToOpen(this,open,&openLen,start,cell,chk);				
	
		chk = BoardGetCellPtr(this,cell->pos.x+0,cell->pos.y+1);
		BoardCalcGroupDistanceAddToOpen(this,open,&openLen,start,cell,chk);		
	}
	SAFE_FREE(open);
}

int BoardGetGroupsByCellWithColor(Board *this,Cell **result,int *retNum,int color){
	int iy,ix;
	int num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if((int)(cell->color) == color){
				CellPtrArrayAddUniqueGroup(result,&num,cell);
			}
		}		
	}
	*retNum = num;
	return num;
}

//markで取るマークを指定する
//もしmark=0だと0でないマーク全てとる
int BoardGetCellsOfMarkAroundColor(Board *this,Cell **result,int *num,int mark,int color){
	int iy,ix;
	*num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if((mark==0 && cell->mark!=0) || 
			   (mark!=0 && cell->mark == mark)){
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
			//BoardIndent(this);
			//printf("one way white expand (%d,%d)\n",arounds[0]->pos.x,arounds[0]->pos.y);
			BoardCellAddToWhiteGroup(this,arounds[0],cell);
			if(this->error)break;
			num++;
			break;
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
	Cell **area = BoardAllocCellPtrArray(this);
	int areaLen = 0;
	Cell **arounds = BoardAllocCellPtrArray(this);
	int aroundsLen = 0;
	
	BoardBlackCellAreaMark(this,area,&areaLen);
	
//	BoardGetGroupsByCellWithColor(this,groups,&groupsLen,CellColorBlack);
	int i;
	
	if(areaLen>1){
		//グループが1つの場合は伸ばすべきと限らない
		for(i=0;i<areaLen;i++){
			Cell *cell = area[i];
			BoardGetCellsOfMarkAroundColor(this,arounds,&aroundsLen,cell->mark,CellColorGray);
			//このマークグループの周りのグレー
			if(aroundsLen == 1){
				//確定伸ばし
				//BoardIndent(this);
				//printf("one way white expand (%d,%d)\n",arounds[0]->pos.x,arounds[0]->pos.y);
				BoardCellSetColor(this,arounds[0],CellColorBlack);
				if(this->error)break;
				num++;
				break;
			}else if(aroundsLen == 0){
				//伸ばせないグループがあったら詰んでる
				this->error = 1;
				BoardIndent(this);
				printf("black split error! (expand) grp=%d (%d,%d)\n",
					   cell->group , cell->pos.x,cell->pos.y);
				break;			
			}
			
		}
		
		/*
		for(i = 0; i< groupsLen; i++){
			Cell *grpCell = groups[i];

			BoardGetCellsOfGroupAroundColor(this,arounds,&aroundsLen,grpCell->group,CellColorGray);

			if(aroundsLen == 1){
				BoardCellSetColor(this,arounds[0],CellColorBlack);
				if(this->error)break;
				num++;
				break;
			}else if(aroundsLen == 0){
				//伸ばせないグループがあったら詰んでる
				this->error = 1;
				BoardIndent(this);
				printf("black split error! (expand) grp=%d (%d,%d)\n",grpCell->group , grpCell->head.x,grpCell->head.y);
				break;				
			}
		}
		 */
	}
	
	SAFE_FREE(arounds);
	SAFE_FREE(area);
	
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
				printf("black split error (full check) ! (%d,%d)\n",ix,iy);
				this->error = 1;
				goto checkend;
			}
		}
	}
checkend:
	
	BoardClearCellMark(this);
	
	SAFE_FREE(cells);

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

//黒を塗り分ける
void BoardBlackCellAreaMark(Board *this,Cell **result,int *num){
	*num = 0;
	int mark = 1;
	int iy,ix;
	BoardClearCellMark(this);
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->mark == 0 && cell->color == CellColorBlack){
				BoardCellAreaSetMarkByColor(this,cell,mark,CellColorBlack);
				CellPtrArrayAddUnique(result,num,cell);
				mark ++;
			}
		}
	}
}

//距離マップを戻りながら到達できるセルを全てマークする
//隣接する同じ距離のところか1小さいところをたどっていく
void BoardCellSetMarkBackDist(Board *this,Cell *cell,int mark){
	if(cell->mark != 0)return;
	if(cell->dist == 0)return;

	cell->mark |= mark;
	
	Cell *chk;
	chk = BoardGetCellPtr(this,cell->pos.x-1,cell->pos.y);
	if(chk && (chk->dist==cell->dist || chk->dist==cell->dist-1))BoardCellSetMarkBackDist(this,chk,mark);
	chk = BoardGetCellPtr(this,cell->pos.x+1,cell->pos.y);
	if(chk && (chk->dist==cell->dist || chk->dist==cell->dist-1))BoardCellSetMarkBackDist(this,chk,mark);
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y-1);
	if(chk && (chk->dist==cell->dist || chk->dist==cell->dist-1))BoardCellSetMarkBackDist(this,chk,mark);
	chk = BoardGetCellPtr(this,cell->pos.x,cell->pos.y+1);
	if(chk && (chk->dist==cell->dist || chk->dist==cell->dist-1))BoardCellSetMarkBackDist(this,chk,mark);
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
	
	BoardCalcGroupDistance(&view,from);
	
	BoardClearCellMark(&view);
	ASSERT(to->dist>1 && from->dist==1);

	BoardCellSetMarkBackDist(&view,to,1);
	BoardClearCellDistWithMask(&view,1);
	
	int *distNum = OMAlloc((to->dist+1)*sizeof(int),"distnum");
	
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
				//BoardIndent(this);
				//printf("dist back fix white (%d,%d)\n",thisCell->pos.x,thisCell->pos.y);
				BoardCellAddToWhiteGroup(this,thisCell,thisFrom);
				if(this->error)goto release;
				num++;
			}
		}
	}

release:
	
	SAFE_FREE(distNum);

	BoardRelease(&view);
	return num;
}

int BoardCellSetBlackUnreachable(Board *this){
	int num = 0;
	
	BoardClearCellMark(this);
	
	Cell **values = BoardAllocCellPtrArray(this);
	int valuesLen = 0;
	Cell **cells = BoardAllocCellPtrArray(this);
	int cellsLen = 0;
	
	BoardGetCellsOfValueWhite(this,values,&valuesLen);
	int i;
	
	for(i=0;i<valuesLen;i++){
		Cell *valueCell = values[i];
		//最短距離を書き込んでマークする
		BoardCalcGroupDistance(this,valueCell);
		int rem = BoardCellGetWhiteGroupRemain(this,valueCell);
		if(rem>0){
			//ついでに予備領域検査
			BoardGetCellsOfWhiteExpansionFromDist(this,cells,&cellsLen,valueCell->group);
			
			if(cellsLen < rem){
				//領域不足で破綻
				BoardIndent(this);
				printf("white cell area is lacking!! rem=%d , space=%d , grp=%d(%d,%d)\n",
					   rem,cellsLen,valueCell->group,valueCell->pos.x,valueCell->pos.y);

				this->error = 1;
				goto endchk;
			}else if(cellsLen == rem){
				//充填確定
				int j;
				//BoardIndent(this);
				//printf("white cell fill fixed. rem=%d, space =%d , grp=%d(%d,%d)\n",
				//	   rem,cellsLen,valueCell->group,valueCell->pos.x,valueCell->pos.y);

				for(j=0;j<cellsLen;j++){
					BoardCellAddToWhiteGroup(this,cells[j],valueCell);
					if(this->error)break;
					num++;
				}

				goto endchk;
			}
					
		}else if(rem==0){
			//サイズチェック
			int size = BoardGetCellsOfGroup(this,cells,&cellsLen,valueCell->group);
			int value =BoardCellGetHeadData(this,valueCell);
			//BoardIndent(this);
			//printf("white cell(%d,%d/grp=%d) area size=%d , value=%d\n",
			//	   valueCell->pos.x,valueCell->pos.y,valueCell->group,size,value);
			//getchar();
			if(size != value){
				BoardIndent(this);
				printf("white cell(%d,%d/grp=%d) area=size %d != value=%d\n",
					   valueCell->pos.x,valueCell->pos.y,valueCell->group,size,value);
				this->error = 1;
				getchar();
				goto endchk;
			}
				
		}else{
			ASSERT(0);
		}
	}
	
	//結果の解析
	int ix,iy;
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray && cell->mark ==0){
				//到達できないグレーセルは黒
				BoardCellSetColor(this,cell,CellColorBlack);
				if(this->error)goto endchk;
				num ++ ;
			}else if(cell->color == CellColorWhite){
				//白セルについて
				if(BoardCellGetHeadData(this,cell)==0){
					//所属不明の白セルについて
					if(cell->mark == 0){
						//到達できないのはエラー
						this->error = 1;
						BoardIndent(this);
						printf("unreachable no value white cell (%d,%d)\n",ix,iy);
						goto endchk;
					}else if(cell->mark == 1){
						//到達グループが1つならそれの一部にする
						Cell *markCell = cell->markCell;
						ASSERT(markCell!=NULL);

						//ぴったり到達したならルートを逆算して塗れる
						int rem = BoardCellGetWhiteGroupRemain(this,markCell);
						if(rem+1 == cell->markDist){
							//BoardIndent(this);
							//printf("markCell(%d,%d) dist back chain cell(%d,%d) \n",
							//	   markCell->pos.x,markCell->pos.y,cell->pos.x,cell->pos.y);
							//BoardPrint(this);
							num += BoardCellSetWhiteByDistBack(this,markCell,cell);							
						}else{
							//BoardIndent(this);
							//printf("markCell(%d,%d) dist back group cell(%d,%d) \n",
							//	   markCell->pos.x,markCell->pos.y,cell->pos.x,cell->pos.y);
							
							//グループ追加
							BoardCellAddToWhiteGroup(this,cell,markCell);
							num ++;
						}
							
						goto endchk;
					}
				}else{
					//所属の確定の白セルについて
					ASSERT(cell->mark < 2);//２種類あったらバグってる
					
					if(cell->mark == 0){
						//到達できないのはエラー
						
						BoardIndent(this);
						printf("cant reach (%d,%d) from any head error\n",
							   cell->pos.x,cell->pos.y);
						this->error = 1;
						goto endchk;
						
					}
				}
			}
		}
	}
endchk:
	BoardClearCellMark(this);
	BoardClearCellDist(this);
	
	SAFE_FREE(cells);
	SAFE_FREE(values);
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
	this->cells = OMAlloc(sizeof(Cell) * this->width * this->height,"Board.cells");

	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			CellInit(cell,ix,iy);
		}
	}
	
	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			//数字セルは白確定する
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(BoardDataGetData(this->data,ix,iy)>0){
				BoardCellSetColor(this,cell,CellColorWhite);
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
	this->cells = OMAlloc(sizeof(Cell) * this->width * this->height,"Board.cells");
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
	if(flags & BoardPrintChain){
		printf("    |");
	}else if(flags & BoardPrintGroup){
		printf("    |");
	}else{
		printf("  |");
	}
	for(ix=0;ix<this->width;ix++){
		if(flags & BoardPrintChain){
			printf("     %2d   |",ix);
		}else if(flags & BoardPrintGroup){
			printf("   %2d |",ix);
		}else{
			printf("%2d|",ix);
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
					
					int data = BoardDataGetData(this->data,ix,iy);
					int value = BoardCellGetHeadData(this,cell);
					if(data != 0){
						printf("%2d(%3d,%3d)",data,cell->group,cell->chainNum);
					}else if(value != 0){
						printf(" +(%3d,%3d)",cell->group,cell->chainNum);
					}else{
						printf(" .(%3d,%3d)",cell->group,cell->chainNum);
					}

				}else{
					printf(" X(%3d,%3d)",cell->group,cell->chainNum);
				}
			}
			printf("\n");
		}
	
	}else if(flags & BoardPrintGroup){
	
		for(iy=0;iy<this->height;iy++){
			BoardIndent(this);
			printf(" %2d |",iy);
			for(ix=0;ix<this->width;ix++){
				Cell *cell = BoardGetCellPtr(this,ix,iy);
				if(cell->color == CellColorGray){
					printf("  (%3d)",cell->group);
				}else if(cell->color == CellColorWhite){
					
					int data = BoardDataGetData(this->data,ix,iy);
					int value = BoardCellGetHeadData(this,cell);
					if(data != 0){
						printf("%2d(%3d)",data,cell->group);
					}else if(value != 0){
						printf(" +(%3d)",cell->group);
					}else{
						printf(" .(%3d)",cell->group);
					}

				}else{
					printf(" X(%3d)",cell->group);
				}
			}
			printf("\n");
		}
	}else{
		for(iy=0;iy<this->height;iy++){
			BoardIndent(this);
			printf("%2d|",iy);
			for(ix=0;ix<this->width;ix++){
				Cell *cell = BoardGetCellPtr(this,ix,iy);
				if(cell->color == CellColorGray){
					printf("   ");
				}else if(cell->color == CellColorWhite){
					
					int data = BoardDataGetData(this->data,ix,iy);
					int value = BoardCellGetHeadData(this,cell);
					if(data != 0){
						printf("%2d ",data);
					}else if(value != 0){
						printf(" + ");
					}else{
						printf(" . ");
					}
					
				}else{
					printf("XX ");
				}
			}
			printf("\n");
		}
	}
	
	BoardIndent(this);
	printf("--[step=%d,open=%d,gray=%d,score=%d]--\n",solveStep,openNodesLen,this->grayCellNum,BoardGetScore(this));
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

//異常状態の発生は操作ごとに検出しているので、
//グレーセルが無いならクリアしているはず。
int BoardIsSolved(Board *this){
	return this->error==0 && this->grayCellNum == 0;
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

int BoardCellGetWhiteGroupRemain(Board *this,Cell *cell){
	return BoardCellGetHeadData(this,cell) - cell->chainNum;
}

int BoardGetScore(Board *this){
	if(this->grayCellNum == 0)return 0;
	//return this->grayCellNum * this->depth;
	return this->grayCellNum + this->depth * 5;
	//return this->grayCellNum;
}

//白が次に伸びうる場所全部
int BoardGetCellsOfWhiteNextExpansion(Board *this,Cell **result,int *resultLen){
	*resultLen = 0;
	
	Cell **area = BoardAllocCellPtrArray(this);
	int areaLen = 0;
	
	//白をマークで塗り分ける
	BoardWhiteCellAreaMark(this,area,&areaLen);
	//マークされたセル全ての周りを取る(mark==0)
	BoardGetCellsOfMarkAroundColor(this,result,resultLen,0,CellColorGray);

	SAFE_FREE(area);
	
	return *resultLen;
}

//黒が次に伸びうる場所全部
int BoardGetCellsOfBlackNextExpansion(Board *this,Cell **result,int *resultLen){
	*resultLen = 0;
	Cell **area = BoardAllocCellPtrArray(this);
	int areaLen = 0;
	BoardBlackCellAreaMark(this,area,&areaLen);
	if(areaLen >= 2){
		//1エリアなら不確定
		BoardGetCellsOfMarkAroundColor(this,result,resultLen,0,CellColorGray);
	}
	SAFE_FREE(area);
	return *resultLen;
}

//0:fail 1:ok
int solveSingle(Board *board){
	int ret = 0;
	//固定計算を進める
	while(1){		
		int num= 0;
		if(board->error)break;
		
		num = BoardCellSetBy3x3(board);
		if(board->error)break;
		if(num>0)continue;
		
		num = BoardCellExpandWhite(board);
		if(board->error)break;
		if(num > 0)continue;
		
		num = BoardCellExpandBlack(board);
		if(board->error)break;
		if(num > 0)continue;
		
		num = BoardCellSetBlackUnreachable(board);
		if(board->error)break;
		if(num>0)continue;
		
		BoardCheckBlackSplit(board);
		if(board->error)break;
		
		ret = 1;
		break;
	}
	BoardCalcGrayCellNum(board);
	return ret;
}

void solveAddToOpen(Board *node){
	int idx;
	int score = BoardGetScore(node);

	idx = BoardArrayFindSame(openNodes,&openNodesLen,node);
	if(idx >= 0){
		int openScore = BoardGetScore(&openNodes[idx]);
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
	
	idx = BoardArrayFindByScore(openNodes,&openNodesLen,score);
	if(idx==-1){
		idx = openNodesLen;
	}
	BoardArrayInsert(openNodes,&openNodesLen,&openNodesReserve,idx,node);
}

int solveBranch(Board *board){
	int ret = 0;
	Cell **cells = BoardAllocCellPtrArray(board);
	int cellsLen = 0;
	
	int childrenReserve = board->width * board->height;
	Board *children = BoardArrayAlloc(childrenReserve);
	int childrenLen = 0;
	int i;
	
	while(1){
		for(i=0;i<childrenLen;i++){
			BoardRelease(&children[i]);
		}
		childrenLen = 0;
		if(board->error)break;
		
		if(BoardIsSolved(board)){
			ret = 1;
			break;
		}
		
		
		//黒塗りテスト、ノードは追加しない
		BoardGetCellsOfBlackNextExpansion(board,cells,&cellsLen);
		for(i=0;i<cellsLen;i++){
			Cell *cell = cells[i];
			Board test;
			BoardInitWithBoard(&test,board);
			test.depth++;
			BoardCellSetColor(&test,BoardGetThisCellPtr(&test,cell),CellColorBlack);
			ret = solveSingle(&test);
			BoardRelease(&test);
			if(ret==0){
				//この失敗による白確定を期待している
				BoardIndent(board);
				printf("black expand test fail => white fixed (%d,%d)\n",
					   cell->pos.x,cell->pos.y);
				BoardCellSetColor(board,BoardGetThisCellPtr(board,cell),CellColorWhite);
				ret = solveSingle(board);
				if(ret == 0){
					BoardIndent(board);
					printf("branch white fixed (%d,%d) error\n",cell->pos.x,cell->pos.y);
					board->error = 1;
					//BoardPrint(board);
				}
				break;
			}
			//成功時特に何もしない
		}
		if(i<cellsLen)continue;
		 
		
				
		BoardGetCellsOfWhiteNextExpansion(board,cells,&cellsLen);
		
		//BoardIndent(board);
		//printf("next expansion(%d)",cellsLen);
		//for(i=0;i<cellsLen;i++){
		//	printf("(%d,%d),",cells[i]->pos.x,cells[i]->pos.y);
		//}
		//printf("\n");
		//BoardPrint(board);
		//getchar();
		
		if(cellsLen == 0){
			BoardIndent(board);
			printf("next expansion is none\n");
			board->error = 1;
			continue;
		}
		
		for(i=0;i<cellsLen;i++){
			Cell *cell = cells[i];
			Board test;
			BoardInitWithBoard(&test,board);
			test.depth++;
			//printf("use expansion[%d] white cell(%d,%d)\n",i,cell->pos.x,cell->pos.y);
			BoardCellSetColor(&test,BoardGetThisCellPtr(&test,cell),CellColorWhite);
			ret = solveSingle(&test);
			//BoardPrint(&test);
			if(ret == 0){
				//白失敗で黒確定
				//BoardIndent(board);
				//printf("white expand fail => black fixed (%d,%d)\n",cell->pos.x,cell->pos.y);
				
				BoardRelease(&test);
				
				BoardCellSetColor(board,BoardGetThisCellPtr(board,cell),CellColorBlack);
				ret = solveSingle(board);
				if(ret == 0){
					BoardIndent(board);
					printf("branch black fixed (%d,%d) error\n",cell->pos.x,cell->pos.y);
					board->error = 1;
					//BoardPrint(board);
				}
				//リストを取るところからやりなおす
				break;
			}else{
				//白成功を溜め込む
				BoardArrayInsert(children,&childrenLen,&childrenReserve,childrenLen,&test);
			}
		}
		if(i<cellsLen)continue;
		
		ASSERT(cellsLen == childrenLen);
		//全部追加できたならオープンに追加して終わり
		BoardIndent(board);
		printf("add branches(%d):",childrenLen);
		for(i=0;i<cellsLen;i++){
			Cell *cell = cells[i];
			printf("(%d,%d),",cell->pos.x,cell->pos.y);
			solveAddToOpen(&children[i]);
		}
		childrenLen = 0;
		printf("\n");
		
		BoardPrint(board);
		getchar();
		
		ret = 1;
		break;		
	}
	
	SAFE_FREE(children);
	SAFE_FREE(cells);
	
	return ret;
}

//0:fail 1:solved
int solve(){
	int solved = 0;
	
	while(openNodesLen>0 && solved==0){
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

		Board board = openNodes[0];
		BoardArrayRemove(openNodes,&openNodesLen,&openNodesReserve,0);
		
		//以下分岐処理
		int ret = solveBranch(&board);
		
		if(ret && BoardIsSolved(&board)){
			BoardIndent(&board);
			printf("==>solved ! step = %d\n",solveStep);
			BoardPrint(&board);
			
			//BoardPrintWithFlag(&board,BoardPrintGroup);
			
			BoardIndent(&board);
			printf("==<solved ! step = %d\n",solveStep);
			solved = 1;//ループ抜ける
		}
		
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
	
	ret = solve();
	
	if(!ret){
		printf("==solve failed ... step = %d\n",solveStep);
	}

	SAFE_FREE(openNodes);
	
	BoardDataRelease(&bData);
	
	OMMemChainPrint();
	
	return EXIT_SUCCESS;
}

