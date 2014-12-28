#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "board.h"
#include "ai.h"
#include <pthread.h>


void initTTable(){
	tTable = (struct ttEntry*)malloc(sizeof(struct ttEntry)*ttSize);
	for(int i = 0; i < ttSize; i++){
		tTable[i].entry = 0ull;
		tTable[i].depth = 0;
		tTable[i].score = 0;
		pthread_mutex_init(&(tTable[i].mutex), NULL);
	}
}

void storeInTTable(U64 storeBoard, int storeDepth, float storeScore){
	int ind = storeBoard % ttSize;
	if(storeDepth >= tTable[ind].depth){
		pthread_mutex_lock (&(tTable[ind].mutex));
		tTable[ind].entry = storeBoard;
		tTable[ind].depth = storeDepth;
		tTable[ind].score = storeScore;
		pthread_mutex_unlock (&(tTable[ind].mutex));
	}
}

bool probeTTable(U64 board, int depth, float* score){
	int ind = board % ttSize;
	if(tTable[ind].depth >= depth && tTable[ind].entry == board){
		*score = tTable[ind].score;
		return true;
	}
	else{
		return false;
	}
}



float getRowEvalScore(U64 board);

float eval(U64 board, int numEmpty){
	float rowScore = row_scores[board & 65535ull] +
		   row_scores[(board & (65535ull << 16)) >> 16] +
		   row_scores[(board & (65535ull << 32)) >> 32] +
		   row_scores[(board & (65535ull << 48)) >> 48];
	U64 boardt = transpose(board);
	float colScore = row_scores[boardt & 65535ull] +
		   row_scores[(boardt & (65535ull << 16)) >> 16] +
		   row_scores[(boardt & (65535ull << 32)) >> 32] +
		   row_scores[(boardt & (65535ull << 48)) >> 48];
	return numEmpty*40320.0f+ rowScore + colScore;
}


float getRowEvalScore(U64 board){
	float score = 0;
	U64 highest = 0; int highIndex=1;
	// find highest tile and position
	for(int i = 0; i < 16; i += 4){
		U64 tile = getTile(board, i);
		if(tile > highest){
			highest = tile;
			highIndex = i;
		}
	}

	//score high if the highest tile in the row is at the ends
	if(highIndex == 12 || highIndex == 0){
		score += 40320.0f;
	}

	//now go for monotonicity along the row
	bool monoI = true; bool monoD = true;
	U64 tile = getTile(board,0);
	for(int i = 1; i < 4; i++){
		if(getTile(board,i*4) < tile){
			monoI = false;
			break;
		}
		tile = getTile(board,i*4);
	}
	tile = getTile(board,12);
	for(int i = 3; i >= 0; i--){
		if(getTile(board,i*4) < tile){
			monoI = false;
			break;
		}
		tile = getTile(board,i*4);
	}

	if(monoI | monoD){
		score += 40320.0f/10*highest;
	}
	return score;
}

// alpha beta pruning game tree search except one player
// is a random mover whose expected value is taken

// this is the random mover
float stochSearch(U64 board, int depth, U64* nodes, float prob){
	U64 temp = board;
	float score = 0.0f;
	int emptyTiles[N];
	int n = getEmptyTiles(board, emptyTiles);
	prob /= (float)2*n;
	if(prob < PROBTHESH){
		depth = 1;
	}
	//loop over all open spots
	if(depth > 1){
		for(int i = 0; i < n; i++){
			board = setTile(board, 1, emptyTiles[i]*4);
			score += .9f*moveSearch(board, depth-1, nodes, .9f*prob)/n;
			board = temp;
			board = setTile(board, 2, emptyTiles[i]*4);
			score += .1f*moveSearch(board, depth-1, nodes, .1f*prob)/n;
			board = temp;
		}

	}
	else{
		//printf("prob: %f\n", prob);
		for(int i = 0; i < n; i++){
			board = setTile(board, 1, emptyTiles[i]*4);
			score += .9f*eval(board,n-1)/n;
			board = temp;
			board = setTile(board, 2, emptyTiles[i]*4);
			score += .1f*eval(board,n-1)/n;
			board = temp;
		}
		*nodes += n;
	}
	return score;
}

// this is the rational mover (the computer player)
float moveSearch(U64 board, int depth, U64* nodes, float prob){
	//first try the tTable
	float highScore = -40320.0f*32 - depth;
	bool hit = probeTTable(board, depth, &highScore);
	if(hit){
		return highScore;
	}

	U64 temp = board;
	//try each move
	for(int i = 0; i < 4; i++){
		board = moveFuncs[i](board);
		if(temp != board){
			float score = stochSearch(board, depth, nodes, prob);
			if(score > highScore){
				highScore = score;
			}
			board = temp;
		}
		else{
			continue;
		}
	}
	storeInTTable(board, depth, highScore);
	return highScore;
}

// non parallel root search
int rootSearch(U64 board, int depth, float* score, U64* nodes){
	float highScore = -40320.0f*32 - depth; int highIndex = 0;
	U64 temp = board;
	//try each move
	for(int i = 0; i < 4; i++){
		board = moveFuncs[i](board);
		if(temp != board){
			float score = stochSearch(board, depth, nodes, 1.0f);
			if(score > highScore){
				highScore = score;
				highIndex = i;
			}
			board = temp;
		}
		else{
			continue;
		}
	}
	*score = highScore;
	return highIndex;
}

struct thread_data{
	U64 board; int depth; float score; U64 nodes; int move;
};
void* rootSearchMove(void* datav);

// naive parallelism on the root search
int rootSearchParallel(U64 board, int depth, float* score, U64* nodes){
	pthread_t threads[4];
	struct thread_data datas[4];
	for(int i = 0; i < 4; i++){
		datas[i].board = board; datas[i].depth = depth; datas[i].score = 0.0f;
		datas[i].nodes = 0; datas[i].move = i;
	}

	for(int i = 0; i < 4; i++){
		int rc = pthread_create(&threads[i], NULL, rootSearchMove, (void *) &(datas[i]));
		 if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
      	}
	}

	for(int i = 0; i < 4; i++){
		pthread_join(threads[i], NULL);
	}

	float highScore = -40320.0f*32 - depth; int highIndex = 0;
	//try each move
	for(int i = 0; i < 4; i++){
		*nodes = *nodes + datas[i].nodes;
		if(datas[i].score > highScore){
			highScore = datas[i].score;
			highIndex = i;
		}
	}
	*score = highScore;
	//pthread_exit(NULL);
	return highIndex;
}

// new threads are sent here
void* rootSearchMove(void* datav){
	struct thread_data* data = (struct thread_data*) datav;
	if(data->move < 0){
		return NULL;
	}
	U64 temp = data->board;
	data->board = moveFuncs[data->move](data->board);
	data->score = -40320.0f*32 - data->depth;
	if(temp != data->board){
		data->score = stochSearch(data->board, data->depth, &(data->nodes), 1.0f);
	}
	//printf("thread launched %i\n", data->move);
	return NULL;
}