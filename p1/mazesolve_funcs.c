#include "mazesolve.h"

////////////////////////////////////////////////////////////////////////////////
// PROVIDED DATA
//
// The data below should not be altered as it should be used as-is in functions
///////////////////////////////////////////////////////////////////////////////

// Global variable controlling how much info should be printed; it is
// assigned values like LOG_ALL (defined in the header as 10) to
// trigger additional output to be printed during certain
// functions. This output is useful to monitor and audit how program
// execution proceeds.
int LOG_LEVEL = 0;

// Pre-specified order in which neighbor tiles should be checked for
// compatibility with tests.
direction_t dir_delta[5] = {NONE, NORTH, SOUTH, WEST, EAST};
int row_delta[5] = {+0, -1, +1, +0, +0};
int col_delta[5] = {+0, +0, +0, -1, +1};
#define DELTA_START 1
#define DELTA_COUNT 5

// strings to print for compact directions
char *direction_compact_strs[5] = {
    "?",  // NONE
    "N",  // NORTH
    "S",  // SOUTH
    "W",  // WEST
    "E",  // EAST
};

// strings to print for verbose directions
char *direction_verbose_strs[5] = {
    "NONE",   // NONE
    "NORTH",  // NORTH
    "SOUTH",  // SOUTH
    "WEST",   // WEST
    "EAST",   // EAST
};

#define TILETYPE_COUNT 6

// strings to print for each tile type
char tiletype_chars[TILETYPE_COUNT] = {
    '?',  // NOTSET = 0,
    '#',  // WALL,
    ' ',  // OPEN,
    '.',  // ONPATH,
    'S',  // START,
    'E',  // END,
};

// characters to print in the visual rendering of the BFS in the maze
// when the distance is evenly divisible by 10: a is 10, b is 20, c
// is 30, etc.
char digit10_chars[] = "0abcdefghijklmnopqrstuvwxyzABCDE%GHIJKLMNOPQR$TUVWXYZ";

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS FOR PROBLEM 1: Row/Col Queue
////////////////////////////////////////////////////////////////////////////////

// Allocates a new queue structure in heap memory and initializes it as empty.
rcqueue_t *rcqueue_allocate() {
  rcqueue_t *one = malloc(sizeof(rcqueue_t));  // Allocate memory for the queue

  one->front = NULL;  // Initialize front pointer to NULL (empty queue)
  one->rear = NULL;   // Initialize rear pointer to NULL (empty queue)
  one->count = 0;     // Initialize node count to 0

  return one;  // Return the allocated queue
}

// Adds a new node with given row and column values to the end of the queue.
void rcqueue_add_rear(rcqueue_t *queue, int row, int col) {
  rcnode_t *new = malloc(sizeof(rcnode_t));  // Allocate memory for the new node

  new->row = row;    // Assign row value
  new->col = col;    // Assign column value
  new->next = NULL;  // Set next pointer to NULL (new node is last in queue)

  if (queue->front == NULL) {  // If the queue is empty
    queue->front = new;        // Set both front and rear to the new node
    queue->rear = new;
  } else {
    queue->rear->next = new;  // Link the current rear node to the new node
    queue->rear = new;        // Update rear to the new node
  }
  queue->count++;  // Increase node count
}

// Frees all memory associated with the queue and its nodes.
void rcqueue_free(rcqueue_t *queue) {
  while (queue->count > 0) {            // Loop until the queue is empty
    rcnode_t *temp = queue->front;      // Store current front node
    queue->front = queue->front->next;  // Move front to next node
    free(temp);                         // Free the previous front node
    queue->count--;                     // Decrease node count
  }
  free(queue);  // Free the queue struct itself
}

// Retrieves the front element's row and column values without modifying the queue.
int rcqueue_get_front(rcqueue_t *queue, int *rowp, int *colp) {
  if (queue->count > 0) {       // Check if the queue is not empty
    *rowp = queue->front->row;  // Retrieve row value
    *colp = queue->front->col;  // Retrieve column value
    return 1;                   // Return success
  }
  return 0;  // Return failure if queue is empty
}

// Removes the front node of the queue and frees its memory.
int rcqueue_remove_front(rcqueue_t *queue) {
  if (queue->count > 0) {               // Check if the queue is not empty
    rcnode_t *temp = queue->front;      // Store the current front node
    queue->front = queue->front->next;  // Move front pointer to the next node
    free(temp);                         // Free the removed node
    queue->count--;                     // Decrease node count

    if (queue->front == NULL) {  // If queue becomes empty after removal
      queue->rear = NULL;        // Set rear pointer to NULL as well
    }
    return 1;  // Return success
  }
  return 0;  // Return failure if queue is empty
}

// Prints the contents of the queue in a formatted table.
void rcqueue_print(rcqueue_t *queue) {
  if (queue) {                                  // Ensure queue is not NULL
    printf("queue count: %d\n", queue->count);  // Print number of nodes
    printf("NN ROW COL\n");                     // Print header

    rcnode_t *temp = queue->front;                     // Start from the front of the queue
    for (int i = 0; i < queue->count; i++) {           // Iterate through the queue
      printf("%2d%3d%3d\n", i, temp->row, temp->col);  // Print node index, row, and col
      temp = temp->next;                               // Move to the next node
    }
  } else {
    printf("null queue\n");  // Print message if queue is NULL
  }
}

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS FOR PROBLEM 2: Tile and Maze Utility Functions
////////////////////////////////////////////////////////////////////////////////

