#include "utils.h"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>

/* Global flag for controlling color output */
bool use_colors = true;

/**
 * Print text with color support
 *
 * Prints formatted text to stdout with optional ANSI color codes. If colors are
 * enabled globally (use_colors), wraps output in the specified color code and
 * resets afterward. Supports printf-style format strings with variable arguments.
 *
 * Behavior:
 * - If use_colors is false, prints without color codes
 * - If color is NULL or empty, prints without color
 * - Otherwise, wraps output with color code and COLOR_RESET
 *
 * @param color ANSI color code string (e.g., COLOR_RED, COLOR_GREEN) or NULL for no color
 * @param format printf-style format string
 * @param ... Variable arguments for format string
 * @return void
 */
void print_color(const char *color, const char *format, ...) {
    va_list args;

    if (use_colors && color && *color) {
        printf("%s", color);
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    if (use_colors && color && *color) {
        printf("%s", COLOR_RESET);
    }
}

/**
 * Print error message in red to stderr
 *
 * Outputs an error message with "Error: " prefix to stderr. If colors are enabled,
 * displays in red (COLOR_RED). Uses printf-style format strings with variable arguments.
 * Automatically adds newline at the end.
 *
 * @param format printf-style format string for the error message
 * @param ... Variable arguments for format string
 * @return void
 */
void print_error(const char *format, ...) {
    va_list args;

    if (use_colors) {
        fprintf(stderr, "%s", COLOR_RED);
    }

    fprintf(stderr, "Error: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");

    if (use_colors) {
        fprintf(stderr, "%s", COLOR_RESET);
    }
}

/**
 * Print warning message in yellow to stderr
 *
 * Outputs a warning message with "Warning: " prefix to stderr. If colors are enabled,
 * displays in yellow (COLOR_YELLOW). Uses printf-style format strings with variable
 * arguments. Automatically adds newline at the end.
 *
 * @param format printf-style format string for the warning message
 * @param ... Variable arguments for format string
 * @return void
 */
void print_warning(const char *format, ...) {
    va_list args;

    if (use_colors) {
        fprintf(stderr, "%s", COLOR_YELLOW);
    }

    fprintf(stderr, "Warning: ");

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");

    if (use_colors) {
        fprintf(stderr, "%s", COLOR_RESET);
    }
}

/**
 * Print success message in green to stdout
 *
 * Outputs a success message to stdout. If colors are enabled, displays in green
 * (COLOR_GREEN). Uses printf-style format strings with variable arguments.
 * Automatically adds newline at the end.
 *
 * @param format printf-style format string for the success message
 * @param ... Variable arguments for format string
 * @return void
 */
void print_success(const char *format, ...) {
    va_list args;

    if (use_colors) {
        printf("%s", COLOR_GREEN);
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");

    if (use_colors) {
        printf("%s", COLOR_RESET);
    }
}

/**
 * Print informational message in cyan to stdout
 *
 * Outputs an informational message to stdout. If colors are enabled, displays in cyan
 * (COLOR_CYAN). Uses printf-style format strings with variable arguments.
 * Automatically adds newline at the end.
 *
 * @param format printf-style format string for the info message
 * @param ... Variable arguments for format string
 * @return void
 */
void print_info(const char *format, ...) {
    va_list args;

    if (use_colors) {
        printf("%s", COLOR_CYAN);
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");

    if (use_colors) {
        printf("%s", COLOR_RESET);
    }
}

/**
 * Safe malloc wrapper with error checking
 *
 * Allocates memory using malloc() and terminates the program if allocation fails.
 * Provides guaranteed memory allocation with automatic error handling, eliminating
 * the need for null checks at every allocation site.
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory (never returns NULL)
 * @note Calls DIE() and terminates program if allocation fails
 */
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        DIE("Memory allocation failed: %s", strerror(errno));
    }
    return ptr;
}

/**
 * Safe calloc wrapper with error checking
 *
 * Allocates zero-initialized memory using calloc() and terminates the program if
 * allocation fails. Provides guaranteed memory allocation with automatic error
 * handling. Memory is initialized to zero.
 *
 * @param nmemb Number of elements to allocate
 * @param size Size of each element in bytes
 * @return Pointer to allocated zero-initialized memory (never returns NULL)
 * @note Calls DIE() and terminates program if allocation fails
 */
void *safe_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        DIE("Memory allocation failed: %s", strerror(errno));
    }
    return ptr;
}

/**
 * Safe realloc wrapper with error checking
 *
 * Reallocates memory using realloc() and terminates the program if reallocation fails.
 * Provides guaranteed memory reallocation with automatic error handling. Only fails
 * and terminates if size > 0 and reallocation fails.
 *
 * @param ptr Pointer to previously allocated memory (or NULL for initial allocation)
 * @param size New size in bytes
 * @return Pointer to reallocated memory (never returns NULL if size > 0)
 * @note Calls DIE() and terminates program if allocation fails and size > 0
 */
