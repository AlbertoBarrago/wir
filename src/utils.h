#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Global flag for color output */
extern bool use_colors;

/* ANSI Color codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/* Color printing functions */
void print_color(const char *color, const char *format, ...);
void print_error(const char *format, ...);
void print_warning(const char *format, ...);
void print_success(const char *format, ...);
void print_info(const char *format, ...);

/* Memory allocation wrappers with error checking */
void *safe_malloc(size_t size);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

/* String utilities */
char *trim_whitespace(char *str);
bool str_starts_with(const char *str, const char *prefix);
bool str_ends_with(const char *str, const char *suffix);

/* Interactive utilities */
char read_single_char(void);
int prompt_kill_process(pid_t pid, const char *process_name);

/* Error handling macros */
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