// Prints the path for a given tile based on the selected format (compact or verbose)
void tile_print_path(tile_t *tile, int format) {
  // If the path is NULL, print that no path was found
  if (tile->path == NULL) {
    printf("No path found\n");
  } else {
    // If the format is compact, print the path in compact form without newlines
    if (format == 1) {
      for (int i = 0; i < tile->path_len; i++) {
        printf("%s", direction_compact_strs[tile->path[i]]);
      }
    }
    // If the format is verbose, print the length of the path and each step with its index
    else if (format == 2) {
      printf("path length: %d\n", tile->path_len);
      for (int i = 0; i < tile->path_len; i++) {
        printf(" %d: %s\n", i, direction_verbose_strs[tile->path[i]]);
      }
    }
    // If an unsupported format is given, print an error message
    else {
      printf("Not tested, error format\n");
    }
  }
}

// PROBLEM 2: Print the path field of `tile` in one of two formats. If
// the path field is NULL, print "No path found\n". Otherwise, if the
// `format` parameter is PATH_FORMAT_COMPACT, iterate over the path
// array and print each element as a string that comes from the global
// direction_compact_strs[] array without printing any newlines. If
// the `format` parameter is PATH_FORMAT_VERBOSE, print the length of
// the path on its own line then each element of the path with its
// index on its on line using strings from teh
// direction_verbose_strs[] global array. If the format is neither of
// COMPACT or VERBOSE, print an error message (not tested).
//
// CONSTRAINT: This function must use the correspondence of
// enumeration values like NORTH to integers to access elements from
// the global arrays direction_compact_strs[] and
// direction_verbose_strs[]. Code that utilizes long-winded
// conditional cases here will lose large quantities of style points.
//
// EXAMPLE:
// tile_t src = {.path_len = 4,  .path = {NORTH, EAST, EAST, SOUTH} };
// tile_print_path(&tile, PATH_FORMAT_COMPACT);
// printf("\n");
//   NEES
// tile_print_path(&tile, PATH_FORMAT_VERBOSE);
//   path length: 4
//     0: NORTH
//     1: EAST
//     2: EAST
//     3: SOUTH
//
// NOTES: This function will be short, less than 30 lines.  It should
// not have conditional structures that decide between NORTH, SOUTH,
// etc. Rather, use an access pattern like the following to "look up"
// the string that is needed in a given case.
//
//   direction_t d = path[i];
//   char *dirstr = direction_verbose_strs[d];
//   printf("%s\n",dirstr);
//
// Or more compactly
//
//   printf("%s\n", direction_verbose_strs[path[i]]);
//
// Handling a multipart conditional using an array of data like this
// is an elegant technique that proficient codes seek to use as often
// as possible.

// Extends the path of src tile by adding a direction to the dst tile
void tile_extend_path(tile_t *src, tile_t *dst, direction_t dir) {
  int dst_len = (src->path_len) + 1;                         // Calculate the new length for dst path
  direction_t *one = malloc(sizeof(direction_t) * dst_len);  // Allocate memory for the new path

  // Copy the path from src to dst (excluding the new direction)
  for (int i = 0; i < dst_len - 1; i++) {
    one[i] = src->path[i];
  }

  one[dst_len - 1] = dir;   // Add the new direction at the end of the path
  dst->path = one;          // Assign the new path to dst
  dst->path_len = dst_len;  // Update dst's path length
}
// PROBLEM 2: Fills in the path/path_len fields of `dst` based on
// arriving at it from the `src` tile via `dir`.  Heap-allocates a
// direction_t array for the path in dst that is one longer than the
// path in src. Copies all elements from the `src` path to `dst`. Adds
// `dir` as the final element of the path in `dst`. This function is
// used when a new tile is found during BFS to store the path used to
// locate it from the starting point.
//
// EXAMPLE:
// tile_t src = {.path_len = 4,  .path = {NORTH, EAST, EAST, SOUTH} };
// tile_t dst = {.path_len = -1, .path = NULL };
// tile_extend_path(&src, &dst, EAST);
// dst is now   {.path_len = 5,  .path = {NORTH, EAST, EAST, SOUTH, EAST} };
//
// NOTES: This function will need to access fields of the
// tiles. Review syntax to do so.

// Allocates a maze of given rows and columns and initializes the tiles
maze_t *maze_allocate(int rows, int cols) {
  maze_t *one = malloc(sizeof(maze_t));  // Allocate memory for the maze struct

  one->rows = rows;
  one->cols = cols;

  // Initialize start/end coordinates and queue as NULL
  one->start_row = -1;
  one->end_row = -1;
  one->start_col = -1;
  one->end_col = -1;
  one->queue = NULL;

  // Allocate memory for a 2D array of tiles
  one->tiles = malloc(sizeof(tile_t) * rows * cols);

  // Initialize each tile with default values (NOTSET for type, 1 for state, NULL for path, -1 for path_len)
  for (int i = 0; i < one->rows; i++) {
    one->tiles[i] = malloc(sizeof(tile_t) * cols);
    for (int j = 0; j < one->cols; j++) {
      one->tiles[i][j].type = NOTSET;
      one->tiles[i][j].state = 1;
      one->tiles[i][j].path = NULL;
      one->tiles[i][j].path_len = -1;
    }
  }

  return one;  // Return the allocated maze
}
// PROBLEM 2: Allocate on the heap a maze with the given rows/cols.
// Allocates space for the maze struct itself and an array of row
// pointers for its tiles. Then uses a nested set of loops to allocate
// each row of tiles for the maze and initialize the fields of each
// tile to be NOTSET, NOTFOUND, NULL, and -1 appropriately. Sets
// start/end row/col fields to be -1 and the queue to be NULL
// initially. Returns the resulting maze.
//
// CONSTRAINT: Assumes malloc() succeeds and does not include checks
// for its failure. Does not bother with checking rows/cols for
// inappropriate values such as 0 or negatives.
//
// NOTES: Consult recent lab work for examples of how to allocate a 2D
// array using repeated malloc() calls and adapt that approach
// hear. Ensure that the allocation is being done via rows of tile_t
// structs. Common errors are to neglect initializing all fields of
// the maze and all fields of every tile.  Valgrind errors that data
// is uninitialized are usually resolved by adding code to explicitly
// initialize everything.

