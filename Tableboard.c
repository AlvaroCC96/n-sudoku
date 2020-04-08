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
#include <string.h>
#include <assert.h>
#include <omp.h> 


#define sudokuVar long long
#define true 1
#define false 0
#define MAX_THREADS 8 //can change to 16 and max MAX_NESTED_THREADS same
#define MAX_NESTED_THREADS 8
typedef int bool;

//queue
struct queue
{
	int* array;
	int size;
};

//sudoku
struct SudokuTable
{
	sudokuVar** squares;
	sudokuVar** registry;
	sudokuVar** countElems;
	int row;
	int col;
	sudokuVar val;
	struct SudokuTable* child;
};


//global variables
struct queue q;
omp_lock_t globalLock;
omp_lock_t ret;
omp_lock_t locks[MAX_THREADS];
bool active[MAX_THREADS];
struct SudokuTable* indexPointers[MAX_THREADS];
int** output;
bool exiting;
int depth;

bool isEmpty(struct queue* q)
{
	return (q->size == 0);
}

void Push(struct queue* q, int put)
{
	q->array[q->size] = put;
	q->size++;
}

int Pop(struct queue* q)
{
	q->size--;
	return q->array[q->size];
}

void Init(struct queue* q)
{
	q->array = (int*)malloc(MAX_THREADS * sizeof(int));
	q->size = 0;
}

void InitAll(struct SudokuTable* input)
{
	int i;

	//dynamic memory allocation
	input->squares = (sudokuVar**)malloc(sizeof(sudokuVar*) * DIMENSIONSUDOKU);
	input->registry = (sudokuVar**)malloc(sizeof(sudokuVar*) * DIMENSIONSUDOKU);
	input->countElems = (sudokuVar**)malloc(sizeof(sudokuVar*) * DIMENSIONSUDOKU);

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		input->squares[i] = (sudokuVar*)malloc(sizeof(sudokuVar) * DIMENSIONSUDOKU);
		input->registry[i] = (sudokuVar*)malloc(sizeof(sudokuVar) * DIMENSIONSUDOKU);
		input->countElems[i] = (sudokuVar*)malloc(sizeof(sudokuVar) * DIMENSIONSUDOKU);
	}
}

void copy(struct SudokuTable* dest, struct SudokuTable* src)
{
	dest->val = src->val;
	dest->row = src->row;
	dest->col = src->col;

	int i;

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{	 //copy "n" bytes for origin position to destiny position
		memcpy(dest->squares[i], src->squares[i], sizeof(sudokuVar) * DIMENSIONSUDOKU);
		memcpy(dest->countElems[i], src->countElems[i], sizeof(sudokuVar) * DIMENSIONSUDOKU);
		memcpy(dest->registry[i], src->registry[i], sizeof(sudokuVar) * DIMENSIONSUDOKU);
	}
}

void freeAll(struct SudokuTable* input)
{
	//free memory spaces
	for (int i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		free(input->squares[i]);
		free(input->registry[i]);
		free(input->countElems[i]);
	}

	free(input->squares);
	free(input->registry);
	free(input->countElems);
}

void bin(sudokuVar i)
{	//print value  
	sudokuVar a = 0;
	int j = DIMENSIONSUDOKU;
	while (j--)
	{
		a = i;
		i = i >> 1;
		printf("%lld ", a & 0x01);
	}
}


void squaresPrintBin(sudokuVar** squares)
{
	printf("starts here something bin \n\n");
	int i, j, k;

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		for (j = 0; j < DIMENSIONSUDOKU; ++j)
		{
			bin(squares[i][j]);
			printf("\n");
		}
		printf("\n\n\n");
	}
}

void squaresPrint(sudokuVar** squares)
{
	printf("starts here something \n\n");
	int i, j, k;

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		for (j = 0; j < DIMENSIONSUDOKU; ++j)
		{
			printf("%lld  ", squares[i][j]);
		}
		printf("\n\n");
	}
}

