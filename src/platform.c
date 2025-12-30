#include "platform.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>

/* Platform-specific includes */
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <libproc.h>
#include <sys/proc_info.h>
#elif __linux__
#include <dirent.h>
#include <sys/stat.h>
#endif

/**
 * Initialize platform-specific resources
 */
int platform_init(void) {
    /* Currently no initialization needed for either platform */
    return 0;
}

/**
 * Clean up platform-specific resources
 */
void platform_cleanup(void) {
    /* Currently no cleanup needed */
}

/**
 * Get username from UID
 */
static void get_username_from_uid(int uid, char *username, size_t size) {
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        snprintf(username, size, "%s", pw->pw_name);
    } else {
        snprintf(username, size, "%d", uid);
    }
}

/* ============================================================================
 * LINUX IMPLEMENTATION
 * ============================================================================ */
#ifdef __linux__

/**
 * Parse /proc/net/tcp or /proc/net/tcp6 to find connections
 */
static int parse_proc_net(const char *filename, int target_port,
                         connection_info_t **connections, int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return -1;
    }

    *connections = NULL;
    *count = 0;
    int capacity = 10;
    *connections = safe_malloc(capacity * sizeof(connection_info_t));

    char line[512];
    /* Skip header line */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        unsigned long local_addr, remote_addr;
        int local_port, remote_port, state, inode;
        int uid;

        /* Parse the line - format varies but generally:
         * sl local_address rem_address st tx_queue rx_queue tr tm->when retrnsmt uid timeout inode
         */
        int matched = sscanf(line, "%*d: %lx:%x %lx:%x %x %*x:%*x %*x:%*x %*x %d %*d %d",
                           &local_addr, &local_port, &remote_addr, &remote_port,
                           &state, &uid, &inode);

        if (matched < 7) {
            continue;
        }

        /* Check if this matches our target port */
        if (local_port != target_port) {
            continue;
        }

        /* Expand array if needed */
        if (*count >= capacity) {
            capacity *= 2;
            *connections = safe_realloc(*connections, capacity * sizeof(connection_info_t));
        }

        connection_info_t *conn = &(*connections)[*count];
        memset(conn, 0, sizeof(*conn));

        /* Convert addresses from hex to dotted decimal (IPv4) */
        snprintf(conn->local_addr, sizeof(conn->local_addr),
                "%lu.%lu.%lu.%lu",
                (local_addr) & 0xFF,
                (local_addr >> 8) & 0xFF,
                (local_addr >> 16) & 0xFF,
                (local_addr >> 24) & 0xFF);

        snprintf(conn->remote_addr, sizeof(conn->remote_addr),
                "%lu.%lu.%lu.%lu",
                (remote_addr) & 0xFF,
                (remote_addr >> 8) & 0xFF,
                (remote_addr >> 16) & 0xFF,
                (remote_addr >> 24) & 0xFF);

        conn->local_port = local_port;
        conn->remote_port = remote_port;

        /* Decode connection state */
        switch (state) {
            case 0x01: strcpy(conn->state, "ESTABLISHED"); break;
            case 0x02: strcpy(conn->state, "SYN_SENT"); break;
            case 0x03: strcpy(conn->state, "SYN_RECV"); break;
            case 0x04: strcpy(conn->state, "FIN_WAIT1"); break;
            case 0x05: strcpy(conn->state, "FIN_WAIT2"); break;
            case 0x06: strcpy(conn->state, "TIME_WAIT"); break;
            case 0x07: strcpy(conn->state, "CLOSE"); break;
            case 0x08: strcpy(conn->state, "CLOSE_WAIT"); break;
            case 0x09: strcpy(conn->state, "LAST_ACK"); break;
            case 0x0A: strcpy(conn->state, "LISTEN"); break;
            case 0x0B: strcpy(conn->state, "CLOSING"); break;
            default: strcpy(conn->state, "UNKNOWN"); break;
        }

        strcpy(conn->protocol, strstr(filename, "tcp6") ? "TCP6" : "TCP");

        /* Find PID by searching for inode in /proc/*/fd/* */
        conn->pid = -1;
        DIR *proc_dir = opendir("/proc");
        if (proc_dir) {
            struct dirent *proc_entry;
            while ((proc_entry = readdir(proc_dir)) != NULL) {
                /* Skip non-numeric directories */
                if (proc_entry->d_name[0] < '0' || proc_entry->d_name[0] > '9') {
                    continue;
                }

                char fd_path[256];
                snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd", proc_entry->d_name);

                DIR *fd_dir = opendir(fd_path);
                if (!fd_dir) {
                    continue;
                }

                struct dirent *fd_entry;
                while ((fd_entry = readdir(fd_dir)) != NULL) {
                    char link_path[512];
                    char link_target[512];
                    snprintf(link_path, sizeof(link_path), "%s/%s", fd_path, fd_entry->d_name);

                    ssize_t len = readlink(link_path, link_target, sizeof(link_target) - 1);
                    if (len > 0) {
                        link_target[len] = '\0';
                        char expected[64];
                        snprintf(expected, sizeof(expected), "socket:[%d]", inode);
                        if (strcmp(link_target, expected) == 0) {
                            conn->pid = atoi(proc_entry->d_name);
                            closedir(fd_dir);
                            goto found_pid;
                        }
                    }
                }
                closedir(fd_dir);
            }
found_pid:
            closedir(proc_dir);
        }

        (*count)++;
    }

    fclose(fp);
    return 0;
}

