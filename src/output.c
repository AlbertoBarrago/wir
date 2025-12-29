#include "output.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * PROCESS OUTPUT
 * ============================================================================ */

/**
 * Output process info in normal (pretty) format
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
    printf("%c\n", info->state);

    if (info->cmdline[0]) {
        print_color(COLOR_CYAN, "  Command: ");
        printf("%s\n", info->cmdline);
    }

    print_color(COLOR_CYAN, "  Memory: ");
    printf("VSZ=%lu KB, RSS=%lu KB\n", info->vsz, info->rss);
}

/**
 * Output process info in short (one-line) format
 */
static void output_process_short(const process_info_t *info) {
    printf("PID %d: %s[%d] by %s - %s\n",
           info->pid, info->name, info->ppid, info->username,
           info->cmdline[0] ? info->cmdline : "(no cmdline)");
}

/**
 * Output process info in JSON format
 */
static void output_process_json(const process_info_t *info) {
    printf("{\n");
    printf("  \"pid\": %d,\n", info->pid);
    printf("  \"name\": \"%s\",\n", info->name);
    printf("  \"ppid\": %d,\n", info->ppid);
    printf("  \"user\": \"%s\",\n", info->username);
    printf("  \"uid\": %d,\n", info->uid);
    printf("  \"state\": \"%c\",\n", info->state);
    printf("  \"cmdline\": \"%s\",\n", info->cmdline);
    printf("  \"memory\": {\n");
    printf("    \"vsz_kb\": %lu,\n", info->vsz);
    printf("    \"rss_kb\": %lu\n", info->rss);
    printf("  }\n");
    printf("}\n");
}

/**
 * Output process information
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
 * Recursively print process tree
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
 * Output process tree in JSON format
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
 * Output process tree
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
 * Output environment variables
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
 * Output port info in normal format
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
 * Output port info in short format
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
 * Output port information
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
 * Output process list in normal format
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
 */
static void output_process_list_short(const process_info_t *processes, int count) {
    for (int i = 0; i < count; i++) {
        const process_info_t *proc = &processes[i];
        printf("%d: %s by %s\n", proc->pid, proc->name, proc->username);
    }
}

/**
 * Output process list in JSON format
 */
static void output_process_list_json(const process_info_t *processes, int count) {
    printf("{\n");
    printf("  \"process_count\": %d,\n", count);
    printf("  \"processes\": [\n");

    for (int i = 0; i < count; i++) {
        const process_info_t *proc = &processes[i];

        printf("    {\n");
        printf("      \"pid\": %d,\n", proc->pid);
        printf("      \"ppid\": %d,\n", proc->ppid);
        printf("      \"name\": \"%s\",\n", proc->name);
        printf("      \"user\": \"%s\",\n", proc->username);
        printf("      \"uid\": %d,\n", proc->uid);
        printf("      \"state\": \"%c\",\n", proc->state);
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
 * Output list of all processes
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
