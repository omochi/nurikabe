#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "OMMem.h"

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
	OMFree(v);\
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
	BoardPrintGroup = 1,
	BoardPrintChain = 2,
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
	int markDist;//マークした時のdist
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

static Board *BoardArrayAlloc(int reserve);
static void BoardArrayInsert(Board *array,int *len,int *reserve,int idx,Board *add);
static void BoardArrayRemove(Board *array,int *len,int *reserve,int idx);
static int BoardArrayFindByScore(Board *array,int *len,int score);
static int BoardArrayFindSame(Board *array,int *len,Board *find); 
static Point PointMake(int x,int y);
static void CellPtrArrayAddUniqueGroup(Cell **array,int *n,Cell *add);
static void CellPtrArrayAddUnique(Cell **array,int *n,Cell *add);
static int CellPtrArrayFind(Cell **array,int *n,Cell *find);
static void CellPtrArrayRemove(Cell **array,int *n,int idx);
static void CellPtrArrayInsert(Cell **array,int *n,int idx,Cell *val);
static int CellPtrArrayFindByDist(Cell **array,int *n,int dist);
static Cell **BoardAllocCellPtrArray(Board *this);
static int *BoardDataGetDataPtr(BoardData *this,int x,int y);
static int BoardDataGetData(BoardData *this,int x,int y);
static void BoardDataSetData(BoardData *this,int x,int y,int v);
static void BoardDataInit(BoardData *this,int w,int h);
static void BoardDataInitWithLoadFile(BoardData *this,char *path);
static void BoardDataRelease(BoardData *this);
void BoardDataPrint(BoardData *this);
static void CellInit(Cell *this,int x,int y);
static Cell *BoardGetCellPtr(Board *this,int x,int y);
static Cell *BoardGetThisCellPtr(Board *this,Cell *cell);
static int BoardCellGetHeadData(Board *this,Cell *cell);
static int BoardIsSame(Board *lhs,Board *rhs);
static void BoardCellChangeGroup(Board *this,Cell *groupCell,int to,Point head, int chainNum);
static void BoardCellGroupJoin(Board *this,Cell *lhs,Cell *rhs);
static void BoardCellSetBlackAroundWhite(Board *this,int group);
static void BoardCellWhiteChainOnChanged(Board *this,Cell *cell);
static void BoardCellSetColor(Board *this,Cell *cell,CellColor color);
static void BoardCellAddToWhiteGroup(Board *this,Cell *add,Cell *group);
static int BoardCellsCantChainWhite(Board *this,Cell *lhs,Cell *rhs);
static int BoardCell4CantChainWhite(Board *this,Cell *c0,Cell *c1,Cell *c2,Cell *c3);
static void BoardGetCellsOfAroundCell(Board *this,Cell *result[8],Cell *cell);
static int CellPtrArrayCheck3Color(Cell **cells,int i0,int i1,int i2,int color);
static int CellPtrArrayCheckAroundBlackL(Cell *cells[8]);
static int BoardCellSetBy3x3(Board *this);
static void BoardCellAreaSetMarkByColor(Board *this,Cell *cell,int mark,int color);
static void BoardGetCellAreaByColor(Board *this,Cell **result,int *resultLen,Cell *cell,int color);
static int BoardCellIsValueWhite(Board *this,Cell *cell);
static int BoardGetCellsOfValueWhite(Board *this,Cell **result,int *resultLen);
static void BoardClearCellDist(Board *this);
static void BoardClearCellDistWithMask(Board *this,int markMask);
static void BoardClearCellMark(Board *this);
static void BoardSetCellMarkUniqueGroup(Board *this,Cell *cell,Cell *mark);
static int BoardCellIsAroundAnotherValueWhite(Board *this,Cell *cell,int group);
static void BoardCalcGroupDistanceAddToOpen(Board *this,Cell **open,int *openLen,Cell *start,Cell *prev,Cell *next);
static int BoardGetCellsOfWhiteExpansionFromDist(Board *this,Cell **result,int *resultLen,int group);
static int BoardGetCellsOfGroup(Board *this,Cell **result,int *resultLen,int group);
static void BoardCalcGroupDistance(Board *this,Cell *start);
//static int BoardGetGroupsByCellWithColor(Board *this,Cell **result,int *retNum,int color);
static int BoardGetCellsOfMarkAroundColor(Board *this,Cell **result,int *num,int mark,int color);
//static int BoardGetCellsOfGroupAroundColor(Board *this,Cell **result,int *num,int group,int color);
static int BoardCellExpandWhite(Board *this);
static int BoardCellExpandBlack(Board *this);
static void BoardCheckBlackSplit(Board *this);
static void BoardWhiteCellAreaMark(Board *this,Cell **result,int *num);
static void BoardBlackCellAreaMark(Board *this,Cell **result,int *num);
static void BoardCellSetMarkBackDist(Board *this,Cell *cell,int mark);
static int BoardCellSetWhiteByDistBack(Board *this,Cell *from,Cell *to);
static int BoardCellSetBlackUnreachable(Board *this);
static void BoardInitWithData(Board *this,BoardData *data);
static void BoardInitWithBoard(Board *this,Board *src);
static void BoardRelease(Board *this);
static void BoardIndent(Board *this);
void BoardPrintWithFlag(Board *this,int flags);
void BoardPrint(Board *this);
void BoardPrintDist(Board *this);
void BoardPrintMark(Board *this);
static int BoardIsSolved(Board *this);
static int BoardCalcGrayCellNum(Board *this);
static int BoardCellGetWhiteGroupRemain(Board *this,Cell *cell);
static int BoardGetScore(Board *this);
static int BoardGetCellsOfWhiteNextExpansion(Board *this,Cell **result,int *resultLen);
 int BoardGetCellsOfBlackNextExpansion(Board *this,Cell **result,int *resultLen);
static int solveSingle(Board *board);
static void solveAddToOpen(Board *node);
static int solveBranch(Board *board);
static int solve();

