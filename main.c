#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "board.h"
#include "ai.h"



int main(){
	struct timeval start, end;
    long mtime, seconds, useconds;  
    initMoveTables();
    initTTable();
    srand(time(NULL));
	U64 board = 0ull;
	// start with two random tiles
	for(int i = 0; i < 2; i++){
		board = spawnRandomTile(board);
	}
	printBoard(board);
	int numGames = 1; // set to higher to play more than one game
	U64 endGames[numGames];
	int highest[numGames]; //highest tile in each game
	for(int num = 0; num < numGames; num++){
		U64 temp = 0ull;
		board = temp;
		for(int i = 0; i < 2; i++){
			board = spawnRandomTile(board);
		}
		bool gameOver = false;
		while(!gameOver){
			temp = board;
		
			float score = 0;
			U64 nodes = 0ull;
			gettimeofday(&start, NULL);
			int depth = 10;
			int move = rootSearchParallel(board, depth, &score, &nodes);
			gettimeofday(&end, NULL);
			seconds  = end.tv_sec  - start.tv_sec;
    		useconds = end.tv_usec - start.tv_usec;
    		mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
    		printf("Elapsed time: %ld milliseconds. %f NPS \n", mtime, ((float)nodes)/((float)mtime)*1000);
			printf("move: %i %f %llu %i\n", move, score, nodes, num);
			board = moveFuncs[move](board);
			if(temp != board){ //move successful
				board = spawnRandomTile(board);
			}
			else{ //there must be no valid moves, end game
				gameOver = true;
				endGames[num] = board;
				int highTile = 0;
				for(int k = 0; k < 16; k++){
					if(getTile(board,k*4) > highTile){
						highTile=getTile(board,k*4);
					}
				}
				highest[num] = highTile;
				printf("HIGH TILE IS: %i\n\n", 1<<highTile);
			}
			printBoard(board);
		}
	}
	//print results
	for(int num = 0; num < numGames; num++){
		printf("HIGH TILE IS: %i\n\n", 1<<highest[num]);
		printBoard(endGames[num]);
	}


	freeMoveTables();
}
