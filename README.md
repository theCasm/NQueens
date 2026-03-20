# N Queens w/ arc-consistency in C

This is a very simple constraint search approach in C to finding a solution to the N queens problem. It could easily be modified to search for multiple solutions, but that is beyond the scope of this project.

## Compiling
Compile with `make` (for the optimized version) or `make dev` for unoptimized version with debugging symbols.

## Usage
`./bin/constraint <N>` - find a solution to the N queens problem on an NxN board.  
`./bin/constraint` - keep finding solutions to the N queens problem, starting with N=1 and increasing from there.

In both cases, the output will look like a list of numbers. The nth number is the vertical position of the queen in the nth column (starting from 0). For example, `1, 3, 0, 2` represents the board:
```
  -----------------
0 |   |   | x |   |
  -----------------
1 | x |   |   |   |
  -----------------
2 |   |   |   | x |
  -----------------
3 |   | x |   |   |
  -----------------
```