/**
 * Get all connections on a specific port (Linux)
 */
int platform_get_port_connections(int port, connection_info_t **connections, int *count) {
    int total = 0;
    connection_info_t *all_conns = NULL;

    /* Parse TCP connections */
    connection_info_t *tcp_conns = NULL;
    int tcp_count = 0;
    if (parse_proc_net("/proc/net/tcp", port, &tcp_conns, &tcp_count) == 0) {
        all_conns = safe_realloc(all_conns, (total + tcp_count) * sizeof(connection_info_t));
        memcpy(all_conns + total, tcp_conns, tcp_count * sizeof(connection_info_t));
        total += tcp_count;
        free(tcp_conns);
    }

    /* Parse TCP6 connections */
    connection_info_t *tcp6_conns = NULL;
    int tcp6_count = 0;
    if (parse_proc_net("/proc/net/tcp6", port, &tcp6_conns, &tcp6_count) == 0) {
        all_conns = safe_realloc(all_conns, (total + tcp6_count) * sizeof(connection_info_t));
        memcpy(all_conns + total, tcp6_conns, tcp6_count * sizeof(connection_info_t));
        total += tcp6_count;
        free(tcp6_conns);
    }

    *connections = all_conns;
    *count = total;

    return 0;
}

/**
 * Get information about a process (Linux)
 */
int platform_get_process_info(pid_t pid, process_info_t *info) {
    memset(info, 0, sizeof(*info));
    info->pid = pid;

    /* Read /proc/[pid]/stat */
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    FILE *fp = fopen(stat_path, "r");
    if (!fp) {
        return -1;
    }

    /* Parse stat file - we need to read more fields to get starttime (field 22) */
    char name[256];
    unsigned long long starttime_ticks;
    if (fscanf(fp, "%*d (%255[^)]) %c %d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu "
                   "%*lu %*lu %*ld %*ld %*ld %*ld %*ld %*ld %llu",
               name, &info->state, &info->ppid, &starttime_ticks) != 4) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    snprintf(info->name, sizeof(info->name), "%s", name);

    /* Calculate process start time */
    /* Get system boot time from /proc/stat */
    time_t boot_time = 0;
    fp = fopen("/proc/stat", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "btime ", 6) == 0) {
                sscanf(line + 6, "%ld", &boot_time);
                break;
            }
        }
        fclose(fp);
    }

    /* Convert ticks to seconds and add to boot time */
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    if (ticks_per_sec > 0 && boot_time > 0) {
        info->start_time = boot_time + (starttime_ticks / ticks_per_sec);
    } else {
        info->start_time = 0;
    }

    /* Read /proc/[pid]/status for UID and memory info */
    char status_path[64];
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);

    fp = fopen(status_path, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Uid:", 4) == 0) {
                sscanf(line + 4, "%d", &info->uid);
            } else if (strncmp(line, "VmSize:", 7) == 0) {
                sscanf(line + 7, "%lu", &info->vsz);
            } else if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%lu", &info->rss);
            }
        }
        fclose(fp);
    }

    /* Get username */
    get_username_from_uid(info->uid, info->username, sizeof(info->username));

    /* Read /proc/[pid]/cmdline */
    char cmdline_path[64];
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);

    fp = fopen(cmdline_path, "r");
    if (fp) {
        size_t n = fread(info->cmdline, 1, sizeof(info->cmdline) - 1, fp);
        info->cmdline[n] = '\0';

        /* Replace null bytes with spaces */
        for (size_t i = 0; i < n; i++) {
            if (info->cmdline[i] == '\0') {
                info->cmdline[i] = ' ';
            }
        }

        /* Trim trailing spaces */
        while (n > 0 && info->cmdline[n - 1] == ' ') {
            info->cmdline[--n] = '\0';
        }

        fclose(fp);
    }

    return 0;
}

/**
 * Get environment variables for a process (Linux)
 */