void PrintAll(struct SudokuTable* input)
{
	squaresPrintBin(input->squares);
	squaresPrint(input->registry);
	squaresPrint(input->countElems);
}


void solutionsAvaible(struct SudokuTable* input, int row, int col)
{
	int i, j, k;

	assert(input->countElems[row][col] > 1);

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		//row
		if (i != col && input->countElems[row][i] == 1)
		{
			input->squares[row][col] &= ~(1LL << input->registry[row][i]);
		}
		if (i != row && input->countElems[i][col] == 1)
		{
			input->squares[row][col] &= ~(1LL << input->registry[i][col]);
		}
	}

	int rowLimit = SIZEBASE + (row / SIZEBASE) * SIZEBASE;
	int colLimit = SIZEBASE + (col / SIZEBASE) * SIZEBASE;

	for (i = (row / SIZEBASE) * SIZEBASE; i < rowLimit; ++i)
	{
		for (j = (col / SIZEBASE) * SIZEBASE; j < colLimit; ++j)
		{
			if (i != row && j != col && input->countElems[i][j] == 1)
			{
				input->squares[row][col] &= ~(1LL << input->registry[i][j]);
			}
		}
	}

	input->countElems[row][col] = 0;
	sudokuVar val = input->squares[row][col];
	k = 0;
	while (val > 0)
	{
		sudokuVar updt = (val & 1LL);
		input->countElems[row][col] += updt;
		val = (val >> 1);
		k++;
	}

}


void update(struct SudokuTable* input, int row, int col)
{
	sudokuVar val = (input->countElems[row][col] == 1) ? input->squares[row][col] : 0;
	sudokuVar k = -1;
	while (val > 0)
	{
		val = (val >> 1);
		k++;
	}
	input->registry[row][col] = k;
}

bool loneRanger(struct SudokuTable* input, int row, int col, sudokuVar index);

bool exclusion(struct SudokuTable* input, int row, int col);

bool elimination(struct SudokuTable* input, int row, int col)
{
	int i, j, k;

	assert(input->countElems[row][col] == 1);

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		if (i != col && (input->squares[row][i] >> input->registry[row][col]) & 1LL == 1)
		{
			input->squares[row][i] &= ~(1LL << input->registry[row][col]);
			input->countElems[row][i] -= 1;
			if (input->countElems[row][i] == 1)
			{
				update(input, row, i);
				if (!elimination(input, row, i))return false;
			}
			if (input->countElems[row][i] < 1)
			{
				return false;
			}
			if (!loneRanger(input, -1, i, input->registry[row][col]))return false;
		}
		if (i != row && (input->squares[i][col] >> input->registry[row][col]) & 1LL == 1)
		{
			input->squares[i][col] &= ~(1LL << input->registry[row][col]);
			input->countElems[i][col] -= 1;
			if (input->countElems[i][col] == 1)
			{
				update(input, i, col);
				if (!elimination(input, i, col))return false;
			}
			else if (input->countElems[i][col] < 1)
			{
				return false;
			}
			if (!loneRanger(input, i, -1, input->registry[row][col]))return false;
		}
	}

	int rowLimit = SIZEBASE + (row / SIZEBASE) * SIZEBASE;
	int colLimit = SIZEBASE + (col / SIZEBASE) * SIZEBASE;

	for (i = (row / SIZEBASE) * SIZEBASE; i < rowLimit; ++i)
	{
		for (j = (col / SIZEBASE) * SIZEBASE; j < colLimit; ++j)
		{
			if (i != row && j != col && (input->squares[i][j] >> input->registry[row][col]) & 1LL == 1)
			{
				input->squares[i][j] &= ~(1LL << input->registry[row][col]);
				input->countElems[i][j] -= 1;
				if (input->countElems[i][j] == 1)
				{
					update(input, i, j);
					if (!elimination(input, i, j))return false;
				}
				else if (input->countElems[i][j] < 1)
				{
					return false;
				}
				if (!loneRanger(input, i, j, input->registry[row][col]))return false;
			}
		}
	}

	return true;
}


