#if !defined(BOARD_h)
#define BOARD_h

typedef unsigned long long U64;
#define N 16 //number of tiles
#define sN 4 //number of tiles on a side

void printBoard(U64 board);
void initMoveTables();
U64 moveDown(U64 board);
U64 moveUp(U64 board);
U64 moveLeft(U64 board);
U64 moveRight(U64 board);
U64 transpose(U64 x);
U64 reverseRow(U64 row);
U64 getMoveResultOnRow(U64 board);
void freeMoveTables();
int getEmptyTiles(U64 board, int buffer[N]);
U64 getRandomTile();
U64 setTile(U64 board, U64 tile, int ind);
U64 getTile(U64 board, int ind);
U64 spawnRandomTile(U64 board);
int getNumEmptyTiles(U64 board);

U64* left_row_table, *right_row_table, *up_col_table, *down_col_table;
float* row_scores;
extern U64 (*moveFuncs[4]) (U64 board);

#endif