int platform_get_process_env(pid_t pid, char ***env_vars, int *count) {
    char env_path[64];
    snprintf(env_path, sizeof(env_path), "/proc/%d/environ", pid);

    FILE *fp = fopen(env_path, "r");
    if (!fp) {
        return -1;
    }

    /* Read entire file */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = safe_malloc(size + 1);
    size_t n = fread(buffer, 1, size, fp);
    buffer[n] = '\0';
    fclose(fp);

    /* Count environment variables (separated by null bytes) */
    *count = 0;
    for (size_t i = 0; i < n; i++) {
        if (buffer[i] == '\0' && i > 0 && buffer[i - 1] != '\0') {
            (*count)++;
        }
    }

    /* Allocate array */
    *env_vars = safe_malloc(*count * sizeof(char *));

    /* Split into individual strings */
    int idx = 0;
    char *start = buffer;
    for (size_t i = 0; i < n; i++) {
        if (buffer[i] == '\0' && start < buffer + n) {
            if (*start) {
                (*env_vars)[idx++] = safe_strdup(start);
            }
            start = buffer + i + 1;
        }
    }

    free(buffer);
    return 0;
}

/* ============================================================================
 * MACOS IMPLEMENTATION
 * ============================================================================ */
#elif __APPLE__

/**
 * Get all connections on a specific port (macOS)
 */
int platform_get_port_connections(int port, connection_info_t **connections, int *count) {
    /* On macOS, we use lsof as a fallback since direct sysctl for network is complex */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lsof -i :%d -F pcn 2>/dev/null", port);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    *connections = NULL;
    *count = 0;
    int capacity = 10;
    *connections = safe_malloc(capacity * sizeof(connection_info_t));

    connection_info_t current;
    memset(&current, 0, sizeof(current));
    bool has_data = false;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        line[strcspn(line, "\n")] = '\0';

        if (line[0] == 'p') {
            /* PID */
            if (has_data && *count < capacity) {
                memcpy(&(*connections)[(*count)++], &current, sizeof(current));
                if (*count >= capacity) {
                    capacity *= 2;
                    *connections = safe_realloc(*connections, capacity * sizeof(connection_info_t));
                }
            }
            current.pid = atoi(line + 1);
            has_data = true;
            current.local_port = port;
            strcpy(current.protocol, "TCP");
            strcpy(current.state, "LISTEN");
        } else if (line[0] == 'c') {
            /* Command name - not used in connection_info_t */
        } else if (line[0] == 'n') {
            /* Network address */
            strncpy(current.local_addr, line + 1, sizeof(current.local_addr) - 1);
        }
    }

    /* Add last entry */
    if (has_data) {
        if (*count >= capacity) {
            capacity++;
            *connections = safe_realloc(*connections, capacity * sizeof(connection_info_t));
        }
        memcpy(&(*connections)[(*count)++], &current, sizeof(current));
    }

    pclose(fp);
    return 0;
}

/**
 * Get information about a process (macOS)
 */
int platform_get_process_info(pid_t pid, process_info_t *info) {
    memset(info, 0, sizeof(*info));
    info->pid = pid;

    /* Get process info using proc_pidinfo */
    struct proc_bsdinfo bsd_info;
    if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsd_info, sizeof(bsd_info)) <= 0) {
        return -1;
    }

    info->ppid = bsd_info.pbi_ppid;
    info->uid = bsd_info.pbi_uid;

    /* Convert macOS status codes to process state characters */
    /* BSD status codes: SIDL=1, SRUN=2, SSLEEP=3, SSTOP=4, SZOMB=5 */
    switch (bsd_info.pbi_status) {
        case 1:  /* SIDL */
            info->state = 'I';  /* Idle/being created */
            break;
        case 2:  /* SRUN */
            info->state = 'R';  /* Running */
            break;
        case 3:  /* SSLEEP */
            info->state = 'S';  /* Sleeping */
            break;
        case 4:  /* SSTOP */
            info->state = 'T';  /* Stopped */
            break;
        case 5:  /* SZOMB */
            info->state = 'Z';  /* Zombie */
            break;
        default:
            info->state = '?';  /* Unknown */
            break;
    }

    snprintf(info->name, sizeof(info->name), "%s", bsd_info.pbi_name);

    /* Get process start time */
    info->start_time = bsd_info.pbi_start_tvsec;

    /* Get username */
    get_username_from_uid(info->uid, info->username, sizeof(info->username));

    /* Get command line using proc_pidpath and PROC_PIDPATHINFO */
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) > 0) {
        snprintf(info->cmdline, sizeof(info->cmdline), "%s", pathbuf);
    }

    /* Get task info for memory */
    struct proc_taskinfo task_info;
    if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info)) > 0) {
        info->vsz = task_info.pti_virtual_size / 1024;
        info->rss = task_info.pti_resident_size / 1024;
    }

    return 0;
}

