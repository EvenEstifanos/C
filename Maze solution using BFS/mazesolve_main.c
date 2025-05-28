#include "mazesolve.h"

// Function to load the maze and set log level if required
maze_t* load_maze(int argc, char *argv[]) {
    maze_t *maze = NULL;
    
    // If there are 4 arguments, the log level is set by the second argument
    if (argc == 4) {
        LOG_LEVEL = atoi(argv[2]); // Set log level from command line argument
        maze = maze_from_file(argv[3]); // Load the maze from the file
    } else if (argc == 2) {
        maze = maze_from_file(argv[1]); // Only load maze if 2 arguments
    }

    return maze;
}

int main(int argc, char *argv[]) {
    // Check if the number of arguments is correct
    if (argc != 2 && argc != 4) {
        printf("Usage: %s [-log N] <maze-file>\n", argv[0]);
        return 1; // Exit if incorrect number of arguments
    }

    // Load the maze based on the command-line arguments
    maze_t *maze = load_maze(argc, argv);
    if (maze == NULL) {
        printf("Error: Could not load maze file. Exiting with error code 1\n");
        return 1;
    }

    // Display initial maze info
    maze_print_tiles(maze);

    // Run BFS algorithm to solve the maze
    maze_bfs_iterate(maze);
    maze_set_solution(maze);

    // Check if a solution exists and print the result
    if (maze->tiles[maze->end_row][maze->end_col].path == NULL) {
        printf("No solution found.\n");
    } else {
        printf("SOLUTION:\n");
        maze_print_tiles(maze); // Print the maze with the solution path
        tile_print_path(&(maze->tiles[maze->end_row][maze->end_col]), PATH_FORMAT_VERBOSE); // Print the solution steps
    }

    maze_free(maze); // Free the maze resources
    return 0;
}