#ifndef PLATFORM_H
#define PLATFORM_H

#include <sys/types.h>
#include <stdbool.h>
#include <time.h>

/* Maximum lengths for various fields */
#define MAX_PROCESS_NAME 256
#define MAX_CMDLINE 1024
#define MAX_USERNAME 64
#define MAX_PATH 1024

/**
 * Network connection/socket information structure
 *
 * Contains details about a network connection including addresses, ports,
 * connection state, and the process using it. Populated by platform-specific
 * implementations when querying port usage.
 *
 * Fields:
 * - local_addr: Local IP address (dotted decimal for IPv4)
 * - local_port: Local port number
 * - remote_addr: Remote IP address (dotted decimal for IPv4)
 * - remote_port: Remote port number (0 if not applicable)
 * - state: Connection state (LISTEN, ESTABLISHED, etc.)
 * - pid: Process ID using this connection
 * - protocol: Protocol name (TCP, TCP6, UDP)
 */
typedef struct {
    char local_addr[64];    /* Local IP address */
    int local_port;         /* Local port number */
    char remote_addr[64];   /* Remote IP address */
    int remote_port;        /* Remote port number */
    char state[16];         /* Connection state (LISTEN, ESTABLISHED, etc.) */
    pid_t pid;              /* Process ID using this connection */
    char protocol[8];       /* Protocol (TCP, UDP) */
} connection_info_t;

/**
 * Process information structure
 *
 * Contains comprehensive information about a running process. Populated by
 * platform_get_process_info() using platform-specific system calls.
 *
 * Fields:
 * - pid: Process ID
 * - ppid: Parent process ID
 * - name: Process name (executable name)
 * - cmdline: Full command line with arguments
 * - username: Username of process owner
 * - state: Process state character (R=Running, S=Sleeping, Z=Zombie, etc.)
 * - vsz: Virtual memory size in kilobytes
 * - rss: Resident set size (physical memory) in kilobytes
 * - uid: User ID of process owner
 * - start_time: Process start time as Unix timestamp (seconds since epoch)
 */
typedef struct {
    pid_t pid;              /* Process ID */
    pid_t ppid;             /* Parent process ID */
    char name[MAX_PROCESS_NAME];        /* Process name */
    char cmdline[MAX_CMDLINE];          /* Full command line */
    char username[MAX_USERNAME];        /* User running the process */
    char state;             /* Process state (R, S, Z, etc.) */
    unsigned long vsz;      /* Virtual memory size (KB) */
    unsigned long rss;      /* Resident set size (KB) */
    int uid;                /* User ID */
    time_t start_time;      /* Process start time (seconds since epoch) */
} process_info_t;

/**
 * Process tree node structure for ancestry visualization
 *
 * Represents a node in a process ancestry tree. Used by --tree option to
 * display process lineage from target process up to root ancestor.
 *
 * Fields:
 * - info: Full process information for this node
 * - parent: Pointer to parent process node (NULL for root/init)
 * - children: Array of pointers to child process nodes
 * - num_children: Number of child nodes in children array
 *
 * Note: Current implementation builds parent chain (tree goes upward),
 * so children array may not be populated in all use cases.
 */
typedef struct process_tree_node {
    process_info_t info;
    struct process_tree_node *parent;
    struct process_tree_node **children;
    int num_children;
} process_tree_node_t;

/**
 * Get all connections on a specific port
 *
 * Platform-specific implementation. See src/platform.c for detailed documentation.
 *
 * @param port Port number to query
 * @param connections Output pointer to dynamically allocated array (caller must free)
 * @param count Output pointer to number of connections found
 * @return 0 on success, -1 on error
 */
int platform_get_port_connections(int port, connection_info_t **connections, int *count);

/**
 * Get information about a specific process
 *
 * Platform-specific implementation. See src/platform.c for detailed documentation.
 *
 * @param pid Process ID to query
 * @param info Pointer to process_info_t structure to populate
 * @return 0 on success, -1 on error (process doesn't exist or access denied)
 */
int platform_get_process_info(pid_t pid, process_info_t *info);

/**
 * Build process ancestry tree recursively
 *
 * Platform-independent implementation. See src/platform.c for detailed documentation.
 *
 * @param pid Process ID to build tree from (becomes the leaf/target node)
 * @param tree Output pointer to dynamically allocated tree (caller must free with platform_free_process_tree)
 * @return 0 on success, -1 on error
 */
int platform_get_process_tree(pid_t pid, process_tree_node_t **tree);

/**
 * Free process tree structure recursively
 *
 * Platform-independent implementation. See src/platform.c for detailed documentation.
 *
 * @param tree Pointer to process tree node to free (NULL-safe)
 * @return void
 */
void platform_free_process_tree(process_tree_node_t *tree);

/**
 * Get environment variables for a process
 *
 * Platform-specific implementation. See src/platform.c for detailed documentation.
 *
 * @param pid Process ID to query
 * @param env_vars Output pointer to dynamically allocated array (caller must free with platform_free_env_vars)
 * @param count Output pointer to number of environment variables found
 * @return 0 on success, -1 on error (permission denied or process doesn't exist)
 */
int platform_get_process_env(pid_t pid, char ***env_vars, int *count);

/**
 * Free environment variables array
 *
 * Platform-independent implementation. See src/platform.c for detailed documentation.
 *
 * @param env_vars Pointer to array of environment variable strings (NULL-safe)
 * @param count Number of environment variables in array
 * @return void
 */
void platform_free_env_vars(char **env_vars, int count);

/**
 * Get list of all running processes
 *
 * Platform-specific implementation. See src/platform.c for detailed documentation.
 *
 * @param processes Output pointer to dynamically allocated array (caller must free)
 * @param count Output pointer to number of processes found
 * @return 0 on success, -1 on error
 */
int platform_get_all_processes(process_info_t **processes, int *count);

/**
 * Initialize platform-specific resources
 *
 * Platform-specific implementation. See src/platform.c for detailed documentation.
 *
 * @return 0 on success (always succeeds in current implementation)
 */
int platform_init(void);

/**
 * Clean up platform-specific resources
 *
 * Platform-specific implementation. See src/platform.c for detailed documentation.
 *
 * @return void
 */
void platform_cleanup(void);

#endif /* PLATFORM_H */
