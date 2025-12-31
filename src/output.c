#include "output.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

/**
 * Output process info in normal (pretty) format
 *
 * Displays detailed process information in a human-readable, colored format.
 * Shows all available process details including PID, name, user, parent process,
 * state, uptime, command line, and memory usage.
 *
 * Output includes:
 * - PID and process name
 * - User information (username and UID)
 * - Parent process ID
 * - Process state with human-readable name
 * - Running time (uptime since process start)
 * - Full command line (if available)
 * - Memory usage (VSZ and RSS in KB)
 *
 * @param info Pointer to process_info_t structure containing process details
 * @return void
 */
static void output_process_normal(const process_info_t *info) {
    print_color(COLOR_BOLD, "Process Information\n");
    print_color(COLOR_CYAN, "  PID: ");
    printf("%d\n", info->pid);

    print_color(COLOR_CYAN, "  Name: ");
    printf("%s\n", info->name);

    print_color(COLOR_CYAN, "  User: ");
    printf("%s (UID: %d)\n", info->username, info->uid);

    print_color(COLOR_CYAN, "  Parent PID: ");
    printf("%d\n", info->ppid);

    print_color(COLOR_CYAN, "  State: ");
    printf("%s (%c)\n", get_state_name(info->state), info->state);

    print_color(COLOR_CYAN, "  Running for: ");
    char uptime_buf[128];
    format_uptime(info->start_time, uptime_buf, sizeof(uptime_buf));
    printf("%s\n", uptime_buf);

    if (info->cmdline[0]) {
        print_color(COLOR_CYAN, "  Command: ");
        printf("%s\n", info->cmdline);
    }

    print_color(COLOR_CYAN, "  Memory: ");
    printf("VSZ=%lu KB, RSS=%lu KB\n", info->vsz, info->rss);
}

/**
 * Output process info in a short (one-line) format
 *
 * Displays concise process information in a single line, useful for quick
 * overview or when space is limited. Format: "PID <pid>: <name>[<ppid>] by <user> - <cmdline>"
 *
 * @param info Pointer to process_info_t structure containing process details
 * @return void
 */
static void output_process_short(const process_info_t *info) {
    printf("PID %d: %s[%d] by %s - %s\n",
           info->pid, info->name, info->ppid, info->username,
           info->cmdline[0] ? info->cmdline : "(no cmdline)");
}

/**
 * Output process info in JSON format
 *
 * Serializes process information as a JSON object for programmatic consumption.
 * Includes all process details in a structured format suitable for parsing,
 * logging, or integration with other tools.
 *
 * JSON structure includes:
 * - pid, name, ppid, user, uid
 * - state (character code) and state_name (human-readable)
 * - start_time (epoch timestamp) and uptime (formatted string)
 * - cmdline (full command line)
 * - memory object with vsz_kb and rss_kb
 *
 * @param info Pointer to process_info_t structure containing process details
 * @return void
 */
static void output_process_json(const process_info_t *info) {
    char uptime_buf[128];
    format_uptime(info->start_time, uptime_buf, sizeof(uptime_buf));

    printf("{\n");
    printf("  \"pid\": %d,\n", info->pid);
    printf("  \"name\": \"%s\",\n", info->name);
    printf("  \"ppid\": %d,\n", info->ppid);
    printf("  \"user\": \"%s\",\n", info->username);
    printf("  \"uid\": %d,\n", info->uid);
    printf("  \"state\": \"%c\",\n", info->state);
    printf("  \"state_name\": \"%s\",\n", get_state_name(info->state));
    printf("  \"start_time\": %ld,\n", (long)info->start_time);
    printf("  \"uptime\": \"%s\",\n", uptime_buf);
    printf("  \"cmdline\": \"%s\",\n", info->cmdline);
    printf("  \"memory\": {\n");
    printf("    \"vsz_kb\": %lu,\n", info->vsz);
    printf("    \"rss_kb\": %lu\n", info->rss);
    printf("  }\n");
    printf("}\n");
}

