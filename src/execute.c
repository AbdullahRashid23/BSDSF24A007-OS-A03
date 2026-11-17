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
    // Check if redirection is present
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

    // Fork and execute
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

    // Check if pipe is present
    if (strchr(cmdline, '|') == NULL) {
        return execute_single_command(cmdline);
    }

    // Pipeline handling
    char line_copy[MAX_LEN];
    strncpy(line_copy, cmdline, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char* cmds[MAXARGS];
    int num_cmds = 0;

    // Split line by '|'
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
            // Child process i
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds in child
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execute_child_command(cmds[i]);
            exit(1);
        }
    }

    // Parent closes all pipe file descriptors
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children in pipeline
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
