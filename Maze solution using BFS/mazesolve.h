#pragma once

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>             // for variadic functions in testing

////////////////////////////////////////////////////////////////////////////////
// rcqueue_t data
////////////////////////////////////////////////////////////////////////////////
typedef struct rcnode {         // node type for row/col queues
  int row, col;                 // row/col coordinates for the node
  struct rcnode *next;          // pointer to next node
} rcnode_t;

typedef struct {                // queue type for row/col coordinates
  rcnode_t *front, *rear;       // pointers to ends of queue
  int count;                    // number of nodes in queue
} rcqueue_t;

////////////////////////////////////////////////////////////////////////////////
// tile enumerations
////////////////////////////////////////////////////////////////////////////////
typedef enum {                  // allowable directions for neighbors
  NONE = 0,                     // NONE should not be used
  NORTH,                        // Same as integer 1
  SOUTH,                        // Same as integer 2
  WEST,                         // Same as integer 3
  EAST,                         // Same as integer 4
} direction_t;
// EXAMPLE USE:
// direction_t dirs[5] = {NORTH, WEST, WEST, SOUTH, SOUTH};
// char *str = direction_verbose_strs[WEST];

typedef enum {                  // types of supported tiles
  NOTSET = 0,                   // NOTSET should not be used
  WALL,                         // cannot be traversed, display as '#'
  OPEN,                         // can be traversed, display as a space ' '
  ONPATH,                       // part of shortest path from Start to End, '.'
  START,                        // starting tile, display as 'S'
  END,                          // ending tile, display as 'E'
} tiletype_t;
// EXAMPLE USE:
// tile_t tile;
// tile.type = OPEN;
// char display = tiletype_chars[WALL];

typedef enum {                  // type used during BFS to track found iles
  UNKNOWN   = 0,                // UNKNOWN should not be used
  NOTFOUND,                     // tile has not yet be found during BFS
  FOUND                         // tile has been found during BFS and has its path set
} searchstate_t;
// EXAMPLE USE:
// tile_t tile;
// tile.state = NOTFOUND;

////////////////////////////////////////////////////////////////////////////////
// tile and maze data
////////////////////////////////////////////////////////////////////////////////
typedef struct {                // tile data for positions in a 2D maze
  tiletype_t type;              // One of NOTSET, OPEN, WALL, ONPATH, START, END
  searchstate_t state;          // One of NOT_FOUND, QUEUED, DONE
  direction_t *path;            // array of directions from start to this position
  int path_len;                 // length of path array
} tile_t;

typedef struct {                // maze data tracking shape of maze and state of BFS search
  tile_t **tiles;               // 2D array of tiles
  int rows, cols;               // number of rows/cols in the 2D tile array
  int start_row, start_col;     // starting position in the maze
  int end_row, end_col;         // ending position in the maze
  rcqueue_t *queue;             // queue of coordinates to search
} maze_t;

////////////////////////////////////////////////////////////////////////////////
// other defined symbols 
////////////////////////////////////////////////////////////////////////////////

// symbols defining the format for paths
#define PATH_FORMAT_COMPACT 1  
#define PATH_FORMAT_VERBOSE 2

// symbols associated with log levels
#define LOG_BFS_STEPS      1
#define LOG_BFS_STATES     2
#define LOG_SET_SOLUTION   3
#define LOG_BFS_PATHS      4
#define LOG_SKIPPED_TILES  5
#define LOG_FILE_LOAD      6
#define LOG_ALL           10


////////////////////////////////////////////////////////////////////////////////
// functions and data in mazesolve_funcs.c
////////////////////////////////////////////////////////////////////////////////

extern int LOG_LEVEL;
rcqueue_t *rcqueue_allocate();
void rcqueue_add_rear(rcqueue_t *queue, int row, int col);
void rcqueue_free(rcqueue_t *queue);
int rcqueue_get_front(rcqueue_t *queue, int *rowp, int *colp);
int rcqueue_remove_front(rcqueue_t *queue);
void rcqueue_print(rcqueue_t *queue);
void tile_print_path(tile_t *tile, int format);
void tile_extend_path(tile_t *src, tile_t *dst, direction_t dir);
maze_t *maze_allocate(int rows, int cols);
void maze_free(maze_t *maze);
int maze_tile_blocked(maze_t *maze, int row, int col);
void maze_print_tiles(maze_t *maze);
void maze_print_state(maze_t *maze);
void maze_bfs_init(maze_t *maze);
int maze_bfs_process_neighbor(maze_t *maze, int cur_row, int cur_col, direction_t dir);
int maze_bfs_step(maze_t *maze);
void maze_bfs_iterate(maze_t *maze);
int maze_set_solution(maze_t *maze);
maze_t *maze_from_file(char *fname);
