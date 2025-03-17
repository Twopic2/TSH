#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>  
#include <pwd.h>       
#include <grp.h>       
#include <time.h>      

#define TSH_TOK_BUFSIZE 64
#define TSH_TOK_DELIM " \t\r\n\a"


//Prototypes
int tshcd(char **args);
int tshexit(char **args);
int list(char **args);

char *builtin_str[] = {
    "tcd",
    "exit",
    "tls"
};

int (*builtin_func[]) (char **) = {
    &tshcd,
    &tshexit,
    &list
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

int list(char **args) {

    DIR *dir;
    struct dirent *entry;

    dir = opendir(".");
    int dashL = 0;

    // shows permissions 
    if (args[1] && strcmp(args[1], "-l") == 0) {
        dashL = 1;

    }

    if (dir == NULL) {
        perror("TSH ls stopped");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // -a shows hidden files starting with .
        if (args[1] && strcmp(args[1], "-a") == 0) {
            printf("%s\n", entry->d_name);
        } 
        if (dashL) {

            struct stat file_stat;

            if (stat(entry->d_name, &file_stat)) {
                perror("tsh: tls");
                continue;
            }

            printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            printf("%s%s%s%s%s%s%s%s%s",
                (file_stat.st_mode & S_IRUSR) ? "r" : "-",
                (file_stat.st_mode & S_IWUSR) ? "w" : "-",
                (file_stat.st_mode & S_IXUSR) ? "x" : "-",
                (file_stat.st_mode & S_IRGRP) ? "r" : "-",
                (file_stat.st_mode & S_IWGRP) ? "w" : "-",
                (file_stat.st_mode & S_IXGRP) ? "x" : "-",
                (file_stat.st_mode & S_IROTH) ? "r" : "-",
                (file_stat.st_mode & S_IWOTH) ? "w" : "-",
                (file_stat.st_mode & S_IXOTH) ? "x" : "-");

            // Details
            printf(" %ld %s %s %5lld ",
                   (long)file_stat.st_nlink,
                   getpwuid(file_stat.st_uid)->pw_name,
                   getgrgid(file_stat.st_gid)->gr_name,
                   (long long)file_stat.st_size);

            // Time
            char time_buf[80];
            printf("%s %s\n", time_buf, entry->d_name);
        } else {
            printf("%s\n", entry->d_name);

        }

    }

    closedir(dir);
    return 1;
    
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