// Frees the memory associated with the maze and its tiles
void maze_free(maze_t *maze) {
  // Free each tile's path if it exists and then free the row of tiles
  for (int i = 0; i < maze->rows; i++) {
    for (int j = 0; j < maze->cols; j++) {
      if (maze->tiles[i][j].path != NULL) {
        free(maze->tiles[i][j].path);  // Free the path if it's not NULL
        maze->tiles[i][j].path = NULL;
      }
    }
    free(maze->tiles[i]);  // Free each row of tiles
  }

  free(maze->tiles);  // Free the array of rows

  // If the maze has a queue, free it as well
  if (maze->queue != NULL) {
    rcqueue_free(maze->queue);  // Free the queue
  }

  free(maze);  // Finally, free the maze struct itself
}
// PROBLEM 2: De-allocates the memory associated with a maze and its
// tiles.  Uses a doubly nested loop to iterate over all tiles and
// de-allocate any non-NULL paths that are part of the tiles.
// Iterates to free each row of the tiles row and frees then frees the
// array of row tile row pointers. If the queue is non-null, frees it
// and finally frees the maze struct itself.

// Returns 1 if a tile at given coordinates is blocked (out of bounds or a wall), otherwise returns 0
int maze_tile_blocked(maze_t *maze, int row, int col) {
  // Check if the tile is out of bounds or if it's a wall
  if (maze->tiles == NULL || row < 0 || col < 0 || row >= maze->rows || col >= maze->cols ||
      maze->tiles[row][col].type == WALL) {
    return 1;  // The tile is blocked
  }
  return 0;  // The tile is not blocked
}

// PROBLEM 2: Return 1 if the indicated coordinate is blocked and
// could not be traversed as part of a path solving a maze. A
// coordinate is blocked if it is out of bounds for the maze (row/col
// below zero or above maze row/col boundaries) or the tile at that
// coordinate has type WALL. If the coordinate is not blocked, returns
// 0.  This function will be used later during the
// Breadth-First-Search to determine if a coordinate should be ignored
// due to being blocked.
//
// NOTES: This function will require accessing fields of the maze so
// review syntax on struct field acces and 2D indexing into the maze's
// tiles table. The tiletype_t enumeration in mazesolve.h establishes
// global symbols for tile types like OPEN, ONPATH, START, and so
// forth. Use one of these in this function.

// Prints the maze and its tiles, showing the start and end points
void maze_print_tiles(maze_t *maze) {
  // Print the maze size and start/end coordinates
  printf("maze: %d rows %d cols\n", maze->rows, maze->cols);
  printf("      (%d,%d) start\n", maze->start_row, maze->start_col);
  printf("      (%d,%d) end\n", maze->end_row, maze->end_col);

  printf("maze tiles:\n");

  // Print each tile character based on its type (from the tiletype_chars[] array)
  for (int i = 0; i < maze->rows; i++) {
    for (int j = 0; j < maze->cols; j++) {
      printf("%c", tiletype_chars[maze->tiles[i][j].type]);
    }
    printf("\n");
  }
}
// PROBLEM 2: Prints `maze` showing the solution path from Start to
// End tiles. First prints maze information including the size in rows
// and columns and the coordinates of Start/End tiles. The prints out
// the maze tiles with each tile printed based on its type. The character
// to print for each tile is based on its type and corresponds to
// elements in the global tiletyp_chars[] array: if the type is WALL,
// prints the character at tiletype_char[WALL]. This function is used
// to print a maze loaded from a file and print the solution path
// after doing a BFS.
//
// EXAMPLE 1: Output for maze which does not yet have a solution so no
// tiles have state ONPATH
//
// maze: 7 rows 16 cols
//       (1,1) start
//       (3,8) end
// maze tiles:
// ################
// #S             #
// # ### ###### # #
// # ### ##E  #   #
// # ### #### ##  #
// #              #
// ################
//
// EXAMPLE 2: The same maze above where some tiles have been marked
// with ONPATH as a solution has been calcualted via BFS. The ONPATH
// tiles are printed with . characters specified in the
// tiletype_chars[] global variable.
//
// maze: 7 rows 16 cols
//       (1,1) start
//       (3,8) end
// maze tiles:
// ################
// #S             #
// #.### ###### # #
// #.### ##E..#   #
// #.### ####.##  #
// #..........    #
// ################

