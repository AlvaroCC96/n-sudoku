/**************************************************************************************************
 * Copyright (c) 2020. Álvaro Lucas Castillo Calabacero.												                  *
 *                                                                                                *
 *                                                                                                *
 * Licensed under the Apache License, Version 2.0 (the "License");                                *
 * you may not use this file except in compliance with the License.                               *
 * You may obtain a copy of the License at							                              *
 *									                                                              *
 *   http://www.apache.org/licenses/LICENSE-2.0                                                   *
 *																                                  *
 * Unless required by applicable law or agreed to in writing, software                            *
 * distributed under the License is distributed on an "AS IS" BASIS,                              *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.                       *
 * See the License for the specific language governing permissions and                            *
 * limitations under the License.									                              *
 *																		                          *
 **************************************************************************************************/

#include "Tableboard.h"
#include <stdio.h>
#include <stdlib.h>
#include "Tableboard.h"
#include <string.h>
#include <omp.h>

int thread_count = 4;


int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Usage: ./sudoku <thread_count> <inputFile>\n");
		exit(0);
	}
	int** originalGrid = readFile(argv[2]);
	int** gridToSolve = readFile(argv[2]);
	thread_count = atoi(argv[1]);
	if (thread_count <= 0) {
		printf("Usage: Thread Count should be positive\n");
	}
	omp_set_num_threads(thread_count);
	int i, j;
	printf("\n");
	printf("************************INITIAL GAME***********************\n");
	for (i = 0; i < DIMENSIONSUDOKU; i++) {
		for (j = 0; j < DIMENSIONSUDOKU; j++) {
			printf("%d ", originalGrid[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	printf("*********************************************************\n");
	printf("\n");
	double start = omp_get_wtime();
	int** outputGrid = resolver(originalGrid);
	double finish = omp_get_wtime();
	printf("************************FINISH GAME***********************\n");
	printf("\n");
	for (i = 0; i < DIMENSIONSUDOKU; i++) {
		for (j = 0; j < DIMENSIONSUDOKU; j++)
			printf("%d ", outputGrid[i][j]);
		printf("\n");
	}
	printf("\n");
	printf("*********************************************************\n");
	if (validation(originalGrid, outputGrid)) {
		printf("SOLUTION FOUND\nTIME = %lf\n", (finish - start));
	}
	else {
		printf("NO SOLUTION FOUND\nTIME =%lf\n", (finish - start));
	}
}

int validation(int** original, int** solution) {
	int valuesSeen[DIMENSIONSUDOKU], i, j, k;

	//check all rows
	for (i = 0; i < DIMENSIONSUDOKU; i++) {
		for (k = 0; k < DIMENSIONSUDOKU; k++) valuesSeen[k] = 0;
		for (j = 0; j < DIMENSIONSUDOKU; j++) {
			if (solution[i][j] == 0) return 0;
			if ((original[i][j]) && (solution[i][j] != original[i][j])) return 0;
			int v = solution[i][j];
			if (valuesSeen[v - 1] == 1) {
				return 0;
			}
			valuesSeen[v - 1] = 1;
		}
	}

	//check all columns
	for (i = 0; i < DIMENSIONSUDOKU; i++) {
		for (k = 0; k < DIMENSIONSUDOKU; k++) valuesSeen[k] = 0;
		for (j = 0; j < DIMENSIONSUDOKU; j++) {
			int v = solution[j][i];
			if (valuesSeen[v - 1] == 1) {
				return 0;
			}
			valuesSeen[v - 1] = 1;
		}
	}

	//check all minigrids or squares
	for (i = 0; i < DIMENSIONSUDOKU; i = i + SIZEBASE) {
		for (j = 0; j < DIMENSIONSUDOKU; j = j + SIZEBASE) {
			for (k = 0; k < DIMENSIONSUDOKU; k++) valuesSeen[k] = 0;
			int rowAux, colAux;
			for (rowAux = i; rowAux < i + SIZEBASE; rowAux++)
				for (colAux = j; colAux < j + SIZEBASE; colAux++) {
					int v = solution[rowAux][colAux];
					if (valuesSeen[v - 1] == 1) {
						return 0;
					}
					valuesSeen[v - 1] = 1;
				}
		}
	}
	return 1;
}

int** readFile(char* filename) {
	FILE* fileOpened;
	fileOpened = fopen(filename, "r");
	int i, j;
	char dummyline[DIMENSIONSUDOKU + 1];
	char dummy;
	int value;
	int** sudokuGrid = (int**)malloc(sizeof(int*) * DIMENSIONSUDOKU);
	for (i = 0; i < DIMENSIONSUDOKU; i++) {
		sudokuGrid[i] = (int*)malloc(sizeof(int) * DIMENSIONSUDOKU);
		for (j = 0; j < DIMENSIONSUDOKU; j++)
			sudokuGrid[i][j] = 0;
	}

	for (i = 0; i < DIMENSIONSUDOKU; i++) {
		for (j = 0; j < DIMENSIONSUDOKU; j++) {
			/* Checking if number of rows is less than DIMENSIONSUDOKU */
			if (feof(fileOpened)) {
				if (i != DIMENSIONSUDOKU) {
					printf("The input puzzle has less number of rows than %d. Exiting.\n", DIMENSIONSUDOKU);
					exit(-1);
				}
			}

			fscanf(fileOpened, "%d", &value);
			if (value >= 0 && value <= DIMENSIONSUDOKU)
				sudokuGrid[i][j] = value;
			else {
				printf("The input puzzle is not a grid of numbers (0 <= n <= %d) of size %dx%d. Exiting.\n", DIMENSIONSUDOKU, DIMENSIONSUDOKU, DIMENSIONSUDOKU);
				exit(-1);
			}
		}
		fscanf(fileOpened, "%c", &dummy); /* To remove stray \0 at the end of each line */

		/* Checking if row has more elements than SIZE */
		if (j > DIMENSIONSUDOKU) {
			printf("Row %d has more number of elements than %d. Exiting.\n", i + 1, DIMENSIONSUDOKU);
			exit(-1);
		}
	}
	return sudokuGrid;
}



