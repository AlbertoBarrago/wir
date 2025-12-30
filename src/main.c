#include <stdio.h>
#include "args.h"
#include "platform.h"
#include "output.h"
#include "utils.h"

/**
 * Handle --pid operation
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
        /* Show process ancestry tree */
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
 * Handle --port operation
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
    int result = output_port_info(args->port, connections, count, args);

    free(connections);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * Handle --all operation
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
 * Main entry point
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