/**
 * Output process information with format selection
 *
 * Main entry point for displaying process information. Selects the appropriate
 * output format based on command-line arguments (JSON, short, or normal).
 * Optionally prompts for interactive process termination if --interactive
 * flag is enabled.
 *
 * Format selection:
 * - JSON format if args->json_output is true
 * - Short (one-line) format if args->short_output is true
 * - Normal (pretty) format otherwise
 *
 * Interactive mode:
 * - If args->interactive is true and not JSON output, prompts user to kill process
 *
 * @param info Pointer to process_info_t structure containing process details
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success
 */
int output_process_info(const process_info_t *info, const cli_args_t *args) {
    if (args->json_output) {
        output_process_json(info);
    } else if (args->short_output) {
        output_process_short(info);
    } else {
        output_process_normal(info);
    }

    /* Interactive mode - prompt to kill process */
    if (args->interactive && !args->json_output) {
        prompt_kill_process(info->pid, info->name);
    }

    return 0;
}

/* ============================================================================
 * PROCESS TREE OUTPUT
 * ============================================================================ */

/**
 * Recursively print a process tree in ASCII art format
 *
 * Renders a process ancestry tree using box-drawing characters (├─ and └─)
 * to show parent-child relationships. Recursively traverses up the tree from
 * the target process to the root ancestor, displaying each process with
 * indentation and tree symbols.
 *
 * Tree visualization:
 * - Uses "├─" for non-last branches
 * - Uses "└─" for last branches
 * - Indents with two spaces per depth level
 * - Colors process names in green
 * - Shows PID in brackets and username in parentheses
 *
 * @param node Pointer to process_tree_node_t to print (NULL-safe)
 * @param depth Current depth level in tree (0 = root)
 * @param is_last Boolean indicating if this is the last child at current depth
 * @return void
 */
static void print_tree_recursive(const process_tree_node_t *node, int depth, bool is_last) {
    if (!node) {
        return;
    }

    /* Print indentation and tree characters */
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    if (depth > 0) {
        printf("%s ", is_last ? "└─" : "├─");
    }

    /* Print process info */
    print_color(COLOR_GREEN, "%s", node->info.name);
    printf("[%d]", node->info.pid);

    if (node->info.username[0]) {
        printf(" (%s)", node->info.username);
    }

    printf("\n");

    /* Print parent (going up the tree) */
    if (node->parent) {
        print_tree_recursive(node->parent, depth + 1, true);
    }
}

/**
 * Output process tree in JSON format recursively
 *
 * Serializes a process ancestry tree as nested JSON objects. Each node contains
 * pid, name, user, and optionally a nested parent object. The recursion travels
 * up the tree, with each parent embedded within its child.
 *
 * JSON structure:
 * - Each node has: pid, name, user
 * - If parent exists, includes "parent" key with nested parent object
 * - Proper indentation based on depth for readability
 *
 * @param node Pointer to process_tree_node_t to serialize (NULL-safe)
 * @param depth Current depth level for indentation (0 = root)
 * @return void
 */
static void output_tree_json_recursive(const process_tree_node_t *node, int depth) {
    if (!node) {
        return;
    }

    for (int i = 0; i < depth; i++) printf("  ");
    printf("{\n");

    for (int i = 0; i < depth + 1; i++) printf("  ");
    printf("\"pid\": %d,\n", node->info.pid);

    for (int i = 0; i < depth + 1; i++) printf("  ");
    printf("\"name\": \"%s\",\n", node->info.name);

    for (int i = 0; i < depth + 1; i++) printf("  ");
    printf("\"user\": \"%s\"", node->info.username);

    if (node->parent) {
        printf(",\n");
        for (int i = 0; i < depth + 1; i++) printf("  ");
        printf("\"parent\": ");
        output_tree_json_recursive(node->parent, depth + 1);
    } else {
        printf("\n");
    }

    for (int i = 0; i < depth; i++) printf("  ");
    printf("}");
    if (depth == 0) {
        printf("\n");
    }
}