// Prints the maze state, showing the progress of the BFS search and the tile states
void maze_print_state(maze_t *maze) {
  // Iterate through each tile to print its state or path length
  for (int i = 0; i < maze->rows; i++) {
    for (int j = 0; j < maze->cols; j++) {
      // If the tile has been found, print its path length as a character
      if (maze->tiles[i][j].state == FOUND) {
        if (maze->tiles[i][j].path_len % 10 == 0) {
          printf("%c", digit10_chars[maze->tiles[i][j].path_len / 10]);  // Print character for multiples of 10
        } else {
          printf("%d", maze->tiles[i][j].path_len % 10);  // Print last digit of path length
        }
      }
      // If the tile is not found, print its type (e.g., wall or open space)
      else {
        printf("%c", tiletype_chars[maze->tiles[i][j].type]);
      }
    }
    printf(": %d\n", i);  // Print the row number
  }

  // Print the column axis labels (0, 1, 2, ..., n)
  int loop = 0;
  int cols_num = maze->cols;
  for (int i = 0; i < cols_num; i++) {
    printf("%d", i);
    if (i == 9) {  // After 9, reset and print the tens
      i = -1;
      cols_num -= 10;
      loop++;
    }
  }
  printf("\n0");
  for (int i = 1; i < loop + 1; i++) {
    printf("%9d", i);  // Print the tens row
  }
  printf("\n");

  // Print the maze queue
  rcqueue_print(maze->queue);
}
// PROBLEM 2: Prints the `maze` in a format that shows its state and
// progress during the BFS search from Start to End positions.  The
// 2D grid of tiles is printed with tile with state FOUND printing a
// single character representaion of its distance from the Start tile
// and all other tiles with state NOTFOUND printing a single character
// that is based on their type (e.g. spaces for OPEN tiles, # for WALL
// tiles, etc). The `maze` queue is also printed using the previously
// written rcqueue_print() function for output.
//
// FOUND tiles will print their distance as a single character per one
// of two cases so that compact be reasonably complete information
// about its distance from the Start tile is shown.
//
// - If the tile's path_len field is NOT divsible by 10, then print
//   the last digit of the path_len (e.g. divide path_len by 10 and
//   take remainder
//
// - If the tiles' path_len IS divisible by 10, print the single
//   character at index (path_len)/10 in the global string
//   digit10_chars[]. These will show 'a' for 10, 'b' for 20, 'c' for
//   30, and so on.
//
// To the right of each maze row print a ": <row#>". Below the maze,
// print two lines: the first a sequence of "01234..890123..." up to
// the number of columns and below that a "tens" digits (0, 1, 2, etc
// printed every 10 characters). The combin combination of row and
// column axis label numbers makes it easier to find a tile at a
// specific coordinate.
//
// This function is used during BFS to show progress of the algorithm
// in a step-by-step manner when logging of BFS steps is enabled.
//
// CONSTRAINT 1: You must provide comments in this function that guide
// a reader on what different blocks and in some cases specific lines
// do. Comments like "print the bottom axis labels" or "tile not
// found, print its normal character" are informative. Comments like
// "print" or "assign tile" are not informative.
//
// CONSTRAINT 2: This function must avoid "many-case" conditionals
// such as if/else an switches based on the tile type or path_len. Use
// the provided global arrays digit10_chars[] and tiletype_chars[] to
// avoid such many-case conditionals.
//
// EXAMPLE: A 7x16 maze printed after a series of BFS steps.
//
// 0         1
// 0123456789012345
// ################: 0
// #0123456789a123#: 1
// #1###5######2#4#: 2
// #2###6##E  #34 #: 3
// #3###7####4##  #: 4
// #456789a1234   #: 5
// ################: 6
// queue count: 4
// NN ROW COL
//  0   4  10
//  1   5  11
//  2   3  13
//  3   2  14
//
// EXAMPLE NOTES: The End tile E at (3,8) has not been found yet so is
// drawn with an E while the Start tile at (1,1) is FOUND so is drawn
// with a 0 (path_len 0 from the Start). Some tiles like the WALL
// tiles marked with # are never FOUND as they are blocked and some
// OPEN tiles have not been FOUND yet so are draw with a space. Some
// FOUND tiles have 'a' in them indicating their path is 10 long from
// the Start tile. The 'a' is taken from the digits10_chars[] array.

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS FOR PROBLEM 3: Breadth First Search of the Maze
////////////////////////////////////////////////////////////////////////////////

void maze_bfs_init(maze_t *maze) {
  // Initialize the start tile for BFS.
  maze->tiles[maze->start_row][maze->start_col].path = malloc(0);  // Empty path initially.
  maze->tiles[maze->start_row][maze->start_col].path_len = 0;      // Start tile has no path length yet.
  maze->tiles[maze->start_row][maze->start_col].state = FOUND;     // Mark the start tile as FOUND.

  // Initialize the queue for BFS.
  maze->queue = rcqueue_allocate();                                 // Allocate the queue.
  rcqueue_add_rear(maze->queue, maze->start_row, maze->start_col);  // Add the start tile to the queue.

  // Log the initialization process if appropriate.
  if (LOG_LEVEL >= LOG_BFS_STATES) {
    printf("LOG: BFS initialization complete\n");
    maze_print_state(maze);  // Print the maze state after initialization.
  }
}
// PROBLEM 3: Initializes the maze for a BFS search. Adjusts the start
// tile: allocates it a length 0 path, sets its path_len to 0, and
// sets its state to FOUND. Allocates an empty rcqueue for the queue
// in the maze using an appropriate function and then adds the Start
// tile to it.
//
// LOGGING: If LOG_LEVEL >= LOG_BFS_STATES, after initialization is
// complete. prints "BFS initialization compelte" and calls
// maze_print_state().
//
// NOTES: This function is called within maze_bfs_iterate() to set up
// the queue and Start tile before the search proceeds. It should not
// be called outside of that context except during some testing of its
// functionality and that of the BFS step function.
//
// EXAMPLE: The queue is initially empty (prints as null). After
//          calling bfs_init(), the Start tile is set to FOUND so
//          prints as its path_len of 0 and appears in the now
//          allocated queue.
//
// print_maze_state(maze);
//   ################:  0
//   #S             #:  1
//   # ### ###### # #:  2
//   # ### ##E  #   #:  3
//   # ### #### ##  #:  4
//   #              #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   null queue
// LOG_LEVEL = LOG_ALL;
// maze_bfs_init(maze);
// print_maze_state(maze);
//   LOG: BFS initialization complete
//   ################:  0
//   #0             #:  1
//   # ### ###### # #:  2
//   # ### ##E  #   #:  3
//   # ### #### ##  #:  4
//   #              #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   queue count: 1
//   NN ROW COL
//    0   1   1

