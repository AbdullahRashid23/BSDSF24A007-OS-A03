#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 1024
#define MAXARGS 64
#define ARGLEN 128
#define PROMPT "FCIT> "
#define HISTORY_SIZE 100

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
void free_arglist(char** arglist);
int execute(char** arglist);
int execute_line(char* cmdline);
int handle_builtin(char** arglist);
int is_builtin(const char* cmd);

// History prototypes
void add_to_history(const char* cmd);
void print_history(void);
char* get_history(int index);
int get_history_count(void);

// Readline completion
void initialize_readline(void);

#endif // SHELL_H
