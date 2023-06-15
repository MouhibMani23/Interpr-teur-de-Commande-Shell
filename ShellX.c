#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>

#define clear() printf("\033[H\033[J")
#define BUF_SZ 256
#define TRUE 1
#define FALSE 0

// Constant
const char *COMMAND_EXIT = "quit";
const char *COMMAND_HELP = "help";
const char *COMMAND_CD = "cd";
const char *COMMAND_IN = "<";
const char *COMMAND_OUT = ">";
const char *COMMAND_PIPE = "|";
const char *COMMAND_HISTORY = "history";

// built-in status code
enum
{
    RESULT_NORMAL,
    ERROR_FORK,
    ERROR_COMMAND,
    ERROR_WRONG_PARAMETER,
    ERROR_MISS_PARAMETER,
    ERROR_TOO_MANY_PARAMETER,
    ERROR_CD,
    ERROR_SYSTEM,
    ERROR_EXIT,

    /* redirected error message */
    ERROR_MANY_IN,
    ERROR_MANY_OUT,
    ERROR_FILE_NOT_EXIST,

    /* Pipeline error message */
    ERROR_PIPE,
    ERROR_PIPE_MISS_PARAMETER
};

// Variable
char username[BUF_SZ];
char curPath[BUF_SZ];
char commands[BUF_SZ][BUF_SZ];

// Declaration of functions
int isCommandExist(const char *command);
void getUsername();
void init_shell();
int getCurWorkDir();
int splitCommands(char command[BUF_SZ]);
int callExit();
int callCommand(int commandNum);
int callCommandWithPipe(int left, int right);
int callCommandWithRedi(int left, int right);
int callCd(int commandNum);

// Main
void open_file(char *filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 1;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("ShellX: Failed to open file");
    }
    else
    {
        while ((read = getline(&line, &len, fp)) != -1)
        {
            printf("%d- %s", i, line);
            i++;
        }

        fclose(fp);
        if (line)
            free(line);
    }
}

