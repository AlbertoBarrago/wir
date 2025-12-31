#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/**
 * Global flag for controlling colored output
 *
 * When true, output functions use ANSI color codes. When false, output is plain text.
 * Can be disabled with --no-color flag. Defaults to true.
 */
extern bool use_colors;

/**
 * ANSI Color codes for terminal output
 *
 * Standard ANSI escape sequences for coloring terminal text. Used throughout
 * the application for colored output when use_colors is enabled.
 */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/**
 * Color printing functions
 *
 * See src/utils.c for detailed documentation of each function.
 */
void print_color(const char *color, const char *format, ...);
void print_error(const char *format, ...);
void print_warning(const char *format, ...);
void print_success(const char *format, ...);
void print_info(const char *format, ...);

/**
 * Memory allocation wrappers with error checking
 *
 * Safe wrappers around standard allocation functions. Terminate the program
 * with error message if allocation fails. See src/utils.c for detailed documentation.
 */
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

/**
 * String utility functions
 *
 * Helper functions for string manipulation and testing.
 * See src/utils.c for detailed documentation.
 */
char *trim_whitespace(char *str);
bool str_starts_with(const char *str, const char *prefix);
bool str_ends_with(const char *str, const char *suffix);

/**
 * Process utility functions
 *
 * Helper functions for formatting and interpreting process information.
 * See src/utils.c for detailed documentation.
 */
const char *get_state_name(char state);
void format_uptime(time_t start_time, char *buffer, size_t buffer_size);

/**
 * Interactive utility functions
 *
 * Functions for interactive terminal input and process management.
 * See src/utils.c for detailed documentation.
 */
char read_single_char(void);
int prompt_kill_process(pid_t pid, const char *process_name);

/**
 * Error handling macros
 *
 * DIE: Fatal error macro - prints error message to stderr and exits with failure status
 * WARN: Warning macro - prints warning message to stderr but continues execution
 * DEBUG_PRINT: Debug printing macro - only active when DEBUG is defined, includes file and line info
 *
 * Usage:
 *   DIE("Failed to open file: %s", filename);
 *   WARN("Retrying connection...");
 *   DEBUG_PRINT("Variable value: %d", value);
 */
#define DIE(fmt, ...) do { \
    fprintf(stderr, "Fatal error: " fmt "\n", ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while(0)

#define WARN(fmt, ...) do { \
    fprintf(stderr, "Warning: " fmt "\n", ##__VA_ARGS__); \
} while(0)

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) do {} while(0)
#endif

#endif /* UTILS_H */