bool exclusion(struct SudokuTable* input, int row, int col)
{
	if (input->countElems[row][col] > 3)return true;

	//exclude the similars positions and values.
	int i, j, k;
	bool whereR[DIMENSIONSUDOKU];
	bool whereC[DIMENSIONSUDOKU];
	sudokuVar countR = 0;
	sudokuVar countC = 0;

	sudokuVar possibleVals[DIMENSIONSUDOKU];
	sudokuVar val = input->squares[row][col];
	assert(val > 0);
	k = 0;
	j = 0;
	while (j < input->countElems[row][col])
	{
		assert(val > 0);
		if ((val & 1LL) == 1)
		{
			possibleVals[j] = (sudokuVar)k;
			j++;
		}
		val = val >> 1;
		k++;
	}

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		if (input->squares[row][col] == input->squares[row][i])
		{
			whereC[i] = 1;
			countC++;
		}
		else
		{
			whereC[i] = 0;
		}
	}

	if (countC == input->countElems[row][col])
	{
		for (i = 0; i < DIMENSIONSUDOKU; ++i)
		{
			if (whereC[i] == 0)
			{
				for (j = 0; j < input->countElems[row][col]; ++j)
				{
					if ((input->squares[row][i] & (1LL << possibleVals[j])) > 0)
					{
						input->squares[row][i] &= ~(1LL << possibleVals[j]);
						input->countElems[row][i] -= 1;
						if (input->countElems[row][i] == 1)
						{
							update(input, row, i);
						}
					}
				}
			}
		}
	}
	else if (countC > input->countElems[row][col])
	{
		return false;
	}


	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		if (input->squares[row][col] == input->squares[i][col])
		{
			whereR[i] = 1;
			countR++;
		}
		else
		{
			whereR[i] = 0;
		}
	}

	if (countR == input->countElems[row][col])
	{
		for (i = 0; i < DIMENSIONSUDOKU; ++i)
		{
			if (whereR[i] == 0)
			{
				for (j = 0; j < input->countElems[row][col]; ++j)
				{
					if ((input->squares[i][col] & (1LL << possibleVals[j])) > 0)
					{
						input->squares[i][col] &= ~(1LL << possibleVals[j]);
						input->countElems[i][col] -= 1;
						if (input->countElems[i][col] == 1)
						{
							update(input, i, col);
						}
					}
				}
			}
		}
	}
	else if (countR > input->countElems[row][col])
	{
		return false;
	}


	sudokuVar count = 0;
	bool where[SIZEBASE][SIZEBASE];

	int rowLimit = SIZEBASE + (row / SIZEBASE) * SIZEBASE;
	int colLimit = SIZEBASE + (col / SIZEBASE) * SIZEBASE;

	for (i = (row / SIZEBASE) * SIZEBASE; i < rowLimit; ++i)
	{
		for (j = (col / SIZEBASE) * SIZEBASE; j < colLimit; ++j)
		{
			if (input->squares[row][col] == input->squares[i][j])
			{
				where[i - rowLimit + SIZEBASE][j - colLimit + SIZEBASE] = 1;
				count++;
			}
			else
			{
				where[i - rowLimit + SIZEBASE][j - colLimit + SIZEBASE] = 0;
			}
		}
	}


	if (count == input->countElems[row][col])
	{
		for (i = (row / SIZEBASE) * SIZEBASE; i < rowLimit; ++i)
		{
			for (j = (col / SIZEBASE) * SIZEBASE; j < colLimit; ++j)
			{
				if (where[i - rowLimit + SIZEBASE][j - colLimit + SIZEBASE] == 0)
				{
					for (k = 0; k < input->countElems[row][col]; ++k)
					{
						if ((input->squares[i][j] & (1LL << possibleVals[k])) > 0)
						{
							input->squares[i][j] &= ~(1LL << possibleVals[k]);
							input->countElems[i][j] -= 1;
							if (input->countElems[i][j] == 1)
							{
								update(input, i, j);
							}
						}
					}
				}
			}
		}
	}
	else if (count > input->countElems[row][col])
	{
		return false;
	}

	return true;

}


