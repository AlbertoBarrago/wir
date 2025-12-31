#ifndef OUTPUT_H
#define OUTPUT_H

#include "platform.h"
#include "args.h"

/**
 * Output process information with format selection
 *
 * Main entry point for displaying process information. Selects output format
 * based on args flags (JSON, short, or normal). See src/output.c for detailed documentation.
 *
 * @param info Pointer to process_info_t structure containing process details
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success
 */
int output_process_info(const process_info_t *info, const cli_args_t *args);

/**
 * Output process tree with format selection
 *
 * Displays process ancestry tree from target process to root ancestor.
 * See src/output.c for detailed documentation.
 *
 * @param tree Pointer to process_tree_node_t representing the target process
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success, -1 if tree is NULL
 */
int output_process_tree(const process_tree_node_t *tree, const cli_args_t *args);

/**
 * Output environment variables for a process
 *
 * Displays all environment variables in normal or JSON format.
 * See src/output.c for detailed documentation.
 *
 * @param env_vars Array of environment variable strings (format: "NAME=value")
 * @param count Number of environment variables in array
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success
 */
int output_process_env(char **env_vars, int count, const cli_args_t *args);

/**
 * Output port information with format selection
 *
 * Displays port connection information in normal, short, JSON, or warnings-only format.
 * See src/output.c for detailed documentation.
 *
 * @param port Port number being queried
 * @param connections Array of connection_info_t structures
 * @param count Number of connections in array
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success, -1 if no connections found
 */
int output_port_info(int port, const connection_info_t *connections,
                     int count, const cli_args_t *args);

/**
 * Output list of all processes with format selection
 *
 * Displays system-wide process list in table, short, or JSON format.
 * See src/output.c for detailed documentation.
 *
 * @param processes Array of process_info_t structures
 * @param count Number of processes in array
 * @param args Pointer to cli_args_t structure containing output format flags
 * @return 0 on success, -1 if no processes found
 */
int output_process_list(const process_info_t *processes, int count,
                        const cli_args_t *args);

#endif /* OUTPUT_H */
