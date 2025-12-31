#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <sys/types.h>

/**
 * Operation mode enumeration - defines what operation the user wants to perform
 *
 * Represents the primary mode of operation selected by command-line arguments.
 * Only one mode can be active at a time (mutually exclusive).
 *
 * Modes:
 * - MODE_NONE: No mode selected (error state, requires validation)
 * - MODE_PORT: Inspect network connections on a specific port (--port)
 * - MODE_PID: Inspect a specific process by PID (--pid)
 * - MODE_ALL: List all running processes (--all)
 * - MODE_HELP: Display help/usage information (--help)
 * - MODE_VERSION: Display version information (--version)
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
 * Parsed command-line arguments structure
 *
 * Contains all parsed and validated command-line arguments. Populated by
 * parse_args() and validated by validate_args(). Used throughout the
 * application to determine behavior and output formatting.
 *
 * Fields:
 * - mode: Primary operation mode (port/pid/all/help/version)
 * - port: Target port number (valid when mode == MODE_PORT)
 * - pid: Target process ID (valid when mode == MODE_PID)
 * - short_output: Enable one-line output format
 * - show_tree: Display process ancestry tree
 * - json_output: Output in JSON format
 * - warnings_only: Show only security warnings (port mode only)
 * - no_color: Disable colored output
 * - show_env: Display environment variables (pid mode only)
 * - interactive: Enable interactive mode with kill prompt
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
 * Parse command-line arguments and populate the cli_args_t structure
 *
 * See src/args.c for detailed documentation.
 *
 * @param argc Argument count from main()
 * @param argv Argument vector from main()
 * @param args Pointer to cli_args_t structure to populate
 * @return 0 on success, -1 on error (invalid argument, missing value, or parse failure)
 */
int parse_args(int argc, char **argv, cli_args_t *args);

/**
 * Print usage/help message to standard output
 *
 * See src/args.c for detailed documentation.
 *
 * @param program_name Name of the program executable (typically argv[0])
 * @return void
 */
void print_usage(const char *program_name);

/**
 * Print version information to standard output
 *
 * See src/args.c for detailed documentation.
 *
 * @return void
 */
void print_version(void);

/**
 * Validate parsed arguments for logical consistency and compatibility
 *
 * See src/args.c for detailed documentation.
 *
 * @param args Pointer to cli_args_t structure containing parsed arguments
 * @return 0 if arguments are valid and consistent, -1 if validation fails
 */
int validate_args(const cli_args_t *args);

#endif /* ARGS_H */
