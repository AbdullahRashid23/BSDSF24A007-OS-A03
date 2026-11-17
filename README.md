# PUCIT Custom UNIX Shell (`myshell`)

**Student Roll No**: BSDSF24A007  
**Course**: Operating Systems  
**Instructor**: Dr. Muhammad Arif Butt  
**Assignment**: Programming Assignment 03  

---

## Overview

A modular, extensible UNIX command-line interpreter (shell) written in C, built incrementally using a professional Git branching, tagging, and GitHub release workflow.

---

## Features Implemented

1. **Feature 1: The Base Shell (Release `v1.0-base`)**
   - Core fork-exec-wait process execution cycle.
   - Modular structure: `main.c`, `shell.c`, `execute.c`, and `shell.h`.
   - Makefile build system generating `bin/myshell`.

2. **Feature 2: Built-in Commands (Release `v2`)**
   - Execution in the parent shell process without unnecessary forks.
   - Built-in commands: `cd <dir>`, `exit`, `help`, and placeholder `jobs`.

3. **Feature 3: Command History (Release `v3`)**
   - In-memory array storing the last 100 commands.
   - `history` built-in command with 1-based numbering.
   - `!n` syntax to recall, display, and execute the *n*-th command from history.

4. **Feature 4: Tab Completion with GNU Readline (Release `v4`)**
   - Integrated GNU Readline library (`-lreadline`).
   - Command auto-completion for built-in commands and files in current directory via `Tab`.
   - Up/Down arrow history cycling out-of-the-box.

5. **Feature 5: I/O Redirection & Pipelines (Release `v5`)**
   - Output redirection (`>`) to truncate/create files.
   - Input redirection (`<`) from file descriptors.
   - Multi-command pipelines (`|`) using `pipe()`, `dup2()`, and file descriptor management.

6. **Feature 6: Command Chaining & Background Execution (Release `v6`)**
   - Sequential command chaining with semicolons (`;`).
   - Background job execution (`&`) without blocking prompt.
   - Asynchronous zombie reaping using `waitpid(-1, &status, WNOHANG)`.
   - Updated `jobs` command displaying active background jobs and PIDs.

7. **Feature 7: If-Then-Else-Fi Control Structure (Release `v7`)**
   - Multi-line scripting control structure: `if ... then ... else ... fi`.
   - Inspection of child process exit status using `WEXITSTATUS(status)`.

8. **Feature 8: Shell Variables (Release `v8`)**
   - Variable assignment syntax: `VARNAME=value`.
   - Variable expansion before execution: `$VARNAME`.
   - Built-in `set` command to print all defined shell variables.

9. **Feature 9: Collaborative Git Workflow**
   - Mock bug reproduction and pull request simulation on branch `fix-help-command-bug`.
   - Fix merged into `main` with updated `help` command listing `set`.

---

## Directory Structure

```
BSDSF24A007-OS-A03/
├── src/
│   ├── main.c
│   ├── shell.c
│   └── execute.c
├── include/
│   └── shell.h
├── Makefile
├── .gitignore
└── README.md
```

---

## Build & Run

### Prerequisites
- GCC compiler
- GNU Make
- GNU Readline development library (`libreadline-dev` / `libreadline`)

### Compilation
```bash
make clean
make
```

### Running the Shell
```bash
./bin/myshell
```

### Cleaning Build Artifacts
```bash
make clean
```
