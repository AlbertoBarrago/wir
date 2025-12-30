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
 * @param color ANSI color code (or NULL/empty for no color)
 * @param format printf-style format string
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
 * Print error message in red
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
 * Print warning message in yellow
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
 * Print success message in green
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
 * Print info message in cyan
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
 * Safe malloc with error checking
 * Dies if allocation fails
 */
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        DIE("Memory allocation failed: %s", strerror(errno));
    }
    return ptr;
}

/**
 * Safe calloc with error checking
 * Dies if allocation fails
 */
void *safe_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        DIE("Memory allocation failed: %s", strerror(errno));
    }
    return ptr;
}

/**
 * Safe realloc with error checking
 * Dies if allocation fails
 */
void *safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        DIE("Memory reallocation failed: %s", strerror(errno));
    }
    return new_ptr;
}

/**
 * Safe strdup with error checking
 * Dies if allocation fails
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
 * Returns the trimmed string
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
 * Returns the character read
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
 * Prompt user to kill a process
 * Returns 0 if process was killed, -1 otherwise
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
 * Returns the full state name as a string
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
 * Format process uptime (how long it's been running)
 * Outputs a human-readable string like "2 days, 3 hours, 45 minutes"
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
