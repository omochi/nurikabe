#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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


typedef struct{
	int x;
	int y;
}Point;

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

typedef struct{
	int width;
	int height;
	BoardData *data;
	Cell *cells;
	int nextGroup;
	
	int error;
}Board;

int solve(Board *board,int depth);
void BoardCellSetColor(Board *this,int x,int y,CellColor color);


//ここまでヘッダ


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



void CellInit(Cell *this){
	this->color = CellColorGray;
	//PointInit(&this->head);
	this->group = 0;
	this->chainNum = 1;
}






Cell *BoardGetCellPtr(Board *this,int x,int y){
	if(x<0 || this->width<=x || y<0 || this->height<=y)return NULL;
	return &this->cells[y*this->width+x];
}

int BoardGetCellHeadData(Board *this,Cell *cell){
	return BoardDataGetData(this->data,cell->head.x,cell->head.y);	
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
	
	if(lhs->group == rhs->group)return;
	
	//printf("join(%d) %d x %d\n",lhs->color,lhs->group,rhs->group);
	
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
			this->error= 1;
			printf("group join(%d) %d=%d x %d=%d error!\n",lhs->color,
				   lhs->group,leftValue,
				   rhs->group,rightValue);
		}
	}
	

}


void BoardCellSetBlackAroundWhite(Board *this,int group){
	printf("black around white(grp=%d)\n",group);
	
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


//検査無し
void BoardCellSetColor(Board *this,int x,int y,CellColor color){

	
	Cell *cell = BoardGetCellPtr(this,x,y);
	cell->color = color;
	
	printf("(%d,%d) = %d\n",x,y,color);
	
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
	
	
	int value = BoardGetCellHeadData(this,cell);
	//ホワイト塗り終わりの周り黒確定
	if(cell->color == CellColorWhite && cell->chainNum == value){
		BoardCellSetBlackAroundWhite(this,cell->group);
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
					printf("black2x2 white right down(%d,%d)\n",ix,iy);
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				//右上の白
				if(BoardCellCheckThreeColor(this,PointMake(ix-1,iy),PointMake(ix-1,iy+1),PointMake(ix,iy+1),CellColorBlack)){
					printf("black2x2 white right up(%d,%d)\n",ix,iy);
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				
				//左下の白
				if(BoardCellCheckThreeColor(this,PointMake(ix,iy-1),PointMake(ix+1,iy-1),PointMake(ix+1,iy),CellColorBlack)){
					printf("black2x2 white left down(%d,%d)\n",ix,iy);
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				//左上の白
				if(BoardCellCheckThreeColor(this,PointMake(ix,iy+1),PointMake(ix+1,iy+1),PointMake(ix+1,iy),CellColorBlack)){
					printf("black2x2 white left up(%d,%d)\n",ix,iy);
					BoardCellSetColor(this,ix,iy,CellColorWhite);
					num++;
					continue;
				}
				
				
			}
		}
	}
	return num;
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

//グループの周りの色のリストを返す
int BoardGetGroupAroundColor(Board *this,Point *result,int *retNum,int group,int color){
	int iy,ix;
	int num = 0;
	for(iy = 0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			//4方向足す
			if(cell->group == group){
				
				Cell *chk;
				int x,y;
				x = ix-1;
				y = iy;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == color){
					PointArrayAddUnique(result,&num,PointMake(x,y));
				}
				
				x = ix;
				y = iy-1;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == color){
					PointArrayAddUnique(result,&num,PointMake(x,y));
				}
				
				x = ix+1;
				y = iy;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == color){
					PointArrayAddUnique(result,&num,PointMake(x,y));
				}
				
				x = ix;
				y = iy+1;
				chk = BoardGetCellPtr(this,x,y);
				if(chk && chk->color == color){
					PointArrayAddUnique(result,&num,PointMake(x,y));
				}
				
			}
		}		
	}
	
	*retNum = num;
	return num;
	
}