/**
 * Output process tree with format selection
 *
 * Main entry point for displaying process ancestry tree. Shows the complete
 * lineage from the target process up to its root ancestor (typically init/PID 1).
 * Selects output format based on command-line arguments.
 *
 * Format selection:
 * - JSON format if args->json_output is true
 * - ASCII tree format (with box-drawing characters) otherwise
 *
 * @param tree Pointer to process_tree_node_t representing the target process (leaf of tree)
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success, -1 if tree is NULL
 */
int output_process_tree(const process_tree_node_t *tree, const cli_args_t *args) {
    if (!tree) {
        print_error("No process tree available");
        return -1;
    }

    if (args->json_output) {
        output_tree_json_recursive(tree, 0);
    } else {
        print_color(COLOR_BOLD, "Process Ancestry Tree\n");
        print_tree_recursive(tree, 0, true);
    }

    return 0;
}

/* ============================================================================
 * ENVIRONMENT VARIABLES OUTPUT
 * ============================================================================ */

/**
 * Output environment variables for a process
 *
 * Displays all environment variables from a process. In normal mode, formats
 * variables with colored names and values separated by '='. In JSON mode,
 * outputs as an array with count.
 *
 * Normal format:
 * - Header showing total count
 * - Each variable with colored name (cyan) and value
 * - Temporarily modifies string to split at '=' for formatting
 *
 * JSON format:
 * - Array of environment variable strings
 * - Total count field
 *
 * @param env_vars Array of environment variable strings (format: "NAME=value")
 * @param count Number of environment variables in array
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success
 */
int output_process_env(char **env_vars, int count, const cli_args_t *args) {
    if (args->json_output) {
        printf("{\n");
        printf("  \"environment\": [\n");
        for (int i = 0; i < count; i++) {
            printf("    \"%s\"", env_vars[i]);
            if (i < count - 1) {
                printf(",");
            }
            printf("\n");
        }
        printf("  ],\n");
        printf("  \"count\": %d\n", count);
        printf("}\n");
    } else {
        print_color(COLOR_BOLD, "Environment Variables (%d total)\n", count);
        for (int i = 0; i < count; i++) {
            /* Split variable into name and value */
            char *equals = strchr(env_vars[i], '=');
            if (equals) {
                *equals = '\0';
                print_color(COLOR_CYAN, "  %s", env_vars[i]);
                printf("=%s\n", equals + 1);
                *equals = '='; /* Restore */
            } else {
                printf("  %s\n", env_vars[i]);
            }
        }
    }

    return 0;
}

/* ============================================================================
 * PORT OUTPUT
 * ============================================================================ */

/**
 * Check if connection has warning conditions
 *
 * Evaluates whether a network connection and its associated process exhibit
 * potentially problematic conditions that warrant user attention.
 *
 * Warning conditions:
 * - Process running as root (UID 0) on non-system port (>= 1024)
 *   This is a security concern as privileged processes shouldn't bind to user ports
 * - Process is in zombie state (state 'Z')
 *   Zombie processes holding ports indicate improper cleanup
 *
 * @param conn Pointer to connection_info_t structure
 * @param proc Pointer to process_info_t structure (NULL-safe)
 * @return true if warning conditions exist, false otherwise
 */
static bool has_warning(const connection_info_t *conn, const process_info_t *proc) {
    /* Running as root on non-system ports */
    if (proc && proc->uid == 0 && conn->local_port >= 1024) {
        return true;
    }

    /* Zombie process */
    if (proc && proc->state == 'Z') {
        return true;
    }

    return false;
}

/**
 * Output port info in normal (detailed) format
 *
 * Displays comprehensive information about all connections on a port in a
 * human-readable format. For each connection, shows network details and
 * associated process information. Displays warnings for security concerns.
 *
 * For each connection shows:
 * - Protocol (TCP/UDP) and state
 * - Local and remote addresses with ports
 * - Process details (name, PID, user, command)
 * - Security warnings if applicable (root on user port, zombie process)
 *
 * @param port Port number being queried
 * @param connections Array of connection_info_t structures
 * @param count Number of connections in array
 * @return void
 */
