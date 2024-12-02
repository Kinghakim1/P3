
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <fnmatch.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64
#define SEARCH_PATHS_COUNT 3

// Search paths for executables
const char* SEARCH_PATHS[] = {
    "/usr/local/bin",
    "/usr/bin",
    "/bin"
};

// Updated Command structure to hold parsed command details
typedef struct Command {
    char* tokens[MAX_TOKENS];
    int token_count;
    int is_builtin;
    char* input_file;     // Input redirection
    char* output_file;    // Output redirection
    int has_pipe;         // Pipeline indicator
    struct Command* next; // Next command in pipeline
} Command;

// Function prototypes
void handle_exit_status(int status, int interactive);
char* find_executable(const char* cmd);
int execute_builtin(Command* cmd);
int execute_external_command(Command* cmd, int interactive);
void free_command(Command* cmd);
Command* parse_command(char* input);
void expand_wildcards(Command* cmd);
void handle_redirection(Command* cmd);
void handle_pipeline(Command* cmd1, Command* cmd2);

// Wildcard Expansion
void expand_wildcards(Command* cmd) {
    for (int i = 0; i < cmd->token_count; i++) {
        if (strchr(cmd->tokens[i], '*')) {
            DIR* dir = opendir(".");
            if (!dir) {
                perror("opendir");
                return;
            }
            struct dirent* entry;
            char* pattern = cmd->tokens[i];
            char** matches = calloc(MAX_TOKENS, sizeof(char*));
            int match_count = 0;
            
            while ((entry = readdir(dir))) {
                if (fnmatch(pattern, entry->d_name, 0) == 0) {
                    matches[match_count++] = strdup(entry->d_name);
                }
            }
            closedir(dir);
            
            if (match_count > 0) {
                free(cmd->tokens[i]);
                for (int j = 0; j < match_count; j++) {
                    cmd->tokens[i + j] = matches[j];
                }
                cmd->token_count += match_count - 1;
                free(matches);
            } else {
                fprintf(stderr, "No matches for wildcard: %s\n", pattern);
                free(matches);
            }
        }
    }
}

// Redirection Handling
void handle_redirection(Command* cmd) {
    if (cmd->input_file) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror("Error opening input file");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (cmd->output_file) {
        int fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Error opening output file");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

// Pipeline Handling
void handle_pipeline(Command* cmd1, Command* cmd2) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        execute_external_command(cmd1, 0);
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        execute_external_command(cmd2, 0);
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}



// Parse Command with Redirection and Pipeline Support
Command* parse_command(char* input) {
    Command* cmd = malloc(sizeof(Command));
    memset(cmd, 0, sizeof(Command));
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->next = NULL;
    cmd->has_pipe = 0;

    input[strcspn(input, "\n")] = 0;

    char* token = strtok(input, " ");
    while (token && cmd->token_count < MAX_TOKENS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token) {
                cmd->input_file = strdup(token);
            } else {
                fprintf(stderr, "Error: Missing input file\n");
                free_command(cmd);
                return NULL;
            }
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token) {
                cmd->output_file = strdup(token);
            } else {
                fprintf(stderr, "Error: Missing output file\n");
                free_command(cmd);
                return NULL;
            }
        } else if (strcmp(token, "|") == 0) {
            cmd->has_pipe = 1;
            char* rest_of_input = strtok(NULL, "\n");
            if (rest_of_input) {
                cmd->next = parse_command(rest_of_input);
            }
            break;
        } else {
            cmd->tokens[cmd->token_count] = strdup(token);
            cmd->token_count++;
        }
        token = strtok(NULL, " ");
    }

    // Check for builtin commands
    if (cmd->token_count > 0) {
        const char* builtin_cmds[] = {"cd", "pwd", "which", "exit"};
        for (size_t i = 0; i < sizeof(builtin_cmds)/sizeof(builtin_cmds[0]); i++) {
            if (strcmp(cmd->tokens[0], builtin_cmds[i]) == 0) {
                cmd->is_builtin = 1;
                break;
            }
        }
    }

    return cmd;
}



// Find Executable
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

// Execute External Command
int execute_external_command(Command* cmd, int interactive) {
    char* executable_path = find_executable(cmd->tokens[0]);
    if (!executable_path) {
        fprintf(stderr, "%s: command not found\n", cmd->tokens[0]);
        return 127;
    }

    cmd->tokens[cmd->token_count] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(executable_path);
        return 1;
    }

    if (pid == 0) {
        // Child process
        handle_redirection(cmd);
        execv(executable_path, cmd->tokens);
        perror("execv");
        exit(1);
    }

    // Parent process
    free(executable_path);
    
    int status;
    waitpid(pid, &status, 0);
    
    if (interactive) {
        handle_exit_status(status, interactive);
    }

    return WEXITSTATUS(status);
}

// Builtin Commands
int builtin_cd(Command* cmd) {
    if (cmd->token_count == 1) {
        char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }
    } else if (cmd->token_count == 2) {
        if (chdir(cmd->tokens[1]) != 0) {
            perror("cd");
            return 1;
        }
    } else {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }
    return 0;
}

int builtin_pwd(Command* cmd) {
    (void)cmd;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    }
    perror("pwd");
    return 1;
}

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
    
    fprintf(stderr, "which: %s not found\n", cmd->tokens[1]);
    return 1;
}

int builtin_exit(Command* cmd) {
    // Print any arguments (if provided) before the exit message
    if (cmd->token_count > 1) {
        for (int i = 1; i < cmd->token_count; i++) {
            printf("%s%s", cmd->tokens[i], i < cmd->token_count - 1 ? " " : "\n");
        }
    }

    // Print the exiting message before terminating the shell
    printf("Exiting my shell.\n");

    // Exit the shell
    exit(0);
}


// Execute Builtin Commands
int execute_builtin(Command* cmd) {
    if (strcmp(cmd->tokens[0], "cd") == 0) return builtin_cd(cmd);
    if (strcmp(cmd->tokens[0], "pwd") == 0) return builtin_pwd(cmd);
    if (strcmp(cmd->tokens[0], "which") == 0) return builtin_which(cmd);
    if (strcmp(cmd->tokens[0], "exit") == 0) return builtin_exit(cmd);
    return -1;
}

// Handle Exit Status
void handle_exit_status(int status, int interactive) {
    if (!interactive) return;

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            fprintf(stderr, "Command failed: exit code %d\n", exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        int signal_num = WTERMSIG(status);
        fprintf(stderr, "Terminated by signal: %d\n", signal_num);
    }
}

// Free Command Resources
void free_command(Command* cmd) {
    if (!cmd) return;

    for (int i = 0; i < cmd->token_count; i++) {
        free(cmd->tokens[i]);
    }

    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    if (cmd->next) free_command(cmd->next);

    free(cmd);
}

// Main Shell Loop
int main(int argc, char* argv[]) {
    char input[MAX_INPUT_SIZE];
    int interactive = isatty(STDIN_FILENO);

    if (interactive) {
        printf("Welcome to MyShell!\n");
    }

    FILE *input_file = NULL;
    if (argc == 2) {
        input_file = fopen(argv[1], "r");
        if (input_file == NULL) {
            perror("Error opening script file");
            return 1;
        }
    }

    while (1) {
        if (interactive) {
            printf("mysh> ");
            fflush(stdout);
        }

        // Read input from file or stdin
        if (input_file) {
            if (fgets(input, sizeof(input), input_file) == NULL) break;
        } else {
            if (fgets(input, sizeof(input), stdin) == NULL) break;
        }

        if (input[0] == '\n') continue;

        Command* cmd = parse_command(input);
        if (!cmd || cmd->token_count == 0) {
            free_command(cmd);
            continue;
        }

        // Wildcard expansion
        expand_wildcards(cmd);

        // Execute command
        if (cmd->is_builtin) {
            execute_builtin(cmd);
        } else if (cmd->has_pipe && cmd->next) {
            handle_pipeline(cmd, cmd->next);
        } else {
            execute_external_command(cmd, interactive);
        }

        free_command(cmd);
    }

    if (input_file) fclose(input_file);

    if (interactive) {
        printf("Goodbye!\n");
    }

    return 0;
}