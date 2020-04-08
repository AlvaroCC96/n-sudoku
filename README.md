## OpenMP Implementation

OpenMP implementation to solve sudoku 16x16 for HPC course from UCN.

- ### For Compile
`gcc -g Sudoku.c Tableboard.c -O3 -fopenmp -o sudoku`

- ### For Run
`./sudoku N_Threads values.txt` 
Defatult threads = 4 in case you write a negative number
Format `N_Threads` is Integer: 1,2,3,... 16

- ### SpeedRun

![alt text](https://github.com/AlvaroCC96/n-sudoku/blob/master/SpeedRun.png "SpeedRun")

### Contact.
alvarolucascc96@gmail.com
