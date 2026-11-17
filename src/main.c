#include "shell.h"

int main() {
    char* cmdline;

    initialize_readline();

    while ((cmdline = readline(PROMPT)) != NULL) {
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

        // Add to Readline history
        add_history(cmdline);

        // Add command to custom history storage
        add_to_history(cmdline);

        // Execute command line (handles pipes and redirections)
        execute_line(cmdline);

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