static void output_port_normal(int port, const connection_info_t *connections, int count) {
    print_color(COLOR_BOLD, "Port %d Connections (%d found)\n", port, count);

    for (int i = 0; i < count; i++) {
        const connection_info_t *conn = &connections[i];

        printf("\n");
        print_color(COLOR_CYAN, "Connection #%d:\n", i + 1);
        printf("  Protocol: %s\n", conn->protocol);
        printf("  State: %s\n", conn->state);
        printf("  Local: %s:%d\n", conn->local_addr[0] ? conn->local_addr : "*",
               conn->local_port);

        if (conn->remote_port > 0) {
            printf("  Remote: %s:%d\n", conn->remote_addr, conn->remote_port);
        }

        if (conn->pid > 0) {
            process_info_t proc;
            if (platform_get_process_info(conn->pid, &proc) == 0) {
                print_color(COLOR_GREEN, "  Process: ");
                printf("%s (PID: %d)\n", proc.name, proc.pid);
                printf("  User: %s\n", proc.username);

                if (proc.cmdline[0]) {
                    printf("  Command: %s\n", proc.cmdline);
                }

                /* Show warning if applicable */
                if (has_warning(conn, &proc)) {
                    print_warning("Process running with elevated privileges (root)");
                }
            }
        } else {
            printf("  Process: Unknown\n");
        }
    }
}

/**
 * Output port info in short (one-line per connection) format
 *
 * Displays concise information about port connections, one line per connection.
 * Format: "Port <port>: <process>[<pid>] by <user> (<state>)"
 *
 * @param port Port number being queried
 * @param connections Array of connection_info_t structures
 * @param count Number of connections in array
 * @return void
 */
static void output_port_short(int port, const connection_info_t *connections, int count) {
    for (int i = 0; i < count; i++) {
        const connection_info_t *conn = &connections[i];

        if (conn->pid > 0) {
            process_info_t proc;
            if (platform_get_process_info(conn->pid, &proc) == 0) {
                printf("Port %d: %s[%d] by %s (%s)\n",
                       port, proc.name, proc.pid, proc.username, conn->state);
            }
        } else {
            printf("Port %d: Unknown process (%s)\n", port, conn->state);
        }
    }
}

/**
 * Output port info in JSON format
 *
 * Serializes port connection information as JSON for programmatic consumption.
 * Includes port number, connection count, and array of connection objects with
 * full network and process details.
 *
 * JSON structure:
 * - port: port number
 * - connection_count: total connections
 * - connections: array of connection objects
 *   Each connection includes: protocol, state, addresses, ports
 *   If process info available: nested process object with pid, name, user, cmdline
 *
 * @param port Port number being queried
 * @param connections Array of connection_info_t structures
 * @param count Number of connections in array
 * @return void
 */
static void output_port_json(int port, const connection_info_t *connections, int count) {
    printf("{\n");
    printf("  \"port\": %d,\n", port);
    printf("  \"connection_count\": %d,\n", count);
    printf("  \"connections\": [\n");

    for (int i = 0; i < count; i++) {
        const connection_info_t *conn = &connections[i];

        printf("    {\n");
        printf("      \"protocol\": \"%s\",\n", conn->protocol);
        printf("      \"state\": \"%s\",\n", conn->state);
        printf("      \"local_address\": \"%s\",\n", conn->local_addr);
        printf("      \"local_port\": %d,\n", conn->local_port);
        printf("      \"remote_address\": \"%s\",\n", conn->remote_addr);
        printf("      \"remote_port\": %d", conn->remote_port);

        if (conn->pid > 0) {
            process_info_t proc;
            if (platform_get_process_info(conn->pid, &proc) == 0) {
                printf(",\n");
                printf("      \"process\": {\n");
                printf("        \"pid\": %d,\n", proc.pid);
                printf("        \"name\": \"%s\",\n", proc.name);
                printf("        \"user\": \"%s\",\n", proc.username);
                printf("        \"cmdline\": \"%s\"\n", proc.cmdline);
                printf("      }\n");
            } else {
                printf("\n");
            }
        } else {
            printf("\n");
        }

        printf("    }%s\n", i < count - 1 ? "," : "");
    }

    printf("  ]\n");
    printf("}\n");
}