//白の違反連結になるグレーは事前に黒になっている
int BoardCellExpandWhite(Board *this){
	int num = 0;
	
	int *groups = BoardAllocGroupArray(this);
	int groupsLen = 0;
	Point *points = BoardAllocPointArray(this);
	int pointsLen = 0;
	
	BoardGetGroupsWithColor(this,groups,&groupsLen,CellColorWhite);
	int i;
	for(i=0;i<groupsLen;i++){
		int grp = groups[i];
		
		BoardGetGroupAroundColor(this,points,&pointsLen,grp,CellColorGray);
		if(pointsLen == 1){
			//一個だった場合は確定なので進める
			int x = points[0].x;
			int y = points[0].y;
			printf("one way white (grp %d) -> (%d,%d)\n",grp,x,y);
			
			BoardCellSetColor(this,x,y,CellColorWhite);
			num++;
		}
	}
	
	SAFE_FREE(points);
	SAFE_FREE(groups);
	
	return num;
}

//黒の連結
int BoardCellExpandBlack(Board *this){
	int num = 0;
	int *groups = BoardAllocGroupArray(this);
	int groupsLen = 0;
	Point *points = BoardAllocPointArray(this);
	int pointsLen = 0;
	
	BoardGetGroupsWithColor(this,groups,&groupsLen,CellColorBlack);
	int i;
	for(i = 0; i< groupsLen; i++){
		int grp = groups[i];
		BoardGetGroupAroundColor(this,points,&pointsLen,grp,CellColorGray);
		if(pointsLen == 1){
			int x = points[0].x;
			int y = points[0].y;
			printf("one way black (grp %d) -> (%d,%d)\n",grp,x,y);
			BoardCellSetColor(this,x,y,CellColorBlack);
			num++;
		}
	}
	
	SAFE_FREE(groups);
	SAFE_FREE(points);
	return num;
}


void BoardInitWithData(Board *this,BoardData *data){
	int iy,ix;
	this->width = data->width;
	this->height = data ->height;
	this->data = data;
	this->nextGroup = 1;
	this->error = 0;
	this->cells = malloc(sizeof(Cell) * this->width * this->height);

	for(iy =0;iy<this->height;iy++){
		for(ix = 0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			CellInit(cell);
			cell->head = PointMake(ix,iy);
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
		printf("    %2d   |",ix);
	}
	printf("\n");
	for(iy=0;iy<this->height;iy++){
		printf("%2d |",iy);
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			if(cell->color == CellColorGray){
				printf("  (%2d,%3d)",cell->group,cell->chainNum);
			}else if(cell->color == CellColorWhite){
				int v = BoardDataGetData(this->data,ix,iy);
				if(v == 0){
					printf(" .(%2d,%3d)",cell->group,cell->chainNum);
				}else{
					printf("%2d(%2d,%3d)",v,cell->group,cell->chainNum);
				}
			}else{
				printf(" X(%2d,%3d)",cell->group,cell->chainNum);
			}
		}
		printf("\n");
	}
}

int BoardIsSolved(Board *this){
	int iy,ix;
	
	int blackGroup = 0;
	
	for(iy=0;iy<this->height;iy++){
		for(ix=0;ix<this->width;ix++){
			Cell *cell = BoardGetCellPtr(this,ix,iy);
			int value = BoardGetCellHeadData(this,cell);
			
			if(cell->color ==CellColorGray){
				printf("there is gray(%d,%d)\n",ix,iy);
				//塗り終わっていない
				return 0;
			}else if(cell->color == CellColorWhite){
				//白の長さが正しい
				if(cell->chainNum != value){
					printf("white num %d != %d (%d,%d)\n",cell->chainNum,value,ix,iy);
					return 0;
				}
			}else if(cell->color ==CellColorBlack){
				//黒が1連結
				if(blackGroup == 0){
					blackGroup = cell->group;
				}else{
					if(blackGroup != cell->group){
						printf("black group over\n");
						return 0;
					}
				}
			}
		}
	}
	
	return 1;
}

