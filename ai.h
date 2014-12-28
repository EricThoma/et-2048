#if !defined(AI_h)
#define AI_h
#include <pthread.h>
int rootSearch(U64 board, int depth, float* score, U64* nodes);
float moveSearch(U64 board, int depth, U64* nodes, float prob);
float stochSearch(U64 board, int depth, U64* nodes, float prob);
float eval(U64 board, int numEmpty);
void initTTable();
void storeInTTable(U64 storeBoard, int storeDepth, float storeScore);
bool probeTTable(U64 board, int depth, float* score);
float getRowEvalScore(U64 board);
int rootSearchParallel(U64 board, int depth, float* score, U64* nodes);

struct ttEntry{
	U64 entry;
	int depth;
	float score;
	pthread_mutex_t mutex;
};

#define ttSize 20000
#define PROBTHESH (.00002f)

struct ttEntry* tTable;

#endif