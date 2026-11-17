#include "shell.h"

static char* history[HISTORY_SIZE];
static int history_count = 0;

static Job background_jobs[MAX_JOBS];
static int job_count = 0;
static int next_job_id = 1;

static ShellVar* var_head = NULL;

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

void add_job(pid_t pid, const char* cmd) {
    if (job_count < MAX_JOBS) {
        background_jobs[job_count].pid = pid;
        background_jobs[job_count].job_id = next_job_id++;
        strncpy(background_jobs[job_count].command, cmd, MAX_LEN - 1);
        background_jobs[job_count].command[MAX_LEN - 1] = '\0';
        job_count++;
    }
}

int get_last_job_id(void) {
    if (job_count > 0) {
        return background_jobs[job_count - 1].job_id;
    }
    return 1;
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (background_jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                background_jobs[j] = background_jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void reap_zombies(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

void print_jobs(void) {
    reap_zombies();
    for (int i = 0; i < job_count; i++) {
        printf("[%d]  PID: %d   Running   %s\n",
               background_jobs[i].job_id,
               background_jobs[i].pid,
               background_jobs[i].command);
    }
}

void set_variable(const char* name, const char* value) {
    ShellVar* curr = var_head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            free(curr->value);
            curr->value = strdup(value ? value : "");
            return;
        }
        curr = curr->next;
    }

    ShellVar* new_node = (ShellVar*)malloc(sizeof(ShellVar));
    new_node->name = strdup(name);
    new_node->value = strdup(value ? value : "");
    new_node->next = var_head;
    var_head = new_node;
}

const char* get_variable(const char* name) {
    ShellVar* curr = var_head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    return NULL;
}

void print_variables(void) {
    ShellVar* curr = var_head;
    while (curr != NULL) {
        printf("%s=%s\n", curr->name, curr->value);
        curr = curr->next;
    }
}

int is_assignment(const char* str) {
    if (str == NULL) return 0;
    while (*str == ' ' || *str == '\t') str++;

    if (!isalpha((unsigned char)*str) && *str != '_') {
        return 0;
    }

    const char* p = str;
    while (isalnum((unsigned char)*p) || *p == '_') {
        p++;
    }

    if (*p == '=') {
        return 1;
    }
    return 0;
}

int handle_assignment(const char* str) {
    while (*str == ' ' || *str == '\t') str++;

    const char* eq = strchr(str, '=');
    if (eq == NULL) return 0;

    int name_len = eq - str;
    char name[MAX_LEN];
    if (name_len >= MAX_LEN) name_len = MAX_LEN - 1;
    strncpy(name, str, name_len);
    name[name_len] = '\0';

    const char* val_start = eq + 1;
    char val[MAX_LEN];
    int vi = 0;

    if (*val_start == '"' || *val_start == '\'') {
        char quote = *val_start++;
        while (*val_start != '\0' && *val_start != quote && vi < MAX_LEN - 1) {
            val[vi++] = *val_start++;
        }
    } else {
        while (*val_start != '\0' && *val_start != '\n' && vi < MAX_LEN - 1) {
            val[vi++] = *val_start++;
        }
        // Trim trailing whitespace
        while (vi > 0 && (val[vi - 1] == ' ' || val[vi - 1] == '\t')) {
            vi--;
        }
    }
    val[vi] = '\0';

    set_variable(name, val);
    return 1;
}

void expand_variables(char** arglist) {
    if (arglist == NULL) return;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (arglist[i][0] == '$') {
            const char* var_name = arglist[i] + 1;
            const char* val = get_variable(var_name);
            if (val == NULL) {
                val = getenv(var_name);
            }
            if (val == NULL) {
                val = "";
            }
            free(arglist[i]);
            arglist[i] = strdup(val);
        }
    }
}

static char* command_generator(const char* text, int state) {
    static int list_index, len;
    static const char* builtins[] = {
        "cd", "exit", "help", "history", "jobs", "set", NULL
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
        strcmp(cmd, "jobs") == 0 ||
        strcmp(cmd, "set") == 0) {
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
        printf("  set             : Display all shell variables\n");
        return 1;
    } else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    } else if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    } else if (strcmp(arglist[0], "set") == 0) {
        print_variables();
        return 1;
    }

    return 0;
}
