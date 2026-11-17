#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        // Skip leading whitespace to inspect command
        char* trimmed = cmdline;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        // Empty line
        if (*trimmed == '\0') {
            free(cmdline);
            continue;
        }

        // Check for !n re-execution
        if (*trimmed == '!') {
            char* endptr;
            long n = strtol(trimmed + 1, &endptr, 10);
            if (endptr == trimmed + 1 || n < 1 || n > get_history_count()) {
                fprintf(stderr, "myshell: %s: event not found\n", trimmed);
                free(cmdline);
                continue;
            }
            char* retrieved = get_history((int)n - 1);
            printf("%s\n", retrieved);
            char* new_cmd = strdup(retrieved);
            free(cmdline);
            cmdline = new_cmd;
        }

        // Add command to history
        add_to_history(cmdline);

        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