void *safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        DIE("Memory reallocation failed: %s", strerror(errno));
    }
    return new_ptr;
}

/**
 * Safe strdup wrapper with error checking
 *
 * Duplicates a string using strdup() and terminates the program if allocation fails.
 * Provides guaranteed string duplication with automatic error handling. Returns NULL
 * if input is NULL (does not allocate).
 *
 * @param s String to duplicate (NULL returns NULL without allocating)
 * @return Pointer to duplicated string (never returns NULL for non-NULL input)
 * @note Calls DIE() and terminates program if allocation fails for non-NULL input
 */
char *safe_strdup(const char *s) {
    if (!s) {
        return NULL;
    }

    char *dup = strdup(s);
    if (!dup) {
        DIE("String duplication failed: %s", strerror(errno));
    }
    return dup;
}

/**
 * Trim leading and trailing whitespace from a string (in-place)
 *
 * Removes all leading and trailing whitespace characters (as defined by isspace())
 * from a string by modifying it in-place. Returns a pointer to the first non-whitespace
 * character, and writes a null terminator after the last non-whitespace character.
 *
 * Behavior:
 * - Returns NULL if input is NULL
 * - Returns pointer to empty string if input is all whitespace
 * - Modifies the original string by writing new null terminator
 * - Returns pointer into the original string (not a new allocation)
 *
 * @param str String to trim (modified in-place)
 * @return Pointer to trimmed string (within original string buffer), or NULL if input is NULL
 */
char *trim_whitespace(char *str) {
    if (!str) {
        return NULL;
    }

    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Trim trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    /* Write new null terminator */
    *(end + 1) = '\0';

    return str;
}

/**
 * Check if string starts with a prefix
 *
 * Tests whether a string begins with a specified prefix by comparing the first
 * prefix_len characters. Case-sensitive comparison.
 *
 * @param str String to test (NULL returns false)
 * @param prefix Prefix to search for (NULL returns false)
 * @return true if str starts with prefix, false otherwise
 */
bool str_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) {
        return false;
    }

    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);

    if (prefix_len > str_len) {
        return false;
    }

    return strncmp(str, prefix, prefix_len) == 0;
}

/**
 * Check if string ends with a suffix
 *
 * Tests whether a string ends with a specified suffix by comparing the last
 * suffix_len characters. Case-sensitive comparison.
 *
 * @param str String to test (NULL returns false)
 * @param suffix Suffix to search for (NULL returns false)
 * @return true if str ends with suffix, false otherwise
 */
bool str_ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/**
 * Read a single character from stdin without waiting for Enter
 *
 * Reads one character directly from stdin in raw mode, without requiring the user
 * to press Enter. Temporarily modifies terminal settings to disable canonical mode
 * and echo, then restores original settings before returning.
 *
 * The function:
 * 1. Saves current terminal settings
 * 2. Disables canonical mode (line buffering) and echo
 * 3. Reads one character with getchar()
 * 4. Restores original terminal settings
 *
 * Used for interactive prompts where immediate response is desired.
 *
 * @return The character read from stdin
 */
