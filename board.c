#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "board.h"
#include "ai.h"

U64 (*moveFuncs[4]) (U64 board) = {moveRight, moveUp, moveLeft, moveDown};

void printBoard(U64 board){
	for(int i = 0; i < sN; i++){
		for(int j = 0; j < sN; j++){
			int ind = (i*sN+j)*4;
			if(getTile(board,ind) == 0){
				printf("%5i  ", 0);
			}
			else{
				printf("%5i  ", 1 << (15ull & (board >> ind)));
			}
		}
		printf("\n");
	}
	printf("\n");
}

U64 getTile(U64 board, int ind){
	return 15ull & (board >> ind);
}

U64 setTile(U64 board, U64 tile, int ind){
	board = (board & ~(15ull << ind)) | (tile << ind);
	return board;
}

//row has the first 16 LSBs set to a row on the board, the function slides from MSB to LSB (left)
//used to generate the row tables
U64 getMoveResultOnRow(U64 board){
	//first slide
	for(int i = 1; i < 4; i++){
		int ns = 1;
		U64 tile = getTile(board,i*4);
		if(tile != 0){
			while(i-ns >= 0 && getTile(board, 4*(i-ns)) == 0 && (i-ns)/sN == i/sN){
				ns++;
			}
			if(ns != 1){
				board = setTile(board, 0, i*4);
				board = setTile(board, tile, 4*(i - ns + 1));
			}
		}
	}
	//now combine
	for(int i = 1; i < 4; i++){
		U64 tile = getTile(board, i*4);
		if(tile != 0){
			U64 leftTile = getTile(board, 4*(i-1));
			if(leftTile == tile){
				board = setTile(board, leftTile+1,4*(i-1));
				board = setTile(board,0,4*i);
			}
		}
	}
	//now slide again
	for(int i = 1; i < 4; i++){
		int ns = 1;
		U64 tile = getTile(board,4*i);
		if(tile != 0){
			while(i-ns >= 0 && getTile(board, 4*(i-ns)) == 0 && (i-ns)/sN == i/sN){
				ns++;
			}
			if(ns != 1){
				board = setTile(board, 0, 4*i);
				board = setTile(board, tile, 4*(i - ns + 1));
			}
		}
	}
	return board;
}

U64 reverseRow(U64 row){
	return ((row&15ull)<<12)|((row&(15ull<<4))<<4)|((row&(15ull<<8))>>4)|((row&(15ull<<12))>>12);
}

//credit to bit hacks found online
U64 transpose(U64 x){
	U64 a1 = x & 0xF0F00F0FF0F00F0FULL;
	U64 a2 = x & 0x0000F0F00000F0F0ULL;
	U64 a3 = x & 0x0F0F00000F0F0000ULL;
	U64 a = a1 | (a2 << 12) | (a3 >> 12);
	U64 b1 = a & 0xFF00FF0000FF00FFULL;
	U64 b2 = a & 0x00FF00FF00000000ULL;
	U64 b3 = a & 0x00000000FF00FF00ULL;
	return b1 | (b2 >> 24) | (b3 << 24);
}

//use generated move tables to move quickly
U64 moveRight(U64 board){
	return right_row_table[board & 65535ull] |
		   (right_row_table[(board & (65535ull << 16)) >> 16] << 16) |
		   (right_row_table[(board & (65535ull << 32)) >> 32] << 32) | 
		   (right_row_table[(board & (65535ull << 48)) >> 48] << 48);
}
U64 moveLeft(U64 board){
	return left_row_table[board & 65535ull] |
		   (left_row_table[(board & (65535ull << 16)) >> 16] << 16) |
		   (left_row_table[(board & (65535ull << 32)) >> 32] << 32) | 
		   (left_row_table[(board & (65535ull << 48)) >> 48] << 48);
}
U64 moveUp(U64 board){
	U64 boardt = transpose(board);
	return up_col_table[boardt & 65535ull] |
		   (up_col_table[(boardt & (65535ull << 16)) >> 16] << 4) |
		   (up_col_table[(boardt & (65535ull << 32)) >> 32] << 8) | 
		   (up_col_table[(boardt & (65535ull << 48)) >> 48] << 12);

}
U64 moveDown(U64 board){
	U64 boardt = transpose(board);
	return down_col_table[boardt & 65535ull] |
		   (down_col_table[(boardt & (65535ull << 16)) >> 16] << 4) |
		   (down_col_table[(boardt & (65535ull << 32)) >> 32] << 8) | 
		   (down_col_table[(boardt & (65535ull << 48)) >> 48] << 12);

}

//gets a 2 or a 4
U64 getRandomTile(){
	return (U64)(rand()%10 == 0 ? (2) : (1));
}

//returns number of empty tiles and array with their indices
int getEmptyTiles(U64 board, int buffer[N]){
	int n = 0;
	for(int i = 0; i < N; i++){
		if(getTile(board, i*4) == 0){
			buffer[n]=i;
			n++;
		}
	}
	return n;
}

int getNumEmptyTiles(U64 board){
	int n = 0;
	for(int i = 0; i < N; i++){
		if(getTile(board, i*4) == 0){
			n++;
		}
	}
	return n;
}

U64 spawnRandomTile(U64 board){
	int emptyTiles[N];
	int n = getEmptyTiles(board, emptyTiles);
	return setTile(board, getRandomTile(), emptyTiles[rand() % n]*4);
}

void initMoveTables(){
	left_row_table = (U64*)malloc(sizeof(U64)*16*16*16*16);
	right_row_table = (U64*)malloc(sizeof(U64)*16*16*16*16);
	up_col_table = (U64*)malloc(sizeof(U64)*16*16*16*16);
	down_col_table = (U64*)malloc(sizeof(U64)*16*16*16*16);
	row_scores = (float*)malloc(sizeof(float)*16*16*16*16);
	// I think there is a much much simpler way to do this
	// aren't I just looping row through all numbers 0 to 2^16 - 1?
	for(U64 a = 0; a < 16; a++){
		for(U64 b = 0; b < 16; b++){
			for(U64 c = 0; c < 16; c++){
				for(U64 d = 0; d < 16; d++){
					U64 row = 0;
					row = setTile(row, a, 0);
					row = setTile(row, b, 4);
					row = setTile(row, c, 8);
					row = setTile(row, d, 12);
					U64 rev = reverseRow(row);
					
					U64 res = getMoveResultOnRow(row);
					left_row_table[(int)row] = res;
					right_row_table[(int)rev] = reverseRow(res);
					up_col_table[(int)row] = transpose(res);
					down_col_table[(int)rev] = transpose(reverseRow(res));
					row_scores[(int)row] = getRowEvalScore(row);
				}
			}
		}
	}
}

void freeMoveTables(){
	free(left_row_table);
	free(right_row_table);
	free(up_col_table);
	free(down_col_table);
}