void append_file(char *fname, char *line)
{
    FILE *pFile;

    pFile = fopen(fname, "a");
    if (pFile == NULL)
    {
        perror("ShellX: Error opening file.");
    }
    else
    {
        fprintf(pFile, "%s\n", line);
    }
    fclose(pFile);
}
int main()
{
    /* Get current working directory, username, hostname */
    int result = getCurWorkDir();
    if (ERROR_SYSTEM == result)
    {
        fprintf(stderr, "\e[31;1mError: System error while getting current work directory.\n\e[0m");
        exit(ERROR_SYSTEM);
    }
    getUsername();

    init_shell();
    /* start myshell */
    char argv[BUF_SZ];
    while (TRUE)
    {
        printf("\e[31;1m%s\e[33;1m@~%s\e[0m%% ", username, curPath); // shown in red for username and yellow for current working directory
        /* Get the command entered by the user */
        fgets(argv, BUF_SZ, stdin);

        int len = strlen(argv);
        if (len != BUF_SZ)
        {
            argv[len - 1] = '\0'; // put EOW in the end of the file
        }
        int commandNum = splitCommands(argv);

        if (commandNum != 0)
        { // User has input command
            append_file("/tmp/.Xhistory", argv);
            if (strcmp(commands[0], COMMAND_EXIT) == 0)
            { // exit command
                result = callExit();
                if (ERROR_EXIT == result)
                {
                    exit(-1);
                }
            }
            else if (strcmp(commands[0], COMMAND_HISTORY) == 0)
            {
                open_file("/tmp/.Xhistory");
            }

            else if (strcmp(commands[0], COMMAND_CD) == 0)
            { // cd command
                result = callCd(commandNum);
                switch (result)
                {
                case ERROR_MISS_PARAMETER:
                    fprintf(stderr, "\e[31;1mError: Miss parameter while using command \"%s\".\n\e[0m", COMMAND_CD);
                    break;
                case ERROR_WRONG_PARAMETER:
                    fprintf(stderr, "\e[31;1mError: No such path \"%s\".\n\e[0m", commands[1]);
                    break;
                case ERROR_TOO_MANY_PARAMETER:
                    fprintf(stderr, "\e[31;1mError: Too many parameters while using command \"%s\".\n\e[0m", COMMAND_CD);
                    break;
                case RESULT_NORMAL: // The cd command is executed normally, and the current working l directory is updated
                    result = getCurWorkDir();
                    if (ERROR_SYSTEM == result)
                    {
                        fprintf(stderr, "\e[31;1mError: System error while getting current work directory.\n\e[0m");
                        exit(ERROR_SYSTEM);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            { // other commands
                result = callCommand(commandNum);
                switch (result)
                {
                case ERROR_FORK:
                    fprintf(stderr, "\e[31;1mError: Fork error.\n\e[0m");
                    exit(ERROR_FORK);
                case ERROR_COMMAND:
                    fprintf(stderr, "\e[31;1mError: Command does not exist.\n\e[0m");
                    break;
                case ERROR_MANY_IN:
                    fprintf(stderr, "\e[31;1mError: Too many redirection symbol \"%s\".\n\e[0m", COMMAND_IN);
                    break;
                case ERROR_MANY_OUT:
                    fprintf(stderr, "\e[31;1mError: Too many redirection symbol \"%s\".\n\e[0m", COMMAND_OUT);
                    break;
                case ERROR_FILE_NOT_EXIST:
                    fprintf(stderr, "\e[31;1mError: Input redirection file not exist.\n\e[0m");
                    break;
                case ERROR_MISS_PARAMETER:
                    fprintf(stderr, "\e[31;1mError: Miss redirect file parameters.\n\e[0m");
                    break;
                case ERROR_PIPE:
                    fprintf(stderr, "\e[31;1mError: Open pipe error.\n\e[0m");
                    break;
                case ERROR_PIPE_MISS_PARAMETER:
                    fprintf(stderr, "\e[31;1mError: Miss pipe parameters.\n\e[0m");
                    break;
                }
            }
        }
    }
}

int isCommandExist(const char *command)
{ // Determine whether the command exists
    if (command == NULL || strlen(command) == 0)
        return FALSE;

    int result = TRUE;

    int fds[2];
    if (pipe(fds) == -1)
    {
        result = FALSE;
    }
    else
    {
        /* Temporary input and output redirection flags */
        int inFd = dup(STDIN_FILENO);
        int outFd = dup(STDOUT_FILENO);

        pid_t pid = vfork();
        if (pid == -1)
        {
            result = FALSE;
        }
        else if (pid == 0)
        {
            /* redirect resulting output to a file identifier */
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);

            char tmp[BUF_SZ];
            sprintf(tmp, "command -v %s", command);
            system(tmp);
            exit(1);
        }
        else
        {
            waitpid(pid, NULL, 0);
            /* input redirection */
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);

            if (getchar() == EOF)
            { // No data, means the command does not exist
                result = FALSE;
            }

            /* resume input, output redirection */
            dup2(inFd, STDIN_FILENO);
            dup2(outFd, STDOUT_FILENO);
        }
    }

    return result;
}

void getUsername()
{ // Get the currently logged in username
    struct passwd *pwd = getpwuid(getuid());
    strcpy(username, pwd->pw_name);
}

void init_shell()
{
    sleep(1);
    clear();
    printf("\t*******************************************************************************************************");
    printf("\n\t\t\t\t\t****THIS SHELL WAS MADE BY****");
    printf("\n\t\t\t\t\t\t-Mouhib Mani-");
    printf("\n\t\t\t\t\t\t-Aziz Fezzani-");
    printf("\n\t\t\t\t\t\t-Haithem Nefeti-");
    printf("\n\n");
    printf("\t\t             ___        ");
    printf("\n\t\t          .'`__ '.       .--------.__     ");
    printf("\n\t\t         /  /  \\  \\      |::::::::|[_I___,");
    printf("\n\t\t         \\  `-'/  /--.   |_.-.____I__.-~;|");
    printf("\n\t\t         /'-=-'` ( _'')   `(_)--------(_) ");
    printf("\n\t\t~~~~~~~  `"
           ""
           ""
           ""
           "`     ");
    printf("\n");
    printf("\n\t******************************************************************************************************");

    printf("\n");
    sleep(1);
}

int getCurWorkDir()
{ // Get the current working directory
    char *result = getcwd(curPath, BUF_SZ);
    if (result == NULL)
        return ERROR_SYSTEM;
    else
        return RESULT_NORMAL;
}

int splitCommands(char command[BUF_SZ])
{ // Split the command with spaces, and return the number of split strings
    int num = 0;
    int i, j;
    int len = strlen(command);

    for (i = 0, j = 0; i < len; ++i)
    {
        if (command[i] != ' ')
        {
            commands[num][j++] = command[i];
        }
        else
        {
            if (j != 0)
            {
                commands[num][j] = '\0';
                ++num;
                j = 0;
            }
        }
    }
    if (j != 0)
    {
        commands[num][j] = '\0';
        ++num;
    }

    return num;
}

int callExit()
{ // Send a terminal signal to exit the process
    pid_t pid = getpid();
    if (kill(pid, SIGTERM) == -1)
        return ERROR_EXIT;
    else
        return RESULT_NORMAL;
}

int callCommand(int commandNum)
{ // A function used by the user to execute the command entered by the user
    pid_t pid = fork();
    if (pid == -1)
    {
        return ERROR_FORK;
    }
    else if (pid == 0)
    {
        /* Get file identifiers for standard input and output */
        int inFds = dup(STDIN_FILENO);
        int outFds = dup(STDOUT_FILENO);

        int result = callCommandWithPipe(0, commandNum);

        /* restore standard input, output redirection */
        dup2(inFds, STDIN_FILENO);
        dup2(outFds, STDOUT_FILENO);
        exit(result);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

int callCommandWithPipe(int left, int right)
{ // The instruction range [left, right) to be executed may contain pipelines
    if (left >= right)
        return RESULT_NORMAL;
    /* Determine whether there is a pipeline command */
    int pipeIdx = -1;
    for (int i = left; i < right; ++i)
    {
        if (strcmp(commands[i], COMMAND_PIPE) == 0)
        {
            pipeIdx = i;
            break;
        }
    }
    if (pipeIdx == -1)
    { // Does not contain pipeline commands
        return callCommandWithRedi(left, right);
    }
    else if (pipeIdx + 1 == right)
    { // Pipeline command '|' is followed by no instruction, parameter is missing
        return ERROR_PIPE_MISS_PARAMETER;
    }

    /* Excuting an order */
    int fds[2];
    if (pipe(fds) == -1)
    {
        return ERROR_PIPE;
    }
    int result = RESULT_NORMAL;
    pid_t pid = vfork();
    if (pid == -1)
    {
        result = ERROR_FORK;
    }
    else if (pid == 0)
    {                                // Subprocess executes a single command
        close(fds[0]);               // close the reading side of the pipe
        dup2(fds[1], STDOUT_FILENO); // redirect standard output to fds[1]
        close(fds[1]);               // close the writing side of the pipe

        result = callCommandWithRedi(left, pipeIdx);
        exit(result); // Value returns to the calling process (the parent process)
    }
    else
    { // The parent process recursively executes subsequent commands
        int status;
        waitpid(pid, &status, 0);
        int exitCode = WEXITSTATUS(status);

        if (exitCode != RESULT_NORMAL)
        { // The command of the child process did not exit normally, and an error message was printed
            char info[4096] = {0};
            char line[BUF_SZ];
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // redirect standard input to fds[0]
            close(fds[0]);
            while (fgets(line, BUF_SZ, stdin) != NULL)
            { // Read the error message of the child process
                strcat(info, line);
            }
            printf("%s", info); // print error message

            result = exitCode;
        }
        else if (pipeIdx + 1 < right)
        {
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // redirect standard input to fds[0]
            close(fds[0]);
            result = callCommandWithPipe(pipeIdx + 1, right); // Recursively execute subsequent instructions
        }
    }

    return result;
}

int callCommandWithRedi(int left, int right)
{ // The range of instructions to be executed [left, right), without pipelines, may contain redirection
    if (!isCommandExist(commands[left]))
    { // command does not exist
        return ERROR_COMMAND;
    }

    /* Determine whether there is a redirection */
    int inNum = 0, outNum = 0;
    char *inFile = NULL, *outFile = NULL;
    int endIdx = right; // The command's termination index before redirection

    for (int i = left; i < right; ++i)
    {
        if (strcmp(commands[i], COMMAND_IN) == 0)
        { // input redirection <
            ++inNum;
            if (i + 1 < right)
                inFile = commands[i + 1];
            else
                return ERROR_MISS_PARAMETER; // Missing filename after redirect symbol

            if (endIdx == right)
                endIdx = i;
        }
        else if (strcmp(commands[i], COMMAND_OUT) == 0)
        { // output redirection >
            ++outNum;
            if (i + 1 < right)
                outFile = commands[i + 1];
            else
                return ERROR_MISS_PARAMETER; // Missing filename after redirect symbol

            if (endIdx == right)
                endIdx = i;
        }
    }
    /* Handle redirects */
    if (inNum == 1)
    {
        FILE *fp = fopen(inFile, "r");
        if (fp == NULL) // input redirection file does not exist
            return ERROR_FILE_NOT_EXIST;

        fclose(fp);
    }

    if (inNum > 1)
    { // input redirector more than one
        return ERROR_MANY_IN;
    }
    else if (outNum > 1)
    { // more than one output redirector
        return ERROR_MANY_OUT;
    }

    int result = RESULT_NORMAL;
    pid_t pid = vfork();
    if (pid == -1)
    {
        result = ERROR_FORK;
    }
    else if (pid == 0)
    {
        /* Input and output redirection */
        if (inNum == 1)
            freopen(inFile, "r", stdin);
        if (outNum == 1)
            freopen(outFile, "w", stdout);

        /* Excuting an order */
        char *comm[BUF_SZ];
        for (int i = left; i < endIdx; ++i)
            comm[i] = commands[i];
        comm[endIdx] = NULL;
        execvp(comm[left], comm + left);
        exit(errno); // Execution error, return errno
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        int err = WEXITSTATUS(status); // Read the return code of the child process

        if (err)
        { // If the return code is not 0, it means that the child process has an error, and the error message is printed in red font
            printf("\e[31;1mError: %s\n\e[0m", strerror(err));
        }
    }

    return result;
}

int callCd(int commandNum)
{ // Execute the cd command
    int result = RESULT_NORMAL;

    if (commandNum < 2)
    {
        result = ERROR_MISS_PARAMETER;
    }
    else if (commandNum > 2)
    {
        result = ERROR_TOO_MANY_PARAMETER;
    }
    else
    {
        int ret = chdir(commands[1]);
        if (ret)
            result = ERROR_WRONG_PARAMETER;
    }

    return result;
}