char read_single_char(void) {
    struct termios old_term, new_term;
    char ch;

    /* Get current terminal settings */
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;

    /* Disable canonical mode and echo */
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    /* Read single character */
    ch = getchar();

    /* Restore old terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    return ch;
}

/**
 * Prompt user to kill a process interactively
 *
 * Displays an interactive prompt asking the user whether to kill a specific process.
 * Reads a single character and performs the appropriate action:
 * - 'k'/'K': Sends SIGTERM to the process, waits 100ms, then checks if it terminated
 * - 'q'/'Q': Quits without killing the process
 * - Any other key: Exits interactive mode
 *
 * Error handling:
 * - EPERM: Permission denied (suggests using sudo)
 * - ESRCH: Process no longer exists
 * - Other errors: Displays error message with strerror()
 *
 * After sending SIGTERM, checks if process still exists and informs user if
 * SIGKILL (-9) may be needed for forceful termination.
 *
 * @param pid Process ID to potentially kill
 * @param process_name Name of process (for display in messages)
 * @return 0 if process was killed successfully, -1 otherwise (quit, error, or user declined)
 */
int prompt_kill_process(pid_t pid, const char *process_name) {
    printf("\n");
    print_color(COLOR_YELLOW, "Press 'k' to kill process, 'q' to quit, or any other key to exit: ");
    fflush(stdout);

    char ch = read_single_char();
    printf("\n");

    if (ch == 'k' || ch == 'K') {
        /* Attempt to kill the process */
        if (kill(pid, SIGTERM) == 0) {
            print_success("Successfully sent SIGTERM to process %d (%s)", pid, process_name);

            /* Give the process a moment to terminate gracefully */
            usleep(100000); /* 100ms */

            /* Check if process still exists */
            if (kill(pid, 0) == 0) {
                print_info("Process is still running. You may need to use SIGKILL (kill -9) if it doesn't terminate.");
            } else {
                print_success("Process %d has been terminated", pid);
            }
            return 0;
        } else {
            if (errno == EPERM) {
                print_error("Permission denied. You may need to run with sudo to kill this process.");
            } else if (errno == ESRCH) {
                print_error("Process %d no longer exists", pid);
            } else {
                print_error("Failed to kill process %d: %s", pid, strerror(errno));
            }
            return -1;
        }
    } else if (ch == 'q' || ch == 'Q') {
        print_info("Quit without killing process");
        return -1;
    } else {
        print_info("Exiting interactive mode");
        return -1;
    }
}

/**
 * Convert process state character to readable name
 *
 * Maps single-character process state codes (from /proc/<pid>/stat on Linux or
 * BSD status codes) to human-readable state names.
 *
 * State mappings:
 * - 'R': Running
 * - 'S': Sleeping (interruptible sleep)
 * - 'D': Waiting (uninterruptible disk sleep)
 * - 'Z': Zombie (terminated but not reaped)
 * - 'T': Stopped (by job control signal)
 * - 't': Tracing Stop (stopped by debugger)
 * - 'I': Idle (kernel thread)
 * - 'W': Waking
 * - 'X'/'x': Dead
 * - 'K': Wakekill
 * - 'P': Parked
 * - '?': Unknown
 *
 * @param state Single character process state code
 * @return Pointer to static string containing human-readable state name
 */
const char *get_state_name(char state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Waiting (Disk Sleep)";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing Stop";
        case 'I': return "Idle";
        case 'W': return "Waking";
        case 'X': return "Dead";
        case 'x': return "Dead";
        case 'K': return "Wakekill";
        case 'P': return "Parked";
        case '?': return "Unknown";
        default:  return "Unknown";
    }
}

/**
 * Format process uptime into human-readable string
 *
 * Calculates how long a process has been running and formats it as a human-readable
 * string like "2 days, 3 hours, 45 minutes". Shows only relevant time units and
 * uses proper singular/plural forms.
 *
 * Formatting rules:
 * - Always shows days, hours, minutes if non-zero
 * - Shows seconds only if total uptime is less than 1 hour
 * - Uses comma separation between units
 * - Handles singular/plural ("1 day" vs "2 days")
 * - Returns "Unknown" if start_time is 0 or invalid (negative uptime)
 *
 * Examples:
 * - "5 seconds"
 * - "3 minutes, 45 seconds"
 * - "2 hours, 30 minutes"
 * - "1 day, 4 hours, 15 minutes"
 *
 * @param start_time Unix timestamp when process started (0 for unknown)
 * @param buffer Output buffer for formatted string
 * @param buffer_size Size of output buffer
 * @return void (result written to buffer)
 */
void format_uptime(const time_t start_time, char *buffer, size_t buffer_size) {
    if (start_time == 0) {
        snprintf(buffer, buffer_size, "Unknown");
        return;
    }

    const time_t now = time(NULL);
    const time_t uptime = now - start_time;

    if (uptime < 0) {
        snprintf(buffer, buffer_size, "Unknown");
        return;
    }

    const int days = uptime / 86400;
    const int hours = (uptime % 86400) / 3600;
    const int minutes = (uptime % 3600) / 60;
    const int seconds = uptime % 60;

    /* Build the string based on what's relevant */
    char *ptr = buffer;
    size_t remaining = buffer_size;
    int written = 0;
    bool need_comma = false;

    if (days > 0) {
        written = snprintf(ptr, remaining, "%d day%s", days, days > 1 ? "s" : "");
        ptr += written;
        remaining -= written;
        need_comma = true;
    }

    if (hours > 0 || days > 0) {
        if (need_comma) {
            written = snprintf(ptr, remaining, ", ");
            ptr += written;
            remaining -= written;
        }
        written = snprintf(ptr, remaining, "%d hour%s", hours, hours != 1 ? "s" : "");
        ptr += written;
        remaining -= written;
        need_comma = true;
    }

    if (minutes > 0 || hours > 0 || days > 0) {
        if (need_comma) {
            written = snprintf(ptr, remaining, ", ");
            ptr += written;
            remaining -= written;
        }
        written = snprintf(ptr, remaining, "%d minute%s", minutes, minutes != 1 ? "s" : "");
        ptr += written;
        remaining -= written;
        need_comma = true;
    }

    /* Only show seconds if uptime is less than an hour */
    if (days == 0 && hours == 0) {
        if (need_comma) {
            written = snprintf(ptr, remaining, ", ");
            ptr += written;
            remaining -= written;
        }
        snprintf(ptr, remaining, "%d second%s", seconds, seconds != 1 ? "s" : "");
    }
}