bool loneRanger(struct SudokuTable* input, int row, int col, sudokuVar index)
{
	int i, j, k;
	sudokuVar sum = 0;
	int whereR, whereC;
	sudokuVar dummy;
	if (col == -1)
	{
		whereR = row;
		for (i = 0; i < DIMENSIONSUDOKU; ++i)
		{
			dummy = (input->squares[row][i] >> index) & 1LL;
			sum += dummy;
			whereC = (dummy == 1) ? i : whereC;
			if (sum > 1)return true;
		}
	}
	else if (row == -1)
	{
		whereC = col;
		for (i = 0; i < DIMENSIONSUDOKU; ++i)
		{
			dummy = (input->squares[i][col] >> index) & 1LL;
			sum += dummy;
			whereR = (dummy == 1) ? i : whereR;
			if (sum > 1)return true;
		}
	}
	else
	{
		int rowLimit = SIZEBASE + (row / SIZEBASE) * SIZEBASE;
		int colLimit = SIZEBASE + (col / SIZEBASE) * SIZEBASE;

		for (i = (row / SIZEBASE) * SIZEBASE; i < rowLimit; ++i)
		{
			for (j = (col / SIZEBASE) * SIZEBASE; j < colLimit; ++j)
			{
				dummy = (input->squares[i][j] >> index) & 1LL;
				sum += dummy;
				whereR = (dummy == 1) ? i : whereR;
				whereC = (dummy == 1) ? j : whereC;
				if (sum > 1)return true;
			}
		}
	}
	if (sum == 1)
	{
		input->squares[whereR][whereC] = 1LL << index;
		input->countElems[whereR][whereC] = 1;
		input->registry[whereR][whereC] = index;
		if (!elimination(input, whereR, whereC))return false;
		return true;
	}
	else if (sum < 1)
	{
		return false;
	}

	return true;
}


void rec(int thread_id, struct SudokuTable* input)
{
	if (exiting)
	{
		return;
	}
	if (input->row != DIMENSIONSUDOKU && input->col != DIMENSIONSUDOKU)
	{
		input->squares[input->row][input->col] = 1LL << input->val;
		input->countElems[input->row][input->col] = 1;
		input->registry[input->row][input->col] = input->val;
		if (!elimination(input, input->row, input->col))return;
	}
	int i, j, k;
	int row = DIMENSIONSUDOKU;
	int col = DIMENSIONSUDOKU;
	int minElems = DIMENSIONSUDOKU + 1;
	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		for (j = 0; j < DIMENSIONSUDOKU; ++j)
		{
			if (minElems > input->countElems[i][j] && input->countElems[i][j] > 1)
			{
				minElems = input->countElems[i][j];
				row = i;
				col = j;
			}
			else if (input->countElems[i][j] < 1)
			{
				return;
			}
		}
	}
	if (row == DIMENSIONSUDOKU && col == DIMENSIONSUDOKU)
	{
		omp_set_lock(&ret);
		for (i = 0; i < DIMENSIONSUDOKU; ++i)
		{
			for (j = 0; j < DIMENSIONSUDOKU; ++j)
			{
				output[i][j] = (int)input->registry[i][j] + 1;
			}
		}
		exiting = 1;
		omp_unset_lock(&ret);
		return;
	}

	sudokuVar possibleVals[DIMENSIONSUDOKU];
	sudokuVar val = input->squares[row][col];
	assert(val > 0);
	k = 0;
	j = 0;
	while (j < minElems)
	{
		assert(val > 0);
		if (val & 1 == 1)
		{
			possibleVals[j] = (sudokuVar)k;
			j++;
		}
		val = val >> 1;
		k++;
	}

	int st = 0;

	if (!isEmpty(&q))
	{
		omp_set_lock(&globalLock);
		if (!isEmpty(&q))
		{
			int j = Pop(&q);
			omp_unset_lock(&globalLock);
			input->row = row;
			input->col = col;
			input->val = possibleVals[0];
			copy(indexPointers[j], input);
			active[j] = 1;
			//give some work
			omp_unset_lock(&locks[j]);
			st++;
		}
		else
		{
			omp_unset_lock(&globalLock);
		}
	}

	input->row = row;
	input->col = col;

	for (k = st; k < minElems; ++k)
	{
		input->val = possibleVals[k];
		copy(input->child, input);
		depth += 1;
		rec(thread_id, input->child);
		depth -= 1;
	}

}


