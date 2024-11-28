#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64
#define MAX_TOKEN_LENGTH 256
#define SEARCH_PATHS_COUNT 3

// Search paths for executables
const char* SEARCH_PATHS[] = {
    "/usr/local/bin",
    "/usr/bin",
    "/bin"
};

// Command structure to hold parsed command details
typedef struct {
    char* tokens[MAX_TOKENS];
    int token_count;
    int is_builtin;
} Command;

// Function prototypes
void handle_exit_status(int status, int interactive);
char* find_executable(const char* cmd);
int execute_builtin(Command* cmd);
int execute_external_command(Command* cmd, int interactive);
void free_command(Command* cmd);
Command* parse_command(char* input);

// Improved built-in commands
int builtin_cd(Command* cmd);
int builtin_pwd(Command* cmd);
int builtin_which(Command* cmd);
int builtin_exit(Command* cmd);

// Parse input into tokens
Command* parse_command(char* input) {
    Command* cmd = malloc(sizeof(Command));
    memset(cmd, 0, sizeof(Command));

    // Remove newline
    input[strcspn(input, "\n")] = 0;

    // Tokenize input
    char* token = strtok(input, " ");
    while (token && cmd->token_count < MAX_TOKENS - 1) {
        cmd->tokens[cmd->token_count] = strdup(token);
        cmd->token_count++;
        token = strtok(NULL, " ");
    }

    // Check if it's a builtin command
    if (cmd->token_count > 0) {
        if (strcmp(cmd->tokens[0], "cd") == 0 ||
            strcmp(cmd->tokens[0], "pwd") == 0 ||
            strcmp(cmd->tokens[0], "which") == 0 ||
            strcmp(cmd->tokens[0], "exit") == 0) {
            cmd->is_builtin = 1;
        }
    }

    return cmd;
}

// Enhanced `cd` with debugging
int builtin_cd(Command* cmd) {
    // Debugging: Print tokens for troubleshooting
    fprintf(stderr, "Debug: cmd->token_count = %d\n", cmd->token_count);
    for (int i = 0; i < cmd->token_count; i++) {
        fprintf(stderr, "Debug: cmd->tokens[%d] = %s\n", i, cmd->tokens[i]);
    }

    // Handle "cd" without arguments (default to $HOME)
    if (cmd->token_count == 1) {
        char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Debug: HOME environment variable is not set.\n");
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return 1;
        }
        fprintf(stderr, "Debug: HOME = %s\n", home); // Print home directory for debugging
        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }

    // Handle "cd" with one argument
    if (cmd->token_count == 2) {
        fprintf(stderr, "Debug: Changing directory to %s\n", cmd->tokens[1]);
        if (chdir(cmd->tokens[1]) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }

    // Handle "cd" with too many arguments
    fprintf(stderr, "Debug: Too many arguments provided to cd.\n");
    fprintf(stderr, "cd: expected one argument\n");
    return 1;
}

// Handle the `pwd` command
int builtin_pwd(Command* cmd) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    }
    perror("pwd");
    return 1;
}

// Handle the `which` command
int builtin_which(Command* cmd) {
    if (cmd->token_count != 2) {
        fprintf(stderr, "which: expected one argument\n");
        return 1;
    }

    char* path = find_executable(cmd->tokens[1]);
    if (path) {
        printf("%s\n", path);
        free(path);
        return 0;
    }
    fprintf(stderr, "which: command not found: %s\n", cmd->tokens[1]);
    return 1;
}

// Handle the `exit` command
int builtin_exit(Command* cmd) {
    // Print any arguments
    if (cmd->token_count > 1) {
        for (int i = 1; i < cmd->token_count; i++) {
            printf("%s%s", cmd->tokens[i], i < cmd->token_count - 1 ? " " : "\n");
        }
    }
    printf("mysh: exiting\n");
    exit(0);
}

// Find executable in search paths
char* find_executable(const char* cmd) {
    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0) {
            return strdup(cmd);
        }
        return NULL;
    }

    for (int i = 0; i < SEARCH_PATHS_COUNT; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", SEARCH_PATHS[i], cmd);
        if (access(path, X_OK) == 0) {
            return strdup(path);
        }
    }

    return NULL;
}

// Execute built-in commands
int execute_builtin(Command* cmd) {
    if (strcmp(cmd->tokens[0], "cd") == 0) return builtin_cd(cmd);
    if (strcmp(cmd->tokens[0], "pwd") == 0) return builtin_pwd(cmd);
    if (strcmp(cmd->tokens[0], "which") == 0) return builtin_which(cmd);
    if (strcmp(cmd->tokens[0], "exit") == 0) return builtin_exit(cmd);

    return -1;
}

// Execute external command
int execute_external_command(Command* cmd, int interactive) {
    char* executable_path = find_executable(cmd->tokens[0]);
    if (!executable_path) {
        fprintf(stderr, "%s: command not found\n", cmd->tokens[0]);
        return 127;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(executable_path);
        return 1;
    }

    if (pid == 0) {
        cmd->tokens[cmd->token_count] = NULL;
        execv(executable_path, cmd->tokens);
        perror("execv");
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    free(executable_path);

    if (interactive) handle_exit_status(status, interactive);

    return WEXITSTATUS(status);
}

// Handle and report exit status
void handle_exit_status(int status, int interactive) {
    if (!interactive) return;

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            fprintf(stderr, "Command failed: code %d\n", exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        int signal_num = WTERMSIG(status);
        fprintf(stderr, "Terminated by signal: %d\n", signal_num);
    }
}

// Free command resources
void free_command(Command* cmd) {
    for (int i = 0; i < cmd->token_count; i++) {
        free(cmd->tokens[i]);
    }
    free(cmd);
}

// Main function
int main(int argc, char* argv[]) {
    char input[MAX_INPUT_SIZE];
    int interactive = isatty(STDIN_FILENO);

    if (interactive) {
        printf("Welcome to my shell!\n");
    }

    while (1) {
        if (interactive) {
            printf("mysh> ");
            fflush(stdout);
        }

        if (fgets(input, sizeof(input), stdin) == NULL) break;

        if (input[0] == '\n') continue;

        Command* cmd = parse_command(input);
        if (cmd->token_count == 0) {
            free_command(cmd);
            continue;
        }

        int result = cmd->is_builtin
            ? execute_builtin(cmd)
            : execute_external_command(cmd, interactive);

        free_command(cmd);
    }

    if (interactive) {
        printf("mysh: exiting\n");
    }

    return 0;
}