int maze_bfs_process_neighbor(maze_t *maze, int cur_row, int cur_col, direction_t dir) {
  tile_t *cur = &maze->tiles[cur_row][cur_col];  // Current tile in the BFS.
  int new_row, new_col;

  // Compute the coordinates of the neighboring tile based on the direction.
  new_row = cur_row + row_delta[dir];
  new_col = cur_col + col_delta[dir];

  // If the neighboring tile is blocked, skip it.
  if (maze_tile_blocked(maze, new_row, new_col) == 1 && LOG_LEVEL >= LOG_SKIPPED_TILES) {
    printf("LOG: Skipping BLOCKED tile at (%d,%d) \n", new_row, new_col);
    return 0;  // Return 0 as the tile is blocked and cannot be processed.
  }

  tile_t *new_one = &maze->tiles[new_row][new_col];  // Neighbor tile.

  // If the neighboring tile has already been found, skip it.
  if (new_one->state == FOUND && LOG_LEVEL >= LOG_SKIPPED_TILES) {
    printf("LOG: Skipping FOUND tile at (%d,%d)\n", new_row, new_col);
    return 0;  // Return 0 as the tile has already been processed.
  }

  // If the neighboring tile is not found, process it.
  if (new_one->state != FOUND && LOG_LEVEL >= LOG_BFS_PATHS) {
    new_one->state = FOUND;                           // Mark the neighbor tile as FOUND.
    tile_extend_path(cur, new_one, dir);              // Extend the path from the current tile to the neighbor.
    rcqueue_add_rear(maze->queue, new_row, new_col);  // Add the neighbor to the queue.

    // Log the newly found tile and its path if appropriate.
    printf("LOG: Found tile at (%d,%d) with len %d path: ", new_row, new_col, new_one->path_len);
    tile_print_path(new_one, PATH_FORMAT_COMPACT);  // Print the compact path of the tile.
    printf("\n");

    return 1;  // Return 1 as the neighbor was successfully processed.
  }

  return 0;  // Return 0 as no changes were made.
}
// PROBLEM 3: Process the neighbor in direction `dir` from coordinates
// `cur_row/cur_col`. Calculates the adjacent tiles row/col
// coordinates using the row_delta[]/col_delta[] global array and
// `dir`.  If the neighbor tile is blocked according to the
// maze_tile_blocked() function, makes no changes and returns 0 as the
// position cannot be reached. If the neighbor tile has state FOUND,
// makes no changes and returns 0 as the tile has already been
// processed in the BFS. Otherwise changes the neighbor tile to be a
// Found tile: extends the current tile's path to the neighbor tile in
// the given direction, changes the neighbor tile's state to FOUND,
// and adds the neighbor tile to the maze search queue. This function
// is used in BFS to propogate paths to all non-blocked neighbor
// tiles and extend the search forntier.
//
// LOGGING:
// 1. If LOG_LEVEL >= LOG_BFS_PATHS and the neighor tile's state
//    changes from NOTFOUND to FOUND, print a message like:
//      LOG: Found tile at (4,10) with len 14 path: SSSSEEEEEEEEEN
//    with the coordinates, path_len, and COMPACT path for the newly
//    found tile.
// 2. If LOG_LEVEL >= LOG_SKIPPED_TILES and the neighbor tile is
//    skipped as it is blocked or already found, print a message like
//    one of
//      LOG: Skipping BLOCKED tile at (6,13)
//      LOG: Skipping FOUND tile at (5,12)
//
// EXAMPLE:
// maze_print_state(maze);
//   ################:  0
//   #0123          #:  1
//   #1### ###### # #:  2
//   #2### ##E  #   #:  3
//   #3### #### ##  #:  4
//   #4             #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   queue count: 2
//   NN ROW COL
//    0   1   4
//    1   5   1
// LOG_LEVEL = LOG_ALL; // above both LOG_BFS_PATHS and LOG_SKIPPED_TILES
// maze_bfs_process_neighbor(maze, 1, 4, NORTH);
//   LOG: Skipping BLOCKED tile at (0,4)
// maze_bfs_process_neighbor(maze, 1, 4, SOUTH);
//   LOG: Skipping BLOCKED tile at (2,4)
// maze_bfs_process_neighbor(maze, 1, 4, WEST);
//   LOG: Skipping FOUND tile at (1,3)
// maze_bfs_process_neighbor(maze, 1, 4, EAST);
//   LOG: Found tile at (1,5) with len 4 path: EEEE
// maze_print_state(maze);
//   ################:  0
//   #01234         #:  1
//   #1### ###### # #:  2
//   #2### ##E  #   #:  3
//   #3### #### ##  #:  4
//   #4             #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   queue count: 2
//   NN ROW COL
//    0   1   4
//    1   5   1
//    2   1   5

int maze_bfs_step(maze_t *maze) {
  if (maze == NULL) {
    printf("maze is NULL");
    return 0;  // Return 0 if the maze pointer is NULL.
  }

  int row, col;
  rcqueue_get_front(maze->queue, &row, &col);  // Get the coordinates of the front tile in the queue.

  // Log the processing of neighbors for the front tile if appropriate.
  if (LOG_LEVEL >= LOG_BFS_STEPS) {
    printf("LOG: processing neighbors of (%d,%d)\n", row, col);

    // Process all neighbors in the four directions (North, South, West, East).
    for (int i = 1; i < 5; i++) {
      maze_bfs_process_neighbor(maze, row, col, dir_delta[i]);  // Process the neighbor in direction `i`.
    }
    rcqueue_remove_front(maze->queue);  // Remove the front tile from the queue.

    // Log the maze state after processing the step if appropriate.
    if (LOG_LEVEL >= LOG_BFS_STATES) {
      printf("LOG: maze state after BFS step\n");
      maze_print_state(maze);  // Print the maze state.
    }
    return 1;  // Return 1 as the step was successfully processed.
  }

  return 0;  // Return 0 if the logging level does not require detailed steps.
}
// PROBLEM 3: Processes the tile in BFS which is at the front of the
// maze search queue. For the front tile, iterates over the directions
// in the global dir_delta[] array from index DELTA_START to less than
// DELTA_COUNT which will be NORTH, SOUTH, WEST, EAST. Processes the
// neighbors in each of these directions with an appropriate
// function. Removes the front element of the search queue and
// returns 1. Note: if this function is called when the maze queue is
// empty, return 0 and print an error message though this case will
// not be tested and should not arise if other parts of the program
// are correct.
//
// LOGGING:
// If LOG_LEVEL >= LOG_BFS_STEPS, print a message like
//   LOG: processing neighbors of (5,1)
// at the start of the function.
//
// If LOG_LEVEL >= LOG_BFS_STATES, prints a message and uses
// maze_print_state() at the end of the function to show the maze
// after processing finishes as in:
//   LOG: maze state after BFS step
//   ################:  0
//   #01234         #:  1
//   #1### ###### # #:  2
//   #2### ##E  #   #:  3
//   #3### #### ##  #:  4
//   #45            #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   queue count: 2
//   NN ROW COL
//    0   1   5
//    1   5   2
//
// EXAMPLE:
// maze_print_state(maze);
//   ################:  0
//   #0123456789a1  #:  1
//   #1###5###### # #:  2
//   #2###6##E  #   #:  3
//   #3###7#### ##  #:  4
//   #456789a12     #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   queue count: 2
//   NN ROW COL
//    0   1  12
//    1   5   9
// LOG_LEVEL = LOG_ALL;
// maze_bfs_step(maze);
//   LOG: processing neighbors of (1,12)
//   LOG: maze state after BFS step
//   ################:  0
//   #0123456789a12 #:  1
//   #1###5######2# #:  2
//   #2###6##E  #   #:  3
//   #3###7#### ##  #:  4
//   #456789a12     #:  5
//   ################:  6
//   0123456789012345
//   0         1
//   queue count: 3
//   NN ROW COL
//    0   5   9
//    1   2  12
//    2   1  13

void maze_bfs_iterate(maze_t *maze) {
  maze_bfs_init(maze);  // Initialize the BFS search on the maze.
  int step = 1;         // Initialize the step counter.

  // Continue iterating until the queue is empty.
  while (maze->queue->count != 0) {
    if (LOG_LEVEL >= LOG_BFS_STEPS) {
      printf("LOG: BFS STEP %d\n", step);  // Log the current step.
    }
    maze_bfs_step(maze);  // Process one step of the BFS.
    step++;               // Increment the step counter.
  }
}
// PROBLEM 3: Initializes a BFS on the maze and iterates BFS steps
// until the queue for the maze is empty and the BFS is complete. Each
// iteration of the algorithm loop will process all neighbors of a
// FOUND tile.  This will calculate paths from the Start tile to all
// other tiles including the End tile.
//
// LOGGING: If LOG_LEVEL >= LOG_BFS_STEPS prints the step number for
// each iteration of the loop as
//   LOG: BFS STEP 45
// where the number starts at 1 and increases each loop iteration.
//
// See EXAMPLES for main() to get an idea of output for iteration.
//
// NOTES: This function will call several of the preceding functions
// to initialize and proceed with the BFS.

