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
 * Connection/socket information
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
 * Process information
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
 * Process tree node (for --tree output)
 */
typedef struct process_tree_node {
    process_info_t info;
    struct process_tree_node *parent;
    struct process_tree_node **children;
    int num_children;
} process_tree_node_t;

/**
 * Get all connections on a specific port
 * @param port Port number to query
 * @param connections Output array of connections (caller must free)
 * @param count Number of connections found
 * @return 0 on success, -1 on error
 */
int platform_get_port_connections(int port, connection_info_t **connections, int *count);

/**
 * Get information about a specific process
 * @param pid Process ID
 * @param info Output structure for process information
 * @return 0 on success, -1 on error
 */
int platform_get_process_info(pid_t pid, process_info_t *info);

/**
 * Build process ancestry tree
 * @param pid Root process ID
 * @param tree Output tree structure (caller must free with free_process_tree)
 * @return 0 on success, -1 on error
 */
int platform_get_process_tree(pid_t pid, process_tree_node_t **tree);

/**
 * Free process tree structure
 * @param tree Tree to free
 */
void platform_free_process_tree(process_tree_node_t *tree);

/**
 * Get environment variables for a process
 * @param pid Process ID
 * @param env_vars Output array of environment variable strings (caller must free)
 * @param count Number of environment variables
 * @return 0 on success, -1 on error
 */
int platform_get_process_env(pid_t pid, char ***env_vars, int *count);

/**
 * Free environment variables array
 * @param env_vars Array to free
 * @param count Number of variables
 */
void platform_free_env_vars(char **env_vars, int count);

/**
 * Get list of all running processes
 * @param processes Output array of process information (caller must free)
 * @param count Number of processes found
 * @return 0 on success, -1 on error
 */
int platform_get_all_processes(process_info_t **processes, int *count);

/**
 * Initialize platform-specific resources
 * @return 0 on success, -1 on error
 */
int platform_init(void);

/**
 * Clean up platform-specific resources
 */
void platform_cleanup(void);

#endif /* PLATFORM_H */
