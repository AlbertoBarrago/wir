#include "args.h"
#include "utils.h"
#include "version.h"
#include <errno.h>
#include <limits.h>
#include <string.h>

/**
 * Print version information to standard output
 *
 * Displays the application name, version number, description, and author
 * information. This function is called when the user runs the program with
 * the --version or -v flag.
 *
 * Output format:
 * - Line 1: Application name and version
 * - Line 2: Description
 * - Line 3: Author information
 *
 * @return void
 */
void print_version(void) {
  printf("%s version %s\n", WIR_NAME, WIR_VERSION);
  printf("%s\n", WIR_DESCRIPTION);
  printf("\nCrafted with ♥️ by %s\n", WIR_AUTHOR);
}

/**
 * Print usage/help message to standard output
 *
 * Displays comprehensive help information including program description,
 * usage syntax, all available command-line options with descriptions,
 * and practical usage examples. This function is called when:
 * - The user runs the program with --help or -h flag
 * - The user runs the program without any arguments
 * - The user provides invalid arguments
 *
 * The help output includes:
 * - Program name, version, and description header
 * - Usage syntax showing how to invoke the program
 * - Detailed list of all available options with explanations
 * - Multiple practical examples demonstrating common use cases
 *
 * @param program_name Name of the program executable (typically argv[0])
 * @return void
 */
void print_usage(const char *program_name) {
  printf("%s v%s - %s\n", WIR_NAME, WIR_VERSION, WIR_DESCRIPTION);
  printf("\n");
  printf("Usage: %s [OPTIONS]\n", program_name);
  printf("\n");
  printf("Options:\n");
  printf("  --pid <n>         Explain a specific PID\n");
  printf("  --port <n>        Explain port usage\n");
  printf("  --all             List all running processes\n");
  printf("  --short           One-line summary\n");
  printf("  --tree            Show full process ancestry tree\n");
  printf("  --json            Output result as JSON\n");
  printf("  --warnings        Show only warnings\n");
  printf("  --no-color        Disable colorized output\n");
  printf(
      "  --env             Show only environment variables for the process\n");
  printf("  --interactive     Enable interactive mode (kill process with 'k')\n");
  printf("  --version         Show version information\n");
  printf("  --help            Show this help message\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s --port 8080\n", program_name);
  printf("  %s --pid 1234 --tree\n", program_name);
  printf("  %s --all --short\n", program_name);
  printf("  %s --port 3000 --json\n", program_name);
  printf("  %s --pid 5678 --env\n", program_name);
  printf("\n");
}

/**
 * Parse a string to an integer with comprehensive error checking
 *
 * Converts a string representation of a number to an integer value using
 * strtol() with full validation. This function is used internally to parse
 * numeric arguments like PID and port numbers from command-line input.
 *
 * Error checking includes:
 * - Range validation: ensures value fits within INT_MIN to INT_MAX
 * - Format validation: ensures the entire string is a valid number
 * - Overflow detection: checks for ERANGE errno from strtol()
 * - Empty string detection: ensures string contains at least one digit
 * - Trailing characters: ensures no invalid characters after the number
 *
 * @param str String to parse (should contain only a valid decimal integer)
 * @param out Pointer to integer where the parsed value will be stored
 * @return 0 on success (valid integer parsed), -1 on error (invalid format, overflow, or out of range)
 */
static int parse_int(const char *str, int *out) {
  char *endPtr;
  const long val = strtol(str, &endPtr, 10);
  errno = 0;

  /* Check for various error conditions */
  if (errno == ERANGE || val > INT_MAX || val < INT_MIN) {
    return -1;
  }

  if (endPtr == str || *endPtr != '\0') {
    return -1;
  }

  *out = (int)val;
  return 0;
}

/**
 * Parse command-line arguments and populate the cli_args_t structure
 *
 * Processes command-line arguments to determine the operating mode and flags.
 * Initializes the args structure with default values, then parses each argument
 * to set appropriate modes and options. Handles special cases like --help and
 * --version which return immediately.
 *
 * Supported arguments:
 * - --help, -h: Display help message
 * - --version, -v: Display version information
 * - --pid <n>: Analyze specific process ID
 * - --port <n>: Analyze processes using specific port
 * - --all: List all running processes
 * - --short: One-line summary output
 * - --tree: Show full process ancestry tree
 * - --json: Output in JSON format
 * - --warnings: Show only warnings
 * - --no-color: Disable colorized output
 * - --env: Show environment variables
 * - --interactive, -i: Enable interactive mode
 *
 * @param argc Argument count from main()
 * @param argv Argument vector from main()
 * @param args Pointer to cli_args_t structure to populate with parsed arguments
 * @return 0 on success, -1 on error (invalid argument, missing value, or parse failure)
 */
