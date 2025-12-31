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
 *
 * Performs any necessary platform-specific initialization before the application
 * can query process and network information. Currently, both Linux and macOS
 * implementations require no initialization, but this function exists to provide
 * a hook for future platform-specific setup if needed.
 *
 * @return 0 on success (always succeeds in current implementation)
 */
int platform_init(void) {
    /* Currently no initialization needed for either platform */
    return 0;
}

/**
 * Clean up platform-specific resources
 *
 * Releases any platform-specific resources allocated during application execution.
 * Currently, both Linux and macOS implementations require no cleanup, but this
 * function exists to provide a hook for future platform-specific teardown if needed.
 * Should be called before application exit.
 *
 * @return void
 */
void platform_cleanup(void) {
    /* Currently no cleanup needed */
}

/**
 * Get username from UID using system password database
 *
 * Converts a numeric user ID to a human-readable username by querying the
 * system password database. Falls back to displaying the numeric UID if
 * username lookup fails.
 *
 * @param uid User ID to look up
 * @param username Buffer to store the username string
 * @param size Size of username buffer
 * @return void (username is always populated, either with name or numeric UID)
 */
static void get_username_from_uid(const int uid, char *username, size_t size) {
    const struct passwd *pw = getpwuid(uid);
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
 * Parse /proc/net/tcp or /proc/net/tcp6 to find connections on a specific port (Linux)
 *
 * Reads and parses Linux's /proc/net/tcp or /proc/net/tcp6 files to find all
 * network connections using a specific port. For each matching connection,
 * extracts network details and attempts to identify the owning process by
 * searching for socket inodes in /proc/*/fd/*.
 *
 * The function:
 * 1. Parses each line of the proc file (format: sl, local_address, rem_address, st, etc.)
 * 2. Filters connections matching the target port
 * 3. Converts hex addresses to dotted decimal notation
 * 4. Decodes TCP connection states (ESTABLISHED, LISTEN, etc.)
 * 5. Searches all /proc/<pid>/fd/* symlinks to find the process owning each socket
 *
 * @param filename Path to /proc/net file ("/proc/net/tcp" or "/proc/net/tcp6")
 * @param target_port Port number to search for
 * @param connections Output pointer to dynamically allocated array of connections
 * @param count Output pointer to number of connections found
 * @return 0 on success, -1 if file cannot be opened
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
 *
 * Retrieves all TCP and TCP6 network connections using the specified port by
 * parsing /proc/net/tcp and /proc/net/tcp6 files. Combines results from both
 * IPv4 and IPv6 connections into a single array.
 *
 * The function:
 * 1. Parses /proc/net/tcp for IPv4 connections
 * 2. Parses /proc/net/tcp6 for IPv6 connections
 * 3. Merges both results into a single output array
 * 4. Allocates memory for the connection array (caller must free)
 *
 * @param port Port number to query
 * @param connections Output pointer to dynamically allocated array of connections
 * @param count Output pointer to total number of connections found
 * @return 0 on success
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
 *
 * Retrieves comprehensive process information by reading multiple files in /proc/<pid>/.
 * Gathers details about process state, parent, user, memory usage, command line, and
 * uptime from various proc filesystem entries.
 *
 * Data sources:
 * - /proc/<pid>/stat: PID, name, state, PPID, start time (in ticks)
 * - /proc/stat: System boot time (btime) for calculating absolute start time
 * - /proc/<pid>/status: UID, virtual memory size (VmSize), resident memory (VmRSS)
 * - /proc/<pid>/cmdline: Full command line with arguments
 * - getpwuid(): Username lookup from UID
 *
 * The function handles:
 * - Converting tick-based start time to Unix timestamp
 * - Replacing null bytes in cmdline with spaces
 * - Trimming trailing whitespace
 * - Graceful handling of missing or inaccessible files
 *
 * @param pid Process ID to query
 * @param info Pointer to process_info_t structure to populate
 * @return 0 on success, -1 if process doesn't exist or /proc/<pid>/stat cannot be read
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
 *
 * Reads and parses the /proc/<pid>/environ file to extract all environment
 * variables for a specific process. The environ file contains null-separated
 * strings in "NAME=value" format.
 *
 * The function:
 * 1. Reads entire /proc/<pid>/environ file into memory
 * 2. Counts environment variables by counting null byte separators
 * 3. Allocates array for storing individual variable strings
 * 4. Splits buffer at null bytes and duplicates each variable string
 * 5. Returns dynamically allocated array (caller must free using platform_free_env_vars)
 *
 * @param pid Process ID to query
 * @param env_vars Output pointer to dynamically allocated array of environment variable strings
 * @param count Output pointer to number of environment variables found
 * @return 0 on success, -1 if /proc/<pid>/environ cannot be opened (permission denied or process doesn't exist)
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
    for (size_t i = 0; i <= n; i++) {
        if (buffer[i] == '\0' && i > 0 && buffer[i - 1] != '\0') {
            (*count)++;
        }
    }

    /* Allocate array */
    *env_vars = safe_malloc(*count * sizeof(char *));

    /* Split into individual strings */
    int idx = 0;
    char *start = buffer;
    for (size_t i = 0; i <= n; i++) {
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
 *
 * Retrieves network connections on a specific port by executing lsof (list open files)
 * command and parsing its output. Uses lsof with the -F flag for parseable output format.
 *
 * The function:
 * 1. Executes "lsof -i :<port> -F pcn" to get processes using the port
 * 2. Parses lsof output in field format:
 *    - 'p' lines: Process ID
 *    - 'c' lines: Command name (not used)
 *    - 'n' lines: Network address
 * 3. Builds connection_info_t structures from parsed data
 * 4. Returns dynamically allocated array (caller must free)
 *
 * Note: This is a simplified implementation using lsof as a fallback since
 * direct sysctl access for network connections on macOS is complex.
 *
 * @param port Port number to query
 * @param connections Output pointer to dynamically allocated array of connections
 * @param count Output pointer to number of connections found
 * @return 0 on success, -1 if popen fails
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

    /* Add the last entry */
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
 *
 * Retrieves comprehensive process information using macOS-specific proc_pidinfo()
 * system calls. Gathers details about process state, parent, user, memory usage,
 * path, and uptime from kernel structures.
 *
 * Data sources:
 * - proc_pidinfo(PROC_PIDTBSDINFO): PPID, UID, status, process name, start time
 * - proc_pidpath(): Full path to process executable
 * - proc_pidinfo(PROC_PIDTASKINFO): Memory usage (virtual and resident size)
 * - getpwuid(): Username lookup from UID
 *
 * State mapping:
 * - BSD SIDL (1) -> 'I' (Idle/being created)
 * - BSD SRUN (2) -> 'R' (Running)
 * - BSD SSLEEP (3) -> 'S' (Sleeping)
 * - BSD SSTOP (4) -> 'T' (Stopped)
 * - BSD SZOMB (5) -> 'Z' (Zombie)
 *
 * @param pid Process ID to query
 * @param info Pointer to process_info_t structure to populate
 * @return 0 on success, -1 if proc_pidinfo fails (process doesn't exist or no permission)
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

    /* Get the process start time */
    info->start_time = bsd_info.pbi_start_tvsec;

    /* Get the username */
    get_username_from_uid(info->uid, info->username, sizeof(info->username));

    /* Get the command line using proc_pidpath and PROC_PIDPATHINFO */
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
 *
 * Retrieves environment variables using the sysctl API with KERN_PROCARGS2.
 *
 * The function:
 * 1. Uses sysctl(KERN_PROCARGS2) to get process args and environment data
 * 2. Parses the returned buffer which contains: argc, exec_path, args, env
 * 3. All strings are null-terminated and packed sequentially in the buffer
 * 4. Skips over argc, executable path, and arguments to reach environment vars
 * 5. Returns dynamically allocated array (caller must free using platform_free_env_vars)
 *
 * Buffer structure from KERN_PROCARGS2:
 * - int argc (4 bytes)
 * - executable path (null-terminated string)
 * - null padding (to align arguments)
 * - arguments (null-separated strings, argc count)
 * - environment variables (null-separated strings in "NAME=value" format)
 *
 * Limitations:
 * - Requires same user or root privileges to access other processes
 * - May fail for system processes or processes with restricted access
 * - Returns error if buffer size is insufficient (very large environments)
 *
 * @param pid Process ID to query
 * @param env_vars Output pointer to a dynamically allocated array of environment variable strings
 * @param count Output pointer to the number of environment variables found
 * @return 0 on success, -1 if sysctl fails (permission denied, the process doesn't exist, or buffer too small)
 */
int platform_get_process_env(const pid_t pid, char ***env_vars, int *count) {
    int mib[3];
    size_t size;
    int argc;

    /* Set up sysctl MIB for KERN_PROCARGS2 */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;

    /* Get the size needed for the buffer */
    if (sysctl(mib, 3, NULL, &size, NULL, 0) < 0) {
        return -1;
    }

    /* Allocate buffer */
    char* buffer = safe_malloc(size);

    /* Get the actual data */
    if (sysctl(mib, 3, buffer, &size, NULL, 0) < 0) {
        free(buffer);
        return -1;
    }

    /* Parse the buffer:
     * First 4 bytes are argc (number of arguments)
     * Then comes the executable path (null-terminated)
     * Then null padding
     * Then the arguments (null-separated, argc times)
     * Then environment variables (null-separated until the end of buffer)
     */

    /* Read argc */
    memcpy(&argc, buffer, sizeof(argc));
    char *ptr = buffer + sizeof(argc);

    /* Skip executable path (null-terminated string) */
    ptr += strlen(ptr) + 1;

    /* Skip any null padding bytes after an executable path */
    while (ptr < buffer + size && *ptr == '\0') {
        ptr++;
    }

    /* Skip all arguments (argc null-terminated strings) */
    for (int i = 0; i < argc && ptr < buffer + size; i++) {
        ptr += strlen(ptr) + 1;
    }

    /* Now ptr points to the start of environment variables */
    /* Count environment variables first */
    *count = 0;
    char *temp_ptr = ptr;
    while (temp_ptr < buffer + size) {
        if (*temp_ptr == '\0') {
            /* End of environment */
            break;
        }
        /* Check if this looks like an environment variable (contains '=') */
        if (strchr(temp_ptr, '=')) {
            (*count)++;
        }
        temp_ptr += strlen(temp_ptr) + 1;
    }

    /* Allocate array for environment variable pointers */
    *env_vars = safe_malloc(*count * sizeof(char *));

    /* Copy environment variables */
    int idx = 0;
    while (ptr < buffer + size && idx < *count) {
        if (*ptr == '\0') {
            /* End of environment */
            break;
        }
        /* Only copy strings that contain '=' (valid env vars) */
        if (strchr(ptr, '=')) {
            (*env_vars)[idx++] = safe_strdup(ptr);
        }
        ptr += strlen(ptr) + 1;
    }

    /* Update count to actual number copied */
    *count = idx;

    free(buffer);
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
int platform_get_process_tree(const pid_t pid, process_tree_node_t **tree) {
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