/**
 * Output port info in warnings-only format
 *
 * Analyzes port connections for security concerns and displays only warnings.
 * If no issues found, displays success message. Useful for security auditing.
 *
 * Checks for:
 * - Processes running as root on non-system ports (>= 1024)
 * - Zombie processes holding ports
 * - Multiple processes listening on same port (potential conflict)
 *
 * @param port Port number being queried
 * @param connections Array of connection_info_t structures
 * @param count Number of connections in array
 * @return void
 */
static void output_port_warnings(int port, const connection_info_t *connections, int count) {
    bool found_warning = false;

    print_color(COLOR_BOLD, "Port %d - Security Warnings\n", port);

    for (int i = 0; i < count; i++) {
        const connection_info_t *conn = &connections[i];

        if (conn->pid > 0) {
            process_info_t proc;
            if (platform_get_process_info(conn->pid, &proc) == 0) {
                if (has_warning(conn, &proc)) {
                    found_warning = true;

                    if (proc.uid == 0 && conn->local_port >= 1024) {
                        print_warning("Process '%s' (PID %d) running as root on non-system port",
                                    proc.name, proc.pid);
                    }

                    if (proc.state == 'Z') {
                        print_warning("Zombie process '%s' (PID %d) holding port",
                                    proc.name, proc.pid);
                    }
                }
            }
        }
    }

    /* Check for multiple processes on same port */
    if (count > 1) {
        found_warning = true;
        print_warning("Multiple processes (%d) listening on port %d", count, port);
    }

    if (!found_warning) {
        print_success("No warnings found for port %d", port);
    }
}

/**
 * Output port information with format selection
 *
 * Main entry point for displaying port connection information. Selects output
 * format based on command-line arguments. Returns error if no connections found.
 * Optionally prompts for interactive process termination.
 *
 * Format selection priority:
 * 1. Warnings-only if args->warnings_only is true
 * 2. JSON if args->json_output is true
 * 3. Short if args->short_output is true
 * 4. Normal (detailed) format otherwise
 *
 * Interactive mode:
 * - If args->interactive is true, prompts to kill first process on port
 *
 * @param port Port number being queried
 * @param connections Array of connection_info_t structures
 * @param count Number of connections in array
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success, -1 if no connections found
 */
int output_port_info(int port, const connection_info_t *connections,
                     int count, const cli_args_t *args) {
    if (count == 0) {
        print_error("No connections found on port %d", port);
        return -1;
    }

    if (args->warnings_only) {
        output_port_warnings(port, connections, count);
    } else if (args->json_output) {
        output_port_json(port, connections, count);
    } else if (args->short_output) {
        output_port_short(port, connections, count);
    } else {
        output_port_normal(port, connections, count);
    }

    /* Interactive mode - prompt to kill process(es) */
    if (args->interactive && !args->json_output && count > 0) {
        /* Get the first connection's PID */
        if (connections[0].pid > 0) {
            process_info_t proc;
            if (platform_get_process_info(connections[0].pid, &proc) == 0) {
                prompt_kill_process(proc.pid, proc.name);
            }
        }
    }

    return 0;
}

/* ============================================================================
 * PROCESS LIST OUTPUT
 * ============================================================================ */

/**
 * Output process list in normal (table) format
 *
 * Displays all processes in a formatted table with headers and aligned columns.
 * Shows PID, PPID, name, user, and command line. Includes header row with
 * column names and separator line. Total count displayed at bottom.
 *
 * Table columns:
 * - PID: Process ID (8 chars wide)
 * - PPID: Parent Process ID (8 chars wide)
 * - NAME: Process name (20 chars wide, colored green)
 * - USER: Username (12 chars wide, colored cyan)
 * - COMMAND: Command line (60 chars max)
 *
 * @param processes Array of process_info_t structures
 * @param count Number of processes in array
 * @return void
 */