/**
 * Get environment variables for a process (macOS)
 */
int platform_get_process_env(pid_t pid, char ***env_vars, int *count) {
    /* On macOS, getting environment variables of other processes is restricted
     * We can try using ps, but it may not work for all processes */

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ps eww -p %d 2>/dev/null | tail -1", pid);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    char line[4096];
    if (!fgets(line, sizeof(line), fp)) {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    /* Parse environment variables from ps output */
    /* This is a simplified version - a real implementation would be more robust */
    *count = 0;
    int capacity = 10;
    *env_vars = safe_malloc(capacity * sizeof(char *));

    /* Skip the initial columns (PID, TTY, STAT, etc.) and get to the environment */
    char *env_start = strstr(line, "=");
    if (!env_start) {
        return 0;
    }

    /* Backtrack to find the start of the variable name */
    while (env_start > line && *(env_start - 1) != ' ') {
        env_start--;
    }

    /* Parse environment variables (space-separated) */
    char *token = env_start;
    while (token && *token) {
        char *next = strchr(token, ' ');
        if (next) {
            *next = '\0';
            next++;
        }

        if (strchr(token, '=')) {
            if (*count >= capacity) {
                capacity *= 2;
                *env_vars = safe_realloc(*env_vars, capacity * sizeof(char *));
            }
            (*env_vars)[(*count)++] = safe_strdup(token);
        }

        token = next;
    }

    return 0;
}

#else
#error "Unsupported platform"
#endif

/* ============================================================================
 * COMMON (PLATFORM-INDEPENDENT) FUNCTIONS
 * ============================================================================ */

/**
 * Build process ancestry tree
 */
int platform_get_process_tree(pid_t pid, process_tree_node_t **tree) {
    process_tree_node_t *node = safe_calloc(1, sizeof(process_tree_node_t));

    if (platform_get_process_info(pid, &node->info) < 0) {
        free(node);
        return -1;
    }

    /* Recursively build parent */
    if (node->info.ppid > 0 && node->info.ppid != pid) {
        process_tree_node_t *parent;
        if (platform_get_process_tree(node->info.ppid, &parent) == 0) {
            node->parent = parent;
        }
    }

    *tree = node;
    return 0;
}

/**
 * Free process tree
 */
void platform_free_process_tree(process_tree_node_t *tree) {
    if (!tree) {
        return;
    }

    if (tree->parent) {
        platform_free_process_tree(tree->parent);
    }

    if (tree->children) {
        for (int i = 0; i < tree->num_children; i++) {
            platform_free_process_tree(tree->children[i]);
        }
        free(tree->children);
    }

    free(tree);
}

/**
 * Free environment variables array
 */
void platform_free_env_vars(char **env_vars, int count) {
    if (!env_vars) {
        return;
    }

    for (int i = 0; i < count; i++) {
        free(env_vars[i]);
    }
    free(env_vars);
}

/**
 * Get list of all running processes
 */
int platform_get_all_processes(process_info_t **processes, int *count) {
    *processes = NULL;
    *count = 0;

#ifdef __linux__
    /* On Linux, scan /proc for numeric directories */
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        return -1;
    }

    int capacity = 100;
    *processes = safe_malloc(capacity * sizeof(process_info_t));

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        /* Check if directory name is numeric (a PID) */
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') {
            continue;
        }

        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) {
            continue;
        }

        /* Expand array if needed */
        if (*count >= capacity) {
            capacity *= 2;
            *processes = safe_realloc(*processes, capacity * sizeof(process_info_t));
        }

        /* Get process info - skip if it fails (process may have exited) */
        if (platform_get_process_info(pid, &(*processes)[*count]) == 0) {
            (*count)++;
        }
    }

    closedir(proc_dir);

#elif __APPLE__
    /* On macOS, use sysctl to get all processes */
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;

    /* Get size needed */
    if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0) {
        return -1;
    }

    /* Allocate buffer */
    struct kinfo_proc *proc_list = safe_malloc(size);

    /* Get process list */
    if (sysctl(mib, 4, proc_list, &size, NULL, 0) < 0) {
        free(proc_list);
        return -1;
    }

    /* Calculate number of processes */
    int num_procs = size / sizeof(struct kinfo_proc);

    /* Allocate output array */
    *processes = safe_malloc(num_procs * sizeof(process_info_t));

    /* Convert each process */
    for (int i = 0; i < num_procs; i++) {
        pid_t pid = proc_list[i].kp_proc.p_pid;

        /* Get detailed info - skip if it fails */
        if (platform_get_process_info(pid, &(*processes)[*count]) == 0) {
            (*count)++;
        }
    }

    free(proc_list);
#endif

    return 0;
}
