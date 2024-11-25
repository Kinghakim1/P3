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

// Find executable in search paths
char* find_executable(const char* cmd) {
    // If cmd contains '/', treat as direct path
    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0) {
            return strdup(cmd);
        }
        return NULL;
    }

    // Search in specified directories
    for (int i = 0; i < SEARCH_PATHS_COUNT; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", SEARCH_PATHS[i], cmd);
        
        if (access(path, X_OK) == 0) {
            return strdup(path);
        }
    }

    return NULL;
}

// Handle built-in commands
int execute_builtin(Command* cmd) {
    // cd command
    if (strcmp(cmd->tokens[0], "cd") == 0) {
        if (cmd->token_count != 2) {
            fprintf(stderr, "cd: expected one argument\n");
            return 1;
        }
        
        if (chdir(cmd->tokens[1]) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }

    // pwd command
    if (strcmp(cmd->tokens[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
            return 0;
        }
        perror("pwd");
        return 1;
    }

    // which command
    if (strcmp(cmd->tokens[0], "which") == 0) {
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
        return 1;
    }

    // exit command
    if (strcmp(cmd->tokens[0], "exit") == 0) {
        // Print any arguments
        if (cmd->token_count > 1) {
            for (int i = 1; i < cmd->token_count; i++) {
                printf("%s%s", cmd->tokens[i], 
                       i < cmd->token_count - 1 ? " " : "\n");
            }
        }
        exit(0);
    }

    return -1; // Not a builtin
}

// Execute external command
int execute_external_command(Command* cmd, int interactive) {
    // Find executable path
    char* executable_path = find_executable(cmd->tokens[0]);
    if (!executable_path) {
        fprintf(stderr, "%s: command not found\n", cmd->tokens[0]);
        return 127; // Standard exit code for command not found
    }

    // Fork a child process
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        perror("fork");
        free(executable_path);
        return 1;
    }

    if (pid == 0) {
        // Child process
        // Replace 0th token with full path
        cmd->tokens[0] = executable_path;
        
        // Null-terminate the token list
        cmd->tokens[cmd->token_count] = NULL;

        // Execute the command
        execv(executable_path, cmd->tokens);
        
        // If execv fails
        perror("execv");
        exit(1);
    }

    // Parent process
    int status;
    waitpid(pid, &status, 0);
    
    // Free the executable path
    free(executable_path);

    // Handle exit status
    if (interactive) {
        handle_exit_status(status, interactive);
    }

    return WEXITSTATUS(status);
}

// Handle and report exit status
void handle_exit_status(int status, int interactive) {
    if (!interactive) return;

    if (WIFEXITED(status)) {
        // Exited normally
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            fprintf(stderr, "Command failed: code %d\n", exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        // Terminated by a signal
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

int main(int argc, char* argv[]) {
    char input[MAX_INPUT_SIZE];
    int interactive = isatty(STDIN_FILENO);

    // Interactive mode welcome
    if (interactive) {
        printf("Welcome to my shell!\n");
    }

    // Main command loop
    while (1) {
        // Print prompt in interactive mode
        if (interactive) {
            printf("mysh> ");
            fflush(stdout);
        }

        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // EOF or error
            break;
        }

        // Skip empty lines
        if (input[0] == '\n') continue;

        // Parse command
        Command* cmd = parse_command(input);
        if (cmd->token_count == 0) {
            free_command(cmd);
            continue;
        }

        // Execute command
        int result;
        if (cmd->is_builtin) {
            result = execute_builtin(cmd);
        } else {
            result = execute_external_command(cmd, interactive);
        }

        // Free command resources
        free_command(cmd);
    }

    // Interactive mode goodbye
    if (interactive) {
        printf("mysh: exiting\n");
    }

    return 0;
}