int parse_args(const int argc, char **argv, cli_args_t *args) {
  memset(args, 0, sizeof(*args));
  args->mode = MODE_NONE;
  args->port = -1;
  args->pid = -1;

  /* No arguments - show help */
  if (argc < 2) {
    args->mode = MODE_HELP;
    return 0;
  }

  /* Parse each argument */
  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      args->mode = MODE_HELP;
      return 0;
    }
    if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
      args->mode = MODE_VERSION;
      return 0;
    }
    if (strcmp(arg, "--all") == 0) {
      if (args->mode == MODE_NONE) {
        args->mode = MODE_ALL;
      }
    } else if (strcmp(arg, "--port") == 0) {
      if (i + 1 >= argc) {
        print_error("--port requires an argument");
        return -1;
      }

      int port;
      if (parse_int(argv[++i], &port) < 0) {
        print_error("Invalid port number: %s", argv[i]);
        return -1;
      }

      if (port < 1 || port > 65535) {
        print_error("Port must be between 1 and 65535");
        return -1;
      }

      args->port = port;
      if (args->mode == MODE_NONE) {
        args->mode = MODE_PORT;
      }
    } else if (strcmp(arg, "--pid") == 0) {
      if (i + 1 >= argc) {
        print_error("--pid requires an argument");
        return -1;
      }

      int pid;
      if (parse_int(argv[++i], &pid) < 0) {
        print_error("Invalid PID: %s", argv[i]);
        return -1;
      }

      if (pid < 1) {
        print_error("PID must be positive");
        return -1;
      }

      args->pid = pid;
      if (args->mode == MODE_NONE) {
        args->mode = MODE_PID;
      }
    } else if (strcmp(arg, "--short") == 0) {
      args->short_output = true;
    } else if (strcmp(arg, "--tree") == 0) {
      args->show_tree = true;
    } else if (strcmp(arg, "--json") == 0) {
      args->json_output = true;
    } else if (strcmp(arg, "--warnings") == 0) {
      args->warnings_only = true;
    } else if (strcmp(arg, "--no-color") == 0) {
      args->no_color = true;
    } else if (strcmp(arg, "--env") == 0) {
      args->show_env = true;
    } else if (strcmp(arg, "--interactive") == 0 || strcmp(arg, "-i") == 0) {
      args->interactive = true;
    } else {
      print_error("Unknown option: %s", arg);
      return -1;
    }
  }

  return 0;
}

/**
 * Validate parsed arguments for logical consistency and compatibility
 *
 * Performs semantic validation on the parsed command-line arguments to ensure
 * they form a valid and consistent configuration. This function is called after
 * parse_args() to catch logical conflicts between options that are individually
 * valid but cannot be used together.
 *
 * Validation rules enforced:
 * - Mode requirement: Must specify --port, --pid, or --all (unless help/version)
 * - Mode exclusivity: Cannot combine --port and --pid together
 * - Mode exclusivity: Cannot combine --all with --port or --pid
 * - Output format limit: Cannot use multiple output formats simultaneously
 *   (--short, --json, --tree, --env are mutually exclusive)
 * - Context validation: --env requires --pid mode
 * - Context validation: --tree requires --pid mode
 * - Context validation: --warnings requires --port mode
 * - Context validation: --interactive requires --pid or --port mode
 * - Compatibility: --interactive cannot be used with --json
 *
 * @param args Pointer to cli_args_t structure containing parsed arguments
 * @return 0 if arguments are valid and consistent, -1 if validation fails
 */
int validate_args(const cli_args_t *args) {
  /* Must have either --port, --pid, or --all (unless showing help) */
  if (args->mode == MODE_NONE) {
    print_error("Must specify either --port, --pid, or --all");
    return -1;
  }

  /* Can't have both --port and --pid */
  if (args->port != -1 && args->pid != -1) {
    print_error("Cannot specify both --port and --pid");
    return -1;
  }

  /* Can't combine --all with --port or --pid */
  if (args->mode == MODE_ALL && (args->port != -1 || args->pid != -1)) {
    print_error("Cannot combine --all with --port or --pid");
    return -1;
  }

  /* Can't have multiple output formats */
  int output_formats = 0;
  if (args->short_output)
    output_formats++;
  if (args->json_output)
    output_formats++;
  if (args->show_tree)
    output_formats++;
  if (args->show_env)
    output_formats++;

  if (output_formats > 1) {
    print_error("Cannot specify multiple output formats (--short, --json, "
                "--tree, --env)");
    return -1;
  }

  /* --env only makes sense with --pid */
  if (args->show_env && args->mode != MODE_PID) {
    print_error("--env can only be used with --pid");
    return -1;
  }

  /* --tree only makes sense with --pid */
  if (args->show_tree && args->mode != MODE_PID) {
    print_error("--tree can only be used with --pid");
    return -1;
  }

  /* --warnings only make sense with --port */
  if (args->warnings_only && args->mode != MODE_PORT) {
    print_error("--warnings can only be used with --port");
    return -1;
  }

  /* --interactive only makes sense with --pid or --port */
  if (args->interactive && args->mode != MODE_PID && args->mode != MODE_PORT) {
    print_error("--interactive can only be used with --pid or --port");
    return -1;
  }

  /* --interactive doesn't work with JSON output */
  if (args->interactive && args->json_output) {
    print_error("--interactive cannot be used with --json");
    return -1;
  }

  return 0;
}
