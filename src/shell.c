#include "shell.h"

static char* history[HISTORY_SIZE];
static int history_count = 0;

void add_to_history(const char* cmd) {
    if (cmd == NULL || *cmd == '\0') return;

    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

void print_history(void) {
    for (int i = 0; i < history_count; i++) {
        printf("%4d  %s\n", i + 1, history[i]);
    }
}

char* get_history(int index) {
    if (index >= 0 && index < history_count) {
        return history[index];
    }
    return NULL;
}

int get_history_count(void) {
    return history_count;
}

static char* command_generator(const char* text, int state) {
    static int list_index, len;
    static const char* builtins[] = {
        "cd", "exit", "help", "history", "jobs", NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    const char* name;
    while ((name = builtins[list_index++]) != NULL) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    return NULL;
}

static char** shell_completion(const char* text, int start, int end) {
    (void)end;
    char** matches = NULL;
    if (start == 0) {
        matches = rl_completion_matches(text, command_generator);
    }
    return matches;
}

void initialize_readline(void) {
    rl_attempted_completion_function = shell_completion;
}

char* read_cmd(char* prompt, FILE* fp) {
    (void)fp;
    return readline(prompt);
}

char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i <= MAXARGS; i++) {
        arglist[i] = NULL;
    }

    const char* p = cmdline;
    int argnum = 0;

    while (*p != '\0' && argnum < MAXARGS) {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (*p == '\0') break;

        char buf[MAX_LEN];
        int bi = 0;

        if (*p == '<' || *p == '>') {
            buf[bi++] = *p++;
            buf[bi] = '\0';
            arglist[argnum++] = strdup(buf);
            continue;
        }

        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '<' && *p != '>') {
            if (*p == '"' || *p == '\'') {
                char quote = *p++;
                while (*p != '\0' && *p != quote) {
                    if (bi < MAX_LEN - 1) buf[bi++] = *p;
                    p++;
                }
                if (*p == quote) p++;
            } else {
                if (bi < MAX_LEN - 1) buf[bi++] = *p;
                p++;
            }
        }
        buf[bi] = '\0';
        if (bi > 0) {
            arglist[argnum++] = strdup(buf);
        }
    }

    if (argnum == 0) {
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

void free_arglist(char** arglist) {
    if (arglist == NULL) return;
    for (int i = 0; arglist[i] != NULL; i++) {
        free(arglist[i]);
    }
    free(arglist);
}

int is_builtin(const char* cmd) {
    if (cmd == NULL) return 0;
    if (strcmp(cmd, "exit") == 0 ||
        strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "help") == 0 ||
        strcmp(cmd, "history") == 0 ||
        strcmp(cmd, "jobs") == 0) {
        return 1;
    }
    return 0;
}

int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            char* home = getenv("HOME");
            if (home != NULL) {
                if (chdir(home) != 0) {
                    perror("cd");
                }
            }
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <directory>  : Change working directory\n");
        printf("  exit            : Terminate the shell\n");
        printf("  help            : Display this help message\n");
        printf("  history         : Display command history\n");
        printf("  jobs            : List background jobs\n");
        return 1;
    } else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    } else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0;
}