int BoardFindNextBranchGroup(Board *this){
	//次に探索するとよさそうなグループを選ぶ
	//残り必要長の小さいグループにする。
	int remain = INT_MAX;
	Cell *choice = NULL;
	
	Cell **groups = BoardAllocCellPtrArray(this);
	int groupsLen = 0;
	BoardGetGroupsByCellWithColor(this,groups,&groupsLen,CellColorWhite);
	int i;
	for(i=0;i<groupsLen;i++){
		Cell *grpCell = groups[i];
		int cellRemain = BoardGetCellHeadData(this,grpCell) - grpCell->chainNum;
		if(cellRemain == 0)continue;//完成している白
		if(cellRemain < remain){
			remain = cellRemain;
			choice = grpCell;
		}		
	}
	
	SAFE_FREE(groups);
	
	if(choice==NULL){
		this->error = 1;
		return 0;
	}
	
	return choice->group;
}

int solveBranch(Board *board,int depth){
	int ret = 0;
	int group = BoardFindNextBranchGroup(board);
	if(group == 0){
		return 0;		
	}
	
	Point *points = BoardAllocPointArray(board);
	int pointsLen = 0;
	
	printf("depth %d:branch group=%d\n",depth,group);
	BoardGetGroupAroundColor(board,points,&pointsLen,group,CellColorGray);
	ASSERT(pointsLen != 1);//1なら事前にぬったはず
	if(pointsLen==0){
		//詰んでる
		SAFE_FREE(points);
		return 0;
	}
	
	//ここで選択肢が出なかった場合はそのままret = 0を返却する
	int i;
	for(i=0;i<pointsLen;i++){
		//この中のどれかが正解であるはず。全部だめなら問題が解無し。
		
		//複製
		Board next;
		BoardInitWithBoard(&next,board);
		
		int x = points[i].x;
		int y = points[i].y;
		
		printf("depth %d:branch[%d/%d]  group=%d -> (%d,%d)\n",depth,i,pointsLen,group,x,y);
		BoardCellSetColor(&next,x,y,CellColorWhite);
		BoardPrint(&next);
		ret = solve(&next,depth+1);
		
		BoardRelease(&next);
		
		if(ret == 0){
			//白が違ったなら黒。確定して次を試す。
			printf("depth %d:white fail -> black (%d,%d)\n",depth,x,y);
			BoardCellSetColor(board,x,y,CellColorBlack);
			
			BoardPrint(board);
			
		}else{
			//成功を返却する
			break;
		}
	}


	SAFE_FREE(points);
	
	return ret;
}


//0:fail 1:solved
int solve(Board *board,int depth){

	while(1){
		int num = 0;
		
		//エラーが出てる？
		if(board->error){
			return 0;
		}
		num = BoardCellSetBlackSplitWhite(board);
		if(num > 0){
			printf("depth %d , split white +%d \n",depth,num);
			BoardPrint(board);
			continue;
		}
		
		num = BoardCellSetWhiteByBlack2x2(board);
		if(num > 0){
			printf("depth %d , black2x2 white +%d \n",depth,num);
			BoardPrint(board);
			continue;
		}
		
		num = BoardCellExpandBlack(board);
		if(num > 0){
			printf("depth %d , expand black +%d\n",depth,num);
			BoardPrint(board);
			continue;
		}
		
		num = BoardCellExpandWhite(board);
		if(num > 0){
			printf("depth %d , expand white +%d\n",depth,num);
			BoardPrint(board);
			continue;
		}
		
		if(BoardIsSolved(board)){
			printf("depth %d , solved\n",depth);
			return 1;
		}
		
		//以下分岐処理
		
		return solveBranch(board,depth);

	}
	


}

int main(int argc,char *argv[]){
	if(argc<2){
		printf("usage:%s file\n",argv[0]);
		return EXIT_SUCCESS;
	}
	BoardData bData;
	BoardDataInitWithLoadFile(&bData,argv[1]);

	printf("\n");
	
	Board board;
	BoardInitWithData(&board,&bData);

	
	int ret = solve(&board,0);
	

	if(ret){
		printf("solved\n");
		BoardPrint(&board);
	}else{
		printf("solve failed\n");
	}
	
	BoardRelease(&board);
	
	return EXIT_SUCCESS;
}