int maze_set_solution(maze_t *maze) {
  tile_t *end = &maze->tiles[maze->end_row][maze->end_col];  // End tile.

  if (end->path == NULL) {
    return 0;  // Return 0 if the end tile has no path (no solution found).
  }

  // Log the solution path if appropriate.
  if (LOG_LEVEL >= LOG_SET_SOLUTION) {
    printf("LOG: solution START at (%d,%d)\n", maze->start_row, maze->start_col);

    // Set the path starting from the start tile and moving towards the end tile.
    int new_row = maze->start_row + row_delta[end->path[0]];
    int new_col = maze->start_col + col_delta[end->path[0]];
    maze->tiles[new_row][new_col].type = ONPATH;  // Mark the first tile in the path.
    printf("LOG: solution path[%d] is %s, set (%d,%d) to ONPATH\n", 0, direction_verbose_strs[end->path[0]], new_row,
           new_col);

    // Continue setting the path for the remaining tiles.
    for (int i = 1; i < end->path_len; i++) {
      new_row += row_delta[end->path[i]];
      new_col += col_delta[end->path[i]];

      maze->tiles[new_row][new_col].type = ONPATH;  // Mark each tile in the path.

      printf("LOG: solution path[%d] is %s, set (%d,%d) to ONPATH\n", i, direction_verbose_strs[end->path[i]], new_row,
             new_col);
    }

    // Log the end of the solution path.
    printf("LOG: solution END at (%d,%d)\n", maze->end_row, maze->end_col);
    end->type = END;  // Mark the end tile as END.

    return 1;  // Return 1 to indicate the solution was set successfully.
  }

  return 0;  // Return 0 if the solution was not set.
}
// PROBLEM 3: Uses the path stored in the End tile to visit each tile
// on the solution path from Star to End and make them as ONPATH to
// show the solution path.  If the End tile has a NULL path, returns 0
// and makes no changes to the maze. Otherwise, visits each direction
// in the End tile's path and, beginning at the Start tile, "moves" in
// the direction indicated and chcnges the nextf tile's state to
// ONPATH. Returns 1 on changing the state of tiles to show the
// solution path.  This function is used to set up printing the
// solution path later.
//
// CONSTRAINT: Makes use of the row_delta[] / col_delta[] global
// arrays when "moving" rather than using multi-way conditional. For
// example, if the current coordinates are (3,5) and the path[i]
// element is WEST, uses row_delta[WEST] which is -1 and
// col_delta[WEST] which is 0 to update the coordinates to (3,4) which
// is one tile to the WEST. Solutions that use a multi-way conditional
// with cases for NORTH, SOUTH, etc. will be penalized.
//
// NOTE: This function may temporarily change the state of the END
// tile to ONPATH as indicated in the log messages below. Before
// finishing, the state of the END tile is changed from ONPATH to END.
//
// LOGGING: If the LOG_LEVEL >= LOG_SET_SOLUTION, prints out
//
// 1. Once at the beginning of the solution construction
//   LOG: solution START at (1,1)
// with the coordinates substituted for the actual START coordinates.
//
// 2. For every element of the End tile path[] array
//   LOG: solution path[5] is EAST, set (5,3) to ONPATH
// with items like the index, direction, and coordinates replaced with
// appropriate data via printf().  Use the direction_verbose_strs[]
// to print strings of the directions.
//
// 3. Once at the end of solution construction:
//   LOG: solution END at (3,8)
// with the coordinates substituted for the actual END coordinates.
//
// EXAMPLE:
// maze_print_tiles(maze);
//   maze: 7 rows 16 cols
//         (1,1) start
//         (3,8) end
//   maze tiles:
//   ################
//   #S             #
//   # ### ###### # #
//   # ### ##E  #   #
//   # ### #### ##  #
//   #              #
//   ################
//
// LOG_LEVEL = LOG_SET_SOLUTION;
// maze_set_solution(maze);
//   LOG: solution START at (1,1)
//   LOG: solution path[0] is SOUTH, set (2,1) to ONPATH
//   LOG: solution path[1] is SOUTH, set (3,1) to ONPATH
//   LOG: solution path[2] is SOUTH, set (4,1) to ONPATH
//   LOG: solution path[3] is SOUTH, set (5,1) to ONPATH
//   LOG: solution path[4] is EAST, set (5,2) to ONPATH
//   LOG: solution path[5] is EAST, set (5,3) to ONPATH
//   LOG: solution path[6] is EAST, set (5,4) to ONPATH
//   LOG: solution path[7] is EAST, set (5,5) to ONPATH
//   LOG: solution path[8] is EAST, set (5,6) to ONPATH
//   LOG: solution path[9] is EAST, set (5,7) to ONPATH
//   LOG: solution path[10] is EAST, set (5,8) to ONPATH
//   LOG: solution path[11] is EAST, set (5,9) to ONPATH
//   LOG: solution path[12] is EAST, set (5,10) to ONPATH
//   LOG: solution path[13] is NORTH, set (4,10) to ONPATH
//   LOG: solution path[14] is NORTH, set (3,10) to ONPATH
//   LOG: solution path[15] is WEST, set (3,9) to ONPATH
//   LOG: solution path[16] is WEST, set (3,8) to ONPATH
//   LOG: solution END at (3,8)
//
// maze_print_tiles(maze);
//   maze: 7 rows 16 cols
//         (1,1) start
//         (3,8) end
//   maze tiles:
//   ################
//   #S             #
//   #.### ###### # #
//   #.### ##E..#   #
//   #.### ####.##  #
//   #..........    #
//   ################

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS FOR PROBLEM 4: Maze Memory Allocation and File Input
////////////////////////////////////////////////////////////////////////////////

maze_t *maze_from_file(char *fname) {
  FILE *handle = fopen(fname, "r");  // open the file

  // if the file can't be openeed, print error message and return NULL
  if (handle == NULL) {
    printf("ERROR: could not open file %s\n", fname);
    return NULL;
  }
  int rows, cols;  // to hold the number of rows and cols of maze read from file

  fscanf(handle, "rows: %d cols: %d\n", &rows, &cols);  // read in the rows and cols #s from file
  fscanf(handle, "tiles:\n");                           // read in the "tiles" lines
  maze_t *one = maze_allocate(rows, cols);              // using the read rows and cols, create new maze

  if (LOG_LEVEL >= LOG_FILE_LOAD) {
    printf("LOG: expecting %d rows and %d columns\n", rows, cols);  // print the number of rows and cols
    printf("LOG: beginning to read tiles\n");                       // print the message to read the tile
  }

  char c = ' ';  // to hold the value for the tiles

  // I can't get to the finished reading row to print correct row, so I am using this
  int i = 0;               // set i to zero
  int printed_rows = i--;  // set the correct reading row to print

  // Read the maze tiles from the file row by row
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      // Read a character for the tile (it could be a space, wall, or a special character)
      fscanf(handle, "%c", &c);
      // Skip the newline character, but don't increment printed_rows here)
      if (c == '\n' && LOG_LEVEL >= LOG_FILE_LOAD) {
        printf("LOG: finished reading row %d of tiles\n", printed_rows++);
        j--;       // return col to zero
        continue;  // proceed to the next row
      }

      // Loop over all possible tile types (assuming there are 6 types)
      for (int z = 0; z < 6; z++) {    // six types, traverse through all
        if (c == tiletype_chars[z]) {  // Check if the character matches one of the tile types

          one->tiles[i][j].type = z;  // Set the tile type in the maze
          // If logging level allows, log details about this tile
          if (LOG_LEVEL >= LOG_FILE_LOAD) {
            printf("LOG: (%d,%d) has character '%c' type %d\n", i, j, c, z);
            // Special case: if the tile is the start point (type 4)
            if (z == 4) {
              printf("LOG: setting START at (%d,%d)\n", i, j);
              one->start_row = i;
              one->start_col = j;
            }
            // Special case: if the tile is the end point (type 5)
            if (z == 5) {
              printf("LOG: setting END at (%d,%d)\n", i, j);
              one->end_row = i;
              one->end_col = j;
            }
          }
        }
      }
    }
  }
  // After finishing each row, log the progress if the logging level is sufficient
  printf("LOG: finished reading row %d of tiles\n", rows - 1);

  // Close the file and return the populated maze structure
  fclose(handle);
  return one;
}