int** resolver(int** originalsquares)
{
	depth = 0;

	struct SudokuTable input[MAX_THREADS];

	int i, j, k;

	output = (int**)malloc(sizeof(int*) * DIMENSIONSUDOKU);

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		output[i] = (int*)malloc(sizeof(int) * DIMENSIONSUDOKU);
	}

	for (i = 0; i < MAX_THREADS; ++i)
	{
		InitAll(&input[i]);
		struct SudokuTable* children = &input[i];
		for (j = 0; j < DIMENSIONSUDOKU * DIMENSIONSUDOKU + 1; ++j)
		{
			//form children till depth DIMENSIONSUDOKU*DIMENSIONSUDOKU
			children->child = malloc(sizeof(struct SudokuTable));
			InitAll(children->child);
			children = children->child;
		}
	}


	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{

		for (j = 0; j < DIMENSIONSUDOKU; ++j)
		{

			if (originalsquares[i][j] == 0)
			{
				input[0].squares[i][j] = (1LL << DIMENSIONSUDOKU) - 1;
			}
			else
			{
				input[0].squares[i][j] = 1LL << (originalsquares[i][j] - 1);
			}

			input[0].registry[i][j] = (originalsquares[i][j] == 0) ? -1 : originalsquares[i][j] - 1;
			input[0].countElems[i][j] = (originalsquares[i][j] == 0) ? DIMENSIONSUDOKU : 1;

		}
	}

	for (i = 0; i < DIMENSIONSUDOKU; ++i)
	{
		for (j = 0; j < DIMENSIONSUDOKU; ++j)
		{
			if (input[0].countElems[i][j] == 1)
				if (!elimination(&input[0], i, j))
				{
					exiting = true;
				}
		}
	}
	input[0].row = DIMENSIONSUDOKU;
	input[0].col = DIMENSIONSUDOKU;
	input[0].val = DIMENSIONSUDOKU;
	//mapping done


	fflush(stdout);


	Init(&q);
	omp_init_lock(&globalLock);
	omp_init_lock(&ret);
	exiting = 0;

	for (i = 0; i < MAX_THREADS; ++i)
	{
		indexPointers[i] = &input[i];
		active[i] = 0;
		omp_init_lock(&locks[i]);
	}
	active[0] = 1;

#pragma omp parallel num_threads(MAX_THREADS) private(i,j,k)
	{
#pragma omp master
		{
			for (j = 1; j < MAX_THREADS; ++j)
			{
				Push(&q, j);
				omp_set_lock(&locks[j]);
			}
		}

#pragma omp barrier

		int thread_id = omp_get_thread_num();
		bool flag = 1;

		while (flag)
		{
			if (active[thread_id] == 1)
			{

				rec(thread_id, &input[thread_id]);

				omp_set_lock(&locks[thread_id]);
				active[thread_id] = 0;
				omp_set_lock(&globalLock);
				Push(&q, thread_id);
				omp_unset_lock(&globalLock);
			}
			else
			{
				omp_set_lock(&locks[thread_id]);
				omp_unset_lock(&locks[thread_id]);
			}
			flag = 0;
			for (j = 0; j < MAX_THREADS; ++j)
			{
				flag |= active[j];
			}
		}


#pragma omp single
		{
			for (j = 0; j < MAX_THREADS; ++j)
			{
				omp_unset_lock(&locks[j]);
			}
		}
	}

	return output;
}
