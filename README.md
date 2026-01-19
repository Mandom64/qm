# Quine McCluskey Solver

A function that solves the Quine McCluskey problem, commonly found in **digital logic design** and **boolean algebra**. Written in C and parallelized using **OpenMP**.

# How to run
To compile with OpenMP, use:

```bash
gcc -Wall -Wextra -fopenmp src/qm.c -o bin/qm.exe
```

# Example Output

```bash
$ ./bin/qm.exe 1
============================================================
OpenMP enabled

Group 0:
    Set 0:  000000000000|   0 |  0
    Set 1:  000000000001|   1 |  0
    Set 2:  000000000010|   2 |  0
    Set 3:  000000000101|   5 |  0
    Set 4:  000000000111|   7 |  0
    Set 5:  000000001000|   8 |  0
    Set 6:  000000001001|   9 |  0
    Set 7:  000000001010|  10 |  0
    Set 8:  000000001101|  13 |  0
    Set 9:  000000001111|  15 |  0

Group 1:
    Set 0:  00000000-000|   0   8 |  0
    Set 1:  00000000-001|   1   9 |  0
    Set 2:  00000000-010|   2  10 |  0
    Set 3:  00000000-101|   5  13 |  0
    Set 4:  00000000-111|   7  15 |  0
    Set 5:  000000000-01|   1   5 |  0
    Set 6:  0000000000-0|   0   2 |  0
    Set 7:  00000000000-|   0   1 |  0
    Set 8:  0000000001-1|   5   7 |  0
    Set 9:  000000001-01|   9  13 |  0
    Set10:  0000000010-0|   8  10 |  0
    Set11:  00000000100-|   8   9 |  0
    Set12:  0000000011-1|  13  15 |  0

Group 2:
    Set 0:  00000000--01|   1   9   5  13 |  1
    Set 1:  00000000-0-0|   0   8   2  10 |  1
    Set 2:  00000000-00-|   0   8   1   9 |  1
    Set 3:  00000000-1-1|   5  13   7  15 |  1

Table state 0:
              0    1    2    5    7    8    9    10   13   15
C'D            - |  X |  - |  X |  - |  - |  X |  - |  X |  - |
B'D'           X |  - |  X |  - |  - |  X |  - |  X |  - |  - |
B'C'           X |  X |  - |  - |  - |  X |  X |  - |  - |  - |
BD             - |  - |  - |  X |  X |  - |  - |  - |  X |  X |
prime implicant B'D' is essential
remove minterms 0 8 2 10 with column dominance

Table state 1:
              1    5    7    9    13   15
C'D            X |  X |  - |  X |  X |  - |
B'C'           X |  - |  - |  X |  - |  - |
BD             - |  X |  X |  - |  X |  X |

prime implicant BD is essential
remove minterms 5 13 7 15 with column dominance

Table state 2:
              1   9
C'D            X |  X |
B'C'           X |  X |

Column, Row dominance stuck removing first implicant C'D
remove minterms 1 9 5 13

F = B'D' + BD + C'D
============================================================

```