static void output_process_list_normal(const process_info_t *processes, int count) {
    print_color(COLOR_BOLD, "Running Processes (%d total)\n", count);
    printf("\n");
    printf("%-8s %-8s %-20s %-12s %s\n", "PID", "PPID", "NAME", "USER", "COMMAND");
    print_color(COLOR_BOLD, "%-8s %-8s %-20s %-12s %s\n",
                "--------", "--------", "--------------------",
                "------------", "-------");

    for (int i = 0; i < count; i++) {
        const process_info_t *proc = &processes[i];

        printf("%-8d %-8d ", proc->pid, proc->ppid);
        print_color(COLOR_GREEN, "%-20.20s ", proc->name);
        print_color(COLOR_CYAN, "%-12.12s ", proc->username);
        printf("%.60s\n", proc->cmdline[0] ? proc->cmdline : "(no cmdline)");
    }

    printf("\n");
    print_color(COLOR_BOLD, "Total: %d processes\n", count);
}

/**
 * Output process list in short format (one per line)
 *
 * Displays minimal process information, one line per process.
 * Format: "<pid>: <name> by <user>"
 *
 * @param processes Array of process_info_t structures
 * @param count Number of processes in array
 * @return void
 */
static void output_process_list_short(const process_info_t *processes, int count) {
    for (int i = 0; i < count; i++) {
        const process_info_t *proc = &processes[i];
        printf("%d: %s by %s\n", proc->pid, proc->name, proc->username);
    }
}

/**
 * Output process list in JSON format
 *
 * Serializes all processes as a JSON object containing process count and array
 * of process objects. Each process object includes complete details like PID,
 * name, user, state, uptime, memory usage, etc.
 *
 * JSON structure:
 * - process_count: total number of processes
 * - processes: array of process objects
 *   Each process includes: pid, ppid, name, user, uid, state, state_name,
 *   start_time, uptime, cmdline, memory (with vsz_kb and rss_kb)
 *
 * @param processes Array of process_info_t structures
 * @param count Number of processes in array
 * @return void
 */
static void output_process_list_json(const process_info_t *processes, int count) {
    printf("{\n");
    printf("  \"process_count\": %d,\n", count);
    printf("  \"processes\": [\n");

    for (int i = 0; i < count; i++) {
        const process_info_t *proc = &processes[i];
        char uptime_buf[128];
        format_uptime(proc->start_time, uptime_buf, sizeof(uptime_buf));

        printf("    {\n");
        printf("      \"pid\": %d,\n", proc->pid);
        printf("      \"ppid\": %d,\n", proc->ppid);
        printf("      \"name\": \"%s\",\n", proc->name);
        printf("      \"user\": \"%s\",\n", proc->username);
        printf("      \"uid\": %d,\n", proc->uid);
        printf("      \"state\": \"%c\",\n", proc->state);
        printf("      \"state_name\": \"%s\",\n", get_state_name(proc->state));
        printf("      \"start_time\": %ld,\n", (long)proc->start_time);
        printf("      \"uptime\": \"%s\",\n", uptime_buf);
        printf("      \"cmdline\": \"%s\",\n", proc->cmdline);
        printf("      \"memory\": {\n");
        printf("        \"vsz_kb\": %lu,\n", proc->vsz);
        printf("        \"rss_kb\": %lu\n", proc->rss);
        printf("      }\n");
        printf("    }%s\n", i < count - 1 ? "," : "");
    }

    printf("  ]\n");
    printf("}\n");
}

/**
 * Output list of all processes with format selection
 *
 * Main entry point for displaying system-wide process list. Selects output
 * format based on command-line arguments. Returns error if no processes found.
 *
 * Format selection:
 * - JSON format if args->json_output is true
 * - Short (one-line) format if args->short_output is true
 * - Normal (table) format otherwise
 *
 * @param processes Array of process_info_t structures
 * @param count Number of processes in array
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success, -1 if no processes found
 */
int output_process_list(const process_info_t *processes, int count,
                        const cli_args_t *args) {
    if (count == 0) {
        print_error("No processes found");
        return -1;
    }

    if (args->json_output) {
        output_process_list_json(processes, count);
    } else if (args->short_output) {
        output_process_list_short(processes, count);
    } else {
        output_process_list_normal(processes, count);
    }

    return 0;
}
