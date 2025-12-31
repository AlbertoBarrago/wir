#include <stdio.h>
#include "args.h"
#include "platform.h"
#include "output.h"
#include "utils.h"

/**
 * Handle --pid operation to display process information
 *
 * Retrieves and displays information about a specific process identified by PID.
 * Supports multiple output modes based on the provided arguments:
 * - Environment variables mode (--env): Shows all environment variables
 * - Process tree mode (--tree): Shows full process ancestry tree
 * - Default mode: Shows basic process information
 *
 * Error handling:
 * - Returns EXIT_FAILURE if a process doesn't exist or access is denied
 * - Cleans up allocated resources (env_vars, tree) before returning
 * - Provides helpful error messages to guide the user
 *
 * @param args Pointer to cli_args_t structure containing parsed arguments with PID and output mode flags
 * @return EXIT_SUCCESS (0) on successful display, EXIT_FAILURE (1) on error
 */
static int handle_pid_operation(const cli_args_t *args) {
    /* Get basic process information */
    process_info_t info;
    if (platform_get_process_info(args->pid, &info) < 0) {
        print_error("Failed to get information for PID %d", args->pid);
        print_error("Process may not exist or you don't have permission to access it");
        return EXIT_FAILURE;
    }

    /* Handle different output modes */
    if (args->show_env) {
        /* Show environment variables */
        char **env_vars = NULL;
        int count = 0;

        if (platform_get_process_env(args->pid, &env_vars, &count) < 0) {
            print_error("Failed to get environment variables for PID %d", args->pid);
            print_error("You may not have permission to access this process");
            return EXIT_FAILURE;
        }

        output_process_env(env_vars, count, args);
        platform_free_env_vars(env_vars, count);
    }
    else if (args->show_tree) {
        /* Show the process ancestry tree */
        process_tree_node_t *tree = NULL;

        if (platform_get_process_tree(args->pid, &tree) < 0) {
            print_error("Failed to build process tree for PID %d", args->pid);
            return EXIT_FAILURE;
        }

        output_process_tree(tree, args);
        platform_free_process_tree(tree);
    }
    else {
        /* Show basic process information */
        output_process_info(&info, args);
    }

    return EXIT_SUCCESS;
}

/**
 * Handle --port operation to display port usage information
 *
 * Queries the system for all network connections using a specific port number.
 * Retrieves connection information including process IDs, connection states,
 * and related details. The output can be filtered to show only warnings
 * using the --warnings flag.
 *
 * The function:
 * 1. Queries all connections on the specified port
 * 2. Formats and displays the connection information
 * 3. Properly frees allocated memory regardless of success or failure
 *
 * Error handling:
 * - Returns EXIT_FAILURE if a port query fails (may need elevated privileges)
 * - Ensures connections array is freed even on error
 * - Provides informative error messages about privilege requirements
 *
 * @param args Pointer to cli_args_t structure containing the port number and output flags
 * @return EXIT_SUCCESS (0) on successful display, EXIT_FAILURE (1) on error
 */
static int handle_port_operation(const cli_args_t *args) {
    connection_info_t *connections = NULL;
    int count = 0;

    /* Get all connections on the port */
    if (platform_get_port_connections(args->port, &connections, &count) < 0) {
        print_error("Failed to query port %d", args->port);
        print_error("You may need elevated privileges to inspect network connections");
        free(connections);
        return EXIT_FAILURE;
    }

    /* Output the results */
    const int result = output_port_info(args->port, connections, count, args);

    free(connections);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * Handle --all operation to display all running processes
 *
 * Retrieves a list of all currently running processes on the system and
 * displays them according to the specified output format (short, JSON, or
 * standard). This provides a system-wide view of process activity.
 *
 * The function:
 * 1. Fetches complete list of running processes from platform layer
 * 2. Passes the process list to output formatter
 * 3. Cleans up allocated memory before returning
 *
 * Error handling:
 * - Returns EXIT_FAILURE if unable to retrieve process list
 * - Ensures processes array is freed even on error
 * - Provides error message if retrieval fails
 *
 * @param args Pointer to cli_args_t structure containing output format flags (short_output, json_output)
 * @return EXIT_SUCCESS (0) on successful display, EXIT_FAILURE (1) on error
 */
static int handle_all_operation(const cli_args_t *args) {
    process_info_t *processes = NULL;
    int count = 0;

    /* Get all processes */
    if (platform_get_all_processes(&processes, &count) < 0) {
        print_error("Failed to get process list");
        free(processes);
        return EXIT_FAILURE;
    }

    /* Output the results */
    int result = output_process_list(processes, count, args);

    free(processes);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * Main entry point of the application
 *
 * Orchestrates the complete application flow from argument parsing to cleanup:
 * 1. Parses command-line arguments using parse_args()
 * 2. Handles special modes (help, version) and exits early if needed
 * 3. Validates argument consistency using validate_args()
 * 4. Applies global settings (color output)
 * 5. Initializes platform-specific subsystems
 * 6. Dispatches to appropriate handler based on operation mode (PID, PORT, ALL)
 * 7. Performs cleanup before exit
 *
 * Exit codes:
 * - EXIT_SUCCESS (0): Operation completed successfully
 * - EXIT_FAILURE (1): Error occurred (parse error, validation failure, or operation failure)
 *
 * Operation modes:
 * - MODE_HELP: Display usage information and exit
 * - MODE_VERSION: Display version information and exit
 * - MODE_PID: Analyze specific process by PID
 * - MODE_PORT: Analyze port usage
 * - MODE_ALL: List all running processes
 *
 * @param argc Argument count from shell
 * @param argv Argument vector from shell
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char **argv) {
    cli_args_t args;
    int exit_code = EXIT_SUCCESS;

    /* Parse command-line arguments */
    if (parse_args(argc, argv, &args) < 0) {
        fprintf(stderr, "\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Handle help mode */
    if (args.mode == MODE_HELP) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    /* Handle version mode */
    if (args.mode == MODE_VERSION) {
        print_version();
        return EXIT_SUCCESS;
    }

    /* Validate arguments */
    if (validate_args(&args) < 0) {
        fprintf(stderr, "\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Apply color settings */
    if (args.no_color) {
        use_colors = false;
    }

    /* Initialize platform layer */
    if (platform_init() < 0) {
        print_error("Failed to initialize platform layer");
        return EXIT_FAILURE;
    }

    /* Execute the requested operation */
    switch (args.mode) {
        case MODE_PID:
            exit_code = handle_pid_operation(&args);
            break;

        case MODE_PORT:
            exit_code = handle_port_operation(&args);
            break;

        case MODE_ALL:
            exit_code = handle_all_operation(&args);
            break;

        default:
            print_error("Invalid operation mode");
            exit_code = EXIT_FAILURE;
            break;
    }

    /* Cleanup */
    platform_cleanup();

    return exit_code;
}
