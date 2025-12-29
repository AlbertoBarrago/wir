#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <sys/types.h>

/**
 * Operation mode - what the user wants to do
 */
typedef enum {
    MODE_NONE,
    MODE_PORT,      /* Inspect a port */
    MODE_PID,       /* Inspect a PID */
    MODE_ALL,       /* List all processes */
    MODE_HELP,      /* Show help */
    MODE_VERSION    /* Show version */
} operation_mode_t;

/**
 * Parsed command-line arguments
 */
typedef struct {
    operation_mode_t mode;

    /* Target values */
    int port;           /* --port <n> */
    pid_t pid;          /* --pid <n> */

    /* Output flags */
    bool short_output;  /* --short */
    bool show_tree;     /* --tree */
    bool json_output;   /* --json */
    bool warnings_only; /* --warnings */
    bool no_color;      /* --no-color */
    bool show_env;      /* --env */
    bool interactive;   /* --interactive */
} cli_args_t;

/**
 * Parse command-line arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @param args Output structure for parsed arguments
 * @return 0 on success, -1 on error
 */
int parse_args(int argc, char **argv, cli_args_t *args);

/**
 * Print usage/help message
 * @param program_name Name of the program (argv[0])
 */
void print_usage(const char *program_name);

/**
 * Print version information
 */
void print_version(void);

/**
 * Validate parsed arguments for logical consistency
 * @param args Parsed arguments
 * @return 0 if valid, -1 if invalid
 */
int validate_args(const cli_args_t *args);

#endif /* ARGS_H */
