#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TSH_TOK_BUFSIZE 64
#define TSH_TOK_DELIM " \t\r\n\a"

int tshcd(char **args);
int tshexit(char **args);

char *builtin_str[] = {
    "tcd",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &tshcd,
    &tshexit
};

int tshnums() {
    return sizeof(builtin_str) / sizeof(char *);
}

int tshcd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "tsh: expected argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("tsh");
        }
    }
    return 1;
}

int tshexit(char **args) {
    return 0;

}

int list() {
    
}

char *tshread() {
    char *line = NULL;
    ssize_t buffersize = 0;

    if (getline(&line, &buffersize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

// Splits the input line into arguments
char **tshline(char *line) {
    int buffersize = TSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(buffersize * sizeof(char *));
    char *token;

    if (!tokens) {
        fprintf(stderr, "tsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TSH_TOK_DELIM);

    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= buffersize) {
            buffersize += TSH_TOK_BUFSIZE;
            tokens = realloc(tokens, buffersize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "tsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TSH_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

int launch(char **args) {
    pid_t pid = fork();
    int state;

    if (pid == 0) {
        // This is the child process
        if (execvp(args[0], args) == -1) {
            perror("tsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork failed");
    } else {
        do {
            waitpid(pid, &state, WUNTRACED);
        } while (!WIFEXITED(state) && !WIFSIGNALED(state));
    }

    return 1;
}

int execute(char **args) {
    if (args[0] == NULL) {
        return 1;
    }

    for (int i = 0; i < tshnums(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return launch(args);

}

void tsh_loop(void) {
    char *line;
    char **args;
    int state;

    printf("Welcome to TSH (Twopic Shell)!\n");
    printf(
        "/__  ___/ //   ) )  //    / / \n"
        "   / /    ((        //___ / /  \n"
        "  / /       \\     / ___   /   \n"
        " / /          ) ) //    / /    \n"
        "/ /    ((___ / / //    / /     \n"
        
    );
    
    printf("\n");

    do {
        printf("$ ");
        line = tshread();
        args = tshline(line);
        state = execute(args);

        free(line);
        free(args);
    } while (state);
}

int main(int argc, char **argv) {
    tsh_loop();
    return EXIT_SUCCESS;
}
