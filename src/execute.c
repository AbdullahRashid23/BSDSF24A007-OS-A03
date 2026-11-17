#include "shell.h"

int execute(char* arglist[]) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    int status;
    pid_t cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            return 1;
        case 0: // Child process
            execvp(arglist[0], arglist);
            perror("Command not found");
            exit(1);
        default: // Parent process
            waitpid(cpid, &status, 0);
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

static void execute_child_command(char* cmd) {
    char** arglist = tokenize(cmd);
    if (arglist == NULL || arglist[0] == NULL) {
        exit(0);
    }

    char* infile = NULL;
    char* outfile = NULL;
    char* clean_args[MAXARGS + 1];
    int c = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            if (arglist[i + 1] != NULL) {
                infile = arglist[++i];
            } else {
                fprintf(stderr, "myshell: syntax error near unexpected token 'newline'\n");
                exit(1);
            }
        } else if (strcmp(arglist[i], ">") == 0) {
            if (arglist[i + 1] != NULL) {
                outfile = arglist[++i];
            } else {
                fprintf(stderr, "myshell: syntax error near unexpected token 'newline'\n");
                exit(1);
            }
        } else {
            clean_args[c++] = arglist[i];
        }
    }
    clean_args[c] = NULL;

    if (c == 0) {
        exit(0);
    }

    if (infile != NULL) {
        int fd_in = open(infile, O_RDONLY);
        if (fd_in < 0) {
            perror(infile);
            exit(1);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    if (outfile != NULL) {
        int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror(outfile);
            exit(1);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    if (is_builtin(clean_args[0])) {
        handle_builtin(clean_args);
        exit(0);
    }

    execvp(clean_args[0], clean_args);
    perror(clean_args[0]);
    exit(1);
}

static int execute_single_command(char* cmd) {
    int has_redirection = (strchr(cmd, '<') != NULL || strchr(cmd, '>') != NULL);

    if (!has_redirection) {
        char** arglist = tokenize(cmd);
        if (arglist == NULL || arglist[0] == NULL) {
            free_arglist(arglist);
            return 0;
        }

        if (is_builtin(arglist[0])) {
            int ret = handle_builtin(arglist);
            free_arglist(arglist);
            return (ret == 1) ? 0 : 1;
        }
        free_arglist(arglist);
    }

    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork failed");
        return 1;
    }

    if (cpid == 0) {
        execute_child_command(cmd);
        exit(1);
    }

    int status;
    waitpid(cpid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

int execute_line(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0') {
        return 0;
    }

    if (strchr(cmdline, '|') == NULL) {
        return execute_single_command(cmdline);
    }

    char line_copy[MAX_LEN];
    strncpy(line_copy, cmdline, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char* cmds[MAXARGS];
    int num_cmds = 0;

    char* p = line_copy;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        char* start = p;
        while (*p != '\0' && *p != '|') p++;

        if (*p == '|') {
            *p = '\0';
            p++;
        }
        cmds[num_cmds++] = start;
        if (num_cmds >= MAXARGS) break;
    }

    if (num_cmds == 0) return 0;
    if (num_cmds == 1) return execute_single_command(cmds[0]);

    int pipes[MAXARGS - 1][2];
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }

    pid_t pids[MAXARGS];
    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return 1;
        }

        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execute_child_command(cmds[i]);
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int last_status = 0;
    for (int i = 0; i < num_cmds; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == num_cmds - 1) {
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    }

    return last_status;
}

void execute_chained(char* line) {
    if (line == NULL || line[0] == '\0') return;

    // Split commands by ';' respecting quotes
    const char* p = line;
    while (*p != '\0') {
        char cmd_buf[MAX_LEN];
        int bi = 0;

        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        while (*p != '\0') {
            if (*p == '"' || *p == '\'') {
                char quote = *p;
                if (bi < MAX_LEN - 1) cmd_buf[bi++] = *p++;
                while (*p != '\0' && *p != quote) {
                    if (bi < MAX_LEN - 1) cmd_buf[bi++] = *p++;
                }
                if (*p == quote) {
                    if (bi < MAX_LEN - 1) cmd_buf[bi++] = *p++;
                }
            } else if (*p == ';') {
                p++; // Consume ';'
                break;
            } else {
                if (bi < MAX_LEN - 1) cmd_buf[bi++] = *p++;
            }
        }
        cmd_buf[bi] = '\0';

        // Trim leading and trailing whitespace
        char* start = cmd_buf;
        while (*start == ' ' || *start == '\t') start++;
        int len = strlen(start);
        while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t')) {
            start[--len] = '\0';
        }

        if (len == 0) continue;

        // Check for background '&'
        int is_bg = 0;
        if (start[len - 1] == '&') {
            is_bg = 1;
            start[--len] = '\0';
            while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t')) {
                start[--len] = '\0';
            }
        }

        if (len == 0) continue;

        if (is_bg) {
            pid_t bg_pid = fork();
            if (bg_pid == -1) {
                perror("fork bg failed");
            } else if (bg_pid == 0) {
                int code = execute_line(start);
                exit(code);
            } else {
                char job_desc[MAX_LEN + 16];
                snprintf(job_desc, sizeof(job_desc), "%s &", start);
                add_job(bg_pid, job_desc);
                printf("[%d] %d\n", get_last_job_id(), bg_pid);
            }
        } else {
            execute_line(start);
        }
    }
}
