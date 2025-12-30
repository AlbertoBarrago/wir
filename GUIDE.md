# Wir User Guide

A comprehensive guide to using `wir` - the "What Is Running" CLI tool.


## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Basic Concepts](#basic-concepts)
4. [Command Reference](#command-reference)
5. [Practical Examples](#practical-examples)
6. [Understanding Output](#understanding-output)
7. [Tips & Tricks](#tips--tricks)
8. [Common Use Cases](#common-use-cases)
9. [Troubleshooting](#troubleshooting)

---

## Introduction

`wir` (What Is Running) is a tool that helps you understand:
- Which process is using a specific network port
- Detailed information about any running process
- The ancestry (parent chain) of a process
- Environment variables of a process
- Interactively kill processes with a simple keypress

Think of it as a combination of `lsof`, `ps`, and `netstat` with a user-friendly interface.

---

## Getting Started

### Installation

#### Via Homebrew (macOS - Recommended)

The easiest way to install `wir` on macOS:

```bash
brew tap AlbertoBarrago/tap
brew install wir
```

That's it! Now you can use `wir` from anywhere:

```bash
wir --help
```

#### Building from Source

If you prefer to build from source or are on Linux:

```bash
# From the Wir directory
make
sudo make install
```

Now you can use `wir` from anywhere:

```bash
wir --help
```

### First Commands

Try these to get familiar:

```bash
# See what's running on a specific port
wir --port 8080

# Get info about your current shell
wir --pid $$

# List all running processes
wir --all --short

# Show the help message
wir --help
```

---

## Basic Concepts

### Three Main Modes

`wir` operates in three primary modes:

1. **Port Mode** (`--port`): Find out what's using a network port
2. **PID Mode** (`--pid`): Get information about a specific process
3. **All Mode** (`--all`): List all running processes on the system

You must choose one of these modes (they're mutually exclusive).

### Output Formats

For each mode, you can choose different output formats:

- **Normal**: Pretty, colorized output (default)
- **Short** (`--short`): One-line summary
- **JSON** (`--json`): Machine-readable format
- **Tree** (`--tree`): Show process hierarchy (PID mode only)
- **Env** (`--env`): Show environment variables (PID mode only)
- **Warnings** (`--warnings`): Security warnings (port mode only)

### Interactive Mode

Interactive mode (`--interactive` or `-i`) allows you to kill processes directly after viewing their information. Available for port and PID modes.

When enabled, after displaying process information, you'll be prompted:
- Press **'k'** to kill the process (sends SIGTERM)
- Press **'q'** to quit without killing
- Press any other key to exit interactive mode

---

## Command Reference

### Port Mode

#### Basic Port Inspection

```bash
wir --port <port_number>
```

**What it does**: Shows all processes listening on or connected to the specified port.

**Example**:
```bash
wir --port 8080
```

**Output includes**:
- Protocol (TCP/TCP6)
- Connection state (LISTEN, ESTABLISHED, etc.)
- Local and remote addresses
- Process name and PID
- User running the process
- Full command line

#### Port with Short Output

```bash
wir --port <port> --short
```

**Example**:
```bash
wir --port 3000 --short
# Output: Port 3000: node[12345] by albz (LISTEN)
```

**Use when**: You just need a quick answer about what's on a port.

#### Port with JSON Output

```bash
wir --port <port> --json
```

**Example**:
```bash
wir --port 5432 --json
```

**Use when**: You want to parse the output in a script or another program.

#### Port with Warnings Only

```bash
wir --port <port> --warnings
```

**What it shows**:
- Processes running as root on non-system ports (1024+)
- Zombie processes holding ports
- Multiple processes on the same port

**Example**:
```bash
wir --port 8080 --warnings
```

**Use when**: You're doing security audits or troubleshooting permission issues.

#### Port with Interactive Mode

```bash
wir --port <port> --interactive
# or
wir --port <port> -i
```

**What it does**: Shows port information and prompts you to kill the process using that port.

**Example**:
```bash
wir --port 8080 --interactive
# After showing port info:
# Press 'k' to kill process, 'q' to quit, or any other key to exit:
```

**Use when**:
- "Address already in use" error - inspect and kill the blocking process
- Quickly freeing up a port during development
- Cleaning up stuck processes on specific ports

### PID Mode

#### Basic Process Information

```bash
wir --pid <process_id>
```

**What it does**: Shows detailed information about the process.

**Example**:
```bash
wir --pid 1234
```

**Sample output**:
```
Process Information
  PID: 1234
  Name: node
  User: albz (UID: 501)
  Parent PID: 890
  State: Sleeping (S)
  Running for: 2 hours, 15 minutes
  Command: /usr/local/bin/node server.js
  Memory: VSZ=1234567 KB, RSS=45678 KB
```

**Output includes**:
- Process name
- User and UID
- Parent PID
- Process state (with human-readable name)
- Process uptime (how long it's been running)
- Command line
- Memory usage (VSZ and RSS)

#### Special PID Variables

You can use shell variables:

```bash
# Current shell
wir --pid $$

# Last background process
wir --pid $!

# Any variable
MY_PID=1234
wir --pid $MY_PID
```

#### Process Tree

```bash
wir --pid <pid> --tree
```

**What it does**: Shows the entire ancestry of the process, from the process up to the init system (PID 1).

**Example**:
```bash
wir --pid $$ --tree
```

**Sample Output**:
```
Process Ancestry Tree
zsh[51424] (albz)
  └─ Terminal[50112] (albz)
    └─ launchd[1] (root)
```

**Use when**:
- Understanding how a process was started
- Debugging process spawning issues
- Finding the parent of a rogue process

#### Environment Variables

```bash
wir --pid <pid> --env
```

**What it does**: Shows all environment variables for the process.

**Example**:
```bash
wir --pid $$ --env
```

**Note**: You can only see environment variables for:
- Your own processes
- Any process if you're running as root/sudo

**Use when**:
- Debugging configuration issues
- Checking what variables a process inherited
- Verifying environment setup

#### Process with JSON

```bash
wir --pid <pid> --json
```

**Example**:
```bash
wir --pid 1234 --json
```

**Use when**: Integrating with scripts, monitoring systems, or data pipelines.

#### Process with Interactive Mode

```bash
wir --pid <pid> --interactive
# or
wir --pid <pid> -i
```

**What it does**: Shows process information and prompts you to kill it with a keypress.

**Example**:
```bash
wir --pid 1234 --interactive
# After showing process info:
# Press 'k' to kill process, 'q' to quit, or any other key to exit:
```

**Use when**:
- You want to quickly inspect and potentially kill a process
- Debugging and need to stop a problematic process
- Cleaning up development processes interactively

**Note**:
- Sends SIGTERM (graceful termination) when you press 'k'
- You may need sudo to kill processes owned by other users
- Cannot be combined with `--json` output

### All Mode

#### List All Processes

```bash
wir --all
```

**What it does**: Shows all running processes on the system with details.

**Example**:
```bash
wir --all
```

**Sample Output**:
```
Running Processes (468 total)

PID      PPID     NAME                 USER         COMMAND
-------- -------- -------------------- ------------ -------
1234     1        systemd              root         /sbin/init
5678     1234     nginx                www-data     nginx: master process
...
```

**Use when**:
- Getting an overview of system activity
- Finding processes by name or user
- System monitoring and diagnostics
- Replacing `ps aux` with better formatting

#### All Processes (Short Format)

```bash
wir --all --short
```

**What it does**: Lists all processes, one per line, with minimal info.

**Example**:
```bash
wir --all --short
```

**Sample Output**:
```
1234: systemd by root
5678: nginx by www-data
9012: node by albz
...
```

**Use when**:
- Quick scans of running processes
- Piping to grep for filtering
- Counting specific processes

#### All Processes (JSON Format)

```bash
wir --all --json
```

**What it does**: Outputs complete process list as JSON.

**Example**:
```bash
wir --all --json | jq '.processes[] | select(.user=="albz")'
```

**Use when**:
- Scripting and automation
- Feeding data to monitoring systems
- Complex filtering with jq

### Global Options

#### Disable Colors

```bash
wir --no-color --port 8080
```

**Use when**:
- Output is going to a file
- Terminal doesn't support colors
- Piping to other commands
- Generating reports

**Example**:
```bash
wir --port 80 --no-color > port_80_info.txt
```

---

## Practical Examples

### Web Development

#### Check if your dev server port is available

```bash
wir --port 3000
```

If nothing is found, the port is free. If something is there, you'll see what's using it.

#### Find and kill a process on a port

```bash
# Interactive mode (easiest)
wir --port 8080 --interactive
# View info, then press 'k' to kill or 'q' to quit

# Or traditional way - Find the PID
wir --port 8080 --short
# Output: Port 8080: node[45678] by albz (LISTEN)

# Kill it manually
kill 45678

# Or in one command
kill $(wir --port 8080 --json | grep -o '"pid": [0-9]*' | head -1 | grep -o '[0-9]*')
```

### System Administration

#### Audit root processes on user ports

```bash
wir --port 8080 --warnings
```

This will flag if a root-owned process is listening on a non-privileged port (potential security issue).

#### Monitor a suspicious process

```bash
# Get basic info
wir --pid 12345

# See what spawned it
wir --pid 12345 --tree

# Check its environment
sudo wir --pid 12345 --env
```

#### Export all port information

```bash
for port in 22 80 443 3000 5432 6379 8080; do
    echo "Checking port $port..."
    wir --port $port --json >> ports_report.json 2>/dev/null
done
```

### Database Administration

#### Check database ports

```bash
# PostgreSQL
wir --port 5432

# MySQL/MariaDB
wir --port 3306

# MongoDB
wir --port 27017

# Redis
wir --port 6379
```

#### Verify database is running as correct user

```bash
wir --port 5432 --short
# Should show: Port 5432: postgres[...] by postgres (LISTEN)
```

### Debugging

#### Why is my port in use?

```bash
# See everything on the port
wir --port 8080

# Just the essential info
wir --port 8080 --short
```

#### What's the full command line?

```bash
wir --pid 12345
# Shows the complete command with all arguments
```

#### How was this process started?

```bash
wir --pid 12345 --tree
# Shows parent → grandparent → ... → init
```

### Scripting

#### Check if port is in use (exit code)

```bash
#!/bin/bash
if wir --port 8080 > /dev/null 2>&1; then
    echo "Port 8080 is in use"
    exit 1
else
    echo "Port 8080 is available"
fi
```

#### Get PID of process on port

```bash
#!/bin/bash
PID=$(wir --port 8080 --json 2>/dev/null | grep -o '"pid": [0-9]*' | head -1 | awk '{print $2}')
if [ -n "$PID" ]; then
    echo "Process on port 8080: $PID"
fi
```

#### Monitor multiple processes

```bash
#!/bin/bash
for pid in $(pgrep node); do
    echo "Node process $pid:"
    wir --pid $pid --short
done
```

---

## Understanding Output

### Process States

The `State` field shows the process state with a human-readable name and the state code in parentheses:

- `Running (R)` - Process is currently executing
- `Sleeping (S)` - Interruptible sleep (waiting for an event)
- `Waiting (D)` - Uninterruptible sleep (usually waiting for I/O)
- `Zombie (Z)` - Process terminated but not yet reaped by parent
- `Stopped (T)` - Process stopped by signal or debugger
- `Idle (I)` - Idle kernel thread (macOS)

Example output:
```
State: Running (R)
```

This makes it easier to understand what the process is doing at a glance.

### Process Uptime

The `Running for:` field shows how long the process has been running in a human-readable format:

**Examples**:
- `Running for: 5 seconds` - Just started
- `Running for: 3 minutes, 45 seconds` - Running for a few minutes
- `Running for: 2 hours, 15 minutes` - Running for hours
- `Running for: 5 days, 8 hours, 23 minutes` - Long-running process

This helps you quickly identify:
- Recently started processes (potential troublemakers)
- Long-running processes (stable services)
- Processes that might have been restarted

**JSON output** also includes:
- `start_time`: Unix timestamp when the process started
- `uptime`: Human-readable duration string

Example:
```json
{
  "state": "S",
  "state_name": "Sleeping",
  "start_time": 1766241982,
  "uptime": "9 days, 19 hours, 43 minutes"
}
```

### Connection States

For network connections:

- `LISTEN` - Listening for connections
- `ESTABLISHED` - Active connection
- `TIME_WAIT` - Connection closed, waiting for packets
- `CLOSE_WAIT` - Remote closed, local hasn't
- `SYN_SENT` - Attempting connection
- `SYN_RECV` - Received connection request

### Memory Information

- **VSZ** (Virtual Size): Total virtual memory the process can access
- **RSS** (Resident Set Size): Physical memory actually in RAM

Example:
```
VSZ=410762032 KB, RSS=3712 KB
```

The process has 401 GB of virtual memory space but only 3.7 MB is actually in RAM.

### Port Ranges

- **1-1023**: System/privileged ports (require root)
- **1024-49151**: Registered ports (common services)
- **49152-65535**: Dynamic/ephemeral ports (temporary)

---

## Tips & Tricks

### Finding Your Own Process

```bash
# Your shell
wir --pid $$

# Your parent process
wir --pid $PPID

# Current process in a script
wir --pid $$
```

### Identifying Recently Restarted Services

The uptime field helps you quickly spot services that recently restarted:

```bash
# Check a service's uptime
wir --pid $(pgrep nginx | head -1)

# If it shows "Running for: 3 minutes" but should have been up for days,
# you know it recently crashed and restarted!

# Compare uptimes of related processes
for pid in $(pgrep node); do
    echo "PID $pid:"
    wir --pid $pid | grep "Running for:"
done
```

### Combining with Other Tools

```bash
# Find all node processes and inspect them
pgrep node | while read pid; do wir --pid $pid --short; done

# Check all common web ports
for port in 80 443 8000 8080 3000; do
    wir --port $port --short 2>/dev/null
done

# Get JSON for programmatic use
wir --pid 1234 --json | jq '.name'
```

### Using with Watch

Monitor a port in real-time:

```bash
watch -n 1 'wir --port 8080 --short'
```

### Quick Aliases

Add to your `.bashrc` or `.zshrc`:

```bash
# What's on this port?
alias what='wir --port'

# Who is this process?
alias who='wir --pid'

# Quick process info
alias pinfo='wir --pid'

# Check common web ports
alias webcheck='for p in 80 443 3000 8080; do wir --port $p --short 2>/dev/null; done'
```

---

## Common Use Cases

### "Address already in use" Error

**Problem**: You try to start a server but get "address already in use".

**Solution**:
```bash
# Interactive mode (recommended)
wir --port 3000 --interactive
# Press 'k' to kill the process blocking the port

# Or traditional way:
# Find what's on the port
wir --port 3000

# If it's your old server, kill it
kill <PID from above>

# Verify it's gone
wir --port 3000  # Should show "No connections found"
```

### Mystery Process Using CPU

**Problem**: High CPU usage, you have the PID from `top`.

**Solution**:
```bash
# Get full details
wir --pid <PID>

# See the ancestry to understand how it started
wir --pid <PID> --tree

# Check environment for clues
sudo wir --pid <PID> --env
```

### Security Audit

**Problem**: Want to check for security issues.

**Solution**:
```bash
# Check all common ports for warnings
for port in 22 80 443 3000 8080; do
    echo "=== Port $port ==="
    wir --port $port --warnings 2>/dev/null
done

# Look for root processes on user ports
for port in $(seq 1024 65535); do
    wir --port $port --warnings 2>/dev/null | grep -i root && echo "Port: $port"
done
```

### Docker Container Debugging

**Problem**: Container port conflicts.

**Solution**:
```bash
# Check if host port is available
wir --port 8080

# Find container processes
docker inspect <container> | grep Pid
wir --pid <container_pid> --tree
```

---

## Troubleshooting

### "Permission denied" Errors

**Problem**: Can't access process or port information.

**Solutions**:

1. **For port queries on Linux**: Need root for network info
   ```bash
   sudo wir --port 80
   ```

2. **For other processes**: Can only see your own processes
   ```bash
   # This works
   wir --pid $$

   # This might fail
   wir --pid 1

   # This should work
   sudo wir --pid 1
   ```

3. **For environment variables**: Only your own or use sudo
   ```bash
   sudo wir --pid 1234 --env
   ```

### "No connections found"

**Problem**: You know something is on the port but wir says nothing.

**Possible causes**:

1. **Wrong port number**
   ```bash
   # Double-check with
   netstat -an | grep <port>
   lsof -i :<port>
   ```

2. **Need elevated privileges**
   ```bash
   sudo wir --port <port>
   ```

3. **Process just started/stopped**
   ```bash
   # Wait a moment and try again
   sleep 1 && wir --port <port>
   ```

### Colors Not Showing

**Problem**: Output is plain text without colors.

**Solutions**:

1. **Terminal doesn't support colors**: Use `--no-color`
2. **Output is redirected**: Colors are auto-disabled when piping
3. **Force colors** (if needed): Colors are on by default for terminal output

### JSON Parsing Issues

**Problem**: Can't parse JSON output.

**Solutions**:

1. **Use a JSON formatter**:
   ```bash
   wir --pid 1234 --json | jq .
   ```

2. **Check for errors mixed in**:
   ```bash
   wir --pid 1234 --json 2>/dev/null
   ```

3. **Validate JSON**:
   ```bash
   wir --pid 1234 --json | python -m json.tool
   ```

### Build Errors

See the main README.md for build troubleshooting.

---

## Advanced Usage

### Creating Reports

```bash
#!/bin/bash
# Port audit script

echo "Port Security Audit - $(date)" > report.txt
echo "================================" >> report.txt

for port in 22 80 443 3000 5432 6379 8080; do
    echo "" >> report.txt
    echo "Port $port:" >> report.txt
    wir --port $port --no-color 2>/dev/null >> report.txt
done
```

### Integration with Monitoring

```bash
#!/bin/bash
# Check if critical services are running

CRITICAL_PORTS="22 80 443 5432"

for port in $CRITICAL_PORTS; do
    if ! wir --port $port >/dev/null 2>&1; then
        echo "ALERT: Nothing listening on port $port"
        # Send alert (email, slack, etc.)
    fi
done
```

### Automated Cleanup

```bash
#!/bin/bash
# Kill old node processes on port 3000

PID=$(wir --port 3000 --json 2>/dev/null | grep -o '"pid": [0-9]*' | head -1 | awk '{print $2}')

if [ -n "$PID" ]; then
    echo "Killing old process $PID on port 3000"
    kill $PID
    sleep 1

    # Verify
    if wir --port 3000 >/dev/null 2>&1; then
        echo "Process didn't die, force killing"
        kill -9 $PID
    fi
fi
```

---

## Quick Reference Card

```
# Port queries
wir --port <n>              # Full info about port
wir --port <n> --short      # One-line summary
wir --port <n> --json       # JSON output
wir --port <n> --warnings   # Security warnings only
wir --port <n> -i           # Interactive mode (kill with 'k')

# Process queries
wir --pid <n>               # Full process info
wir --pid <n> --short       # One-line summary
wir --pid <n> --tree        # Show ancestry
wir --pid <n> --env         # Show environment
wir --pid <n> --json        # JSON output
wir --pid <n> -i            # Interactive mode (kill with 'k')

# List all processes
wir --all                   # Show all processes
wir --all --short           # One-line per process
wir --all --json            # JSON output

# Global options
--interactive, -i           # Enable interactive mode
--no-color                  # Disable colors
--help                      # Show help
--version                   # Show version info

# Special PIDs
$$                          # Current shell PID
$PPID                       # Parent PID
$!                          # Last background process
```

---

## Getting Help

- Run `wir --help` for quick reference
- Check the README.md for installation and building
- Report bugs at the project repository
- For system-level issues, check `man lsof`, `man netstat`, `man proc`

---

**Happy debugging!** 🔍