// PROBLEM 4: Read a maze from a text file. The text file format
// starts with the size of the maze and then contains its tiles. An
// example is:
//
// rows: 7 cols: 19
// tiles:
// ###################
// #          #    # #
// # ###  ##    ## # #
// #  ##  # S #  # # #
// ##  #  #####  # # #
// #E  #         #   #
// ###################
//
// If `fname` cannot be opened as a file, prints the error message
//   ERROR: could not open file <FILENAME>
// and returns NULL. Otherwise proceeds to read row/col numbers,
// allocate a maze of appropriate size, and read characters into
// it. Each tile read has its state changed per the character it is
// shown as. The global array tiletype_chars[] should be searched to
// find the character read for a tile and the index at which it is
// found set to be the tile state; e.g. the character 'S' was read
// which appears at index 4 of tiletype_chars[] so the tile.type = 4
// which is START in the tiletype enumeration.
//
// CONSTRAINT: You must use fscanf() for this function.
//
// CONSTRAINT: You MUST comment your code to describe the intent of
// blocks of code. Full credit comments will balance some details with
// broad descriptions of blocks of code.  Missing comments or pedantic
// comments which describe every code line with unneeded detail will
// not receive full credit. Aim to make it easy for a human reader to
// scan your function to find a specific point of interest such as
// where a tile type is determined or when a row is finished
// processing. Further guidance on good/bad comments is in the C
// Coding Style Guide.
//
// LOGGING: If LOG_LEVEL >= LOG_FILE_LOAD prints messages to help
// track parsing progress. There are many log messages required as
// they will be useful for debugging parsing problems that always
// arise when reading such data.
//
// LOG: expecting 6 rows and 9 columns
//   Printed after reading the first line indicating rows/cols in the
//   maze.
//
// LOG: beginning to read tiles
//   Printed after reading the "tiles\n" line when the main loop is
//   about to start reading character/tiles.
//
// LOG: (2,1) has character ' ' type 2
//   Printed with the coordinates of each character/tile that is
//   read. The coordinates, character read, and an integer indicating
//   the type decided upon for the tile is shown.
//
// LOG: (2,4) has character 'S' type 4
// LOG: setting START at (2,4)
//   Printed when the Start tile, marked with an S, is found
//
// LOG: (3,7) has character 'E' type 5
// LOG: setting END at (3,7)
//   Printed when the End tile, marked with an E, is found
//
// LOG: finished reading row 4 of tiles
//   Printed after reading each row of the maze file to help track
//   progress.
//
// NOTES: The fscanf() function is essential for this function. A call
// like fscanf(fin,"age: %d name: %s\n",&num, str) will read the literal
// word "age:" then a number, then the literal word "name:" and a
// string.  Use this facility to parse the simple format. It is fine
// to read a literal string only as in fscanf(fin,"skip this stuff\n")
// which will bypass that exact line in a file.  Use the format
// specifier %c to read single characters as you process the tiles as
// this is the easiest mechanism to work on the character parsing of
// the maze tiles.  Keep in mind that all lines of the maze will end
// with the character '\n' which must be read past in order to get to
// the next row.

// int main(int argc, char *argv[]);
//  PROBLEM 4: main() in mazesolve_main.c
//
//  The program must support two command line forms.
//  1. ./mazesolve_main <mazefile>
//  2. ./mazesolve_main -log <N> <mazefile>
//  Both forms have a maze data file as the last command line
//  argument. The 2nd form sets the global variable LOG_LEVEL to the
//  value N which enables additional output.
//
//  NOTES
//  - It is a good idea to check that the number of command line arguments
//    is either 2 (Form 1) and 4 (Form 2) and if not, bail out from the
//    program. This won't be tested but it simplifies the rest of the
//    program.
//  - All command line arguments come into C programs as strings
//    (char*).  That means the number after the -log option will also
//    be a string of characters and needs to be converted to an int to
//    be used in the program.  The atoi() function is useful for this:
//    search for documentation on it and use it for the conversion.
//  - Make sure to check that loading a maze from a file succeeds. If
//    not, print the following error message:
//      Could not load maze file. Exiting with error code 1
//    This message is in addition to the error message that is printed
//    byt he maze_from_file() function. Return 1 from main() to indicate
//    that the program was not successful.
//  - If a maze is successfully loaded, perform a BFS on it using
//    appropriate functions. Don't forget to free its memory before
//    ending the program.