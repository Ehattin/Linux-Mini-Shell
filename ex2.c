#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <assert.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#define SIZE 510

int flag_redirect = 0, redic = 0;
int flag_pipe = 0, pipec = 0;
int type = 0;
int command_num = 0; //counter for total amount of commands

void write_to_log(const char *input, const char *path);
void error(char *msg);
char *remove_space(char *command);
void exec_pipe(char **pre, char **post, char *path);
char **make_string(char *user_input);
int string_checker(char *str, char **splitString);
void redirection(char **command, char *str, int type);

int main(int argc, char *argv[])
{

    int num_of_arguments = 0;  //counts how many arguments are given by user
    int total_command_len = 0; //total length of all user input commands
    char user_input[SIZE];     //saves current user input
    char copy[SIZE];           //copy char array for using strtok
    char buffer[SIZE];         //for getting user id
    char *temp = NULL;         //temp for strtok
    char *path = NULL;
    char **post = NULL; //array to contain commands before the pipe
    char **pre = NULL;
    char **command = NULL; //array to contain commands after the pipe
    char **string;         //used for saving all of the rows into a single double array

    getcwd(buffer, SIZE);
    struct passwd *user_id;
    user_id = getpwuid(getuid());

    while (strcmp("done", user_input) != 0)
    {
        flag_redirect = 0;
        flag_pipe = 0;
        printf("%s@%s>", user_id->pw_name, buffer); //print current username to screen
        fgets(user_input, SIZE, stdin);             //get user input
        if (strcmp(user_input, "done") == 1)        //this is how we terminate the shell
            break;
        user_input[strlen(user_input) - 1] = '\0';
        if (user_input[0] == '\0') //if current command is only space, return to main loop
            continue;
        if (user_input == NULL) // make sure that the current input isn't null
            continue;

        total_command_len += strlen(user_input); //counts the total length of the argument
        command_num++;
        //counts the total command number (not the argument number)
        strcpy(copy, user_input);
        temp = strtok(copy, "cd");
        if (strcmp(temp, "cd") == 1)
        {
            printf("command not supported (yet)\n");
            continue;
        }
        char **splitString = (char **)malloc(3 * sizeof(char *)); //memory alloc for string in the size of the words in copy
        assert(splitString);
        for (size_t i = 0; i < 3; i++)
        {
            splitString[i] = (char *)malloc(strlen(user_input) * sizeof(char));
            assert(splitString[i]);
        }

        //check to see if this causes a problem
        pid_t son;
        switch (string_checker(user_input, splitString))
        {

        case 1:
            /* no pipe and yes redirect */

            if ((son = fork()) < 0)
                error("fork failed\n");
            if (son == 0)
            {
                command = make_string(splitString[0]);
                path = remove_space(splitString[1]);
                redirection(command, path, type);
            }
            wait(NULL);
            break;

        case 2:
            /* yes pipe and no redirection */
            pre = make_string(splitString[0]);
            post = make_string(splitString[1]);
            exec_pipe(pre, post, NULL);
            break;

        case 3:
            /* yes to both */
            pre = make_string(splitString[0]);
            post = make_string(splitString[1]);
            path = remove_space(splitString[2]);
            exec_pipe(pre, post, path);
            break;

        default:
            num_of_arguments = 0;
            string = make_string(user_input);
            int i = 0;
            while (string[i] != NULL)
            {
                num_of_arguments++;
                i++;
            }
            __pid_t process = fork();
            if (process == -1) //make sure that the fork worked
            {
                perror("fork failed");
                exit(1);
            }
            if (process == 0) //this is the child
            {
                execvp(string[0], string);
                exit(0);
            }
            wait(NULL);

            //this is the parent
            //we need to free the memory
            for (i = 0; i < num_of_arguments; i++)
                free(string[i]);

            free(string);
            //break;
        }
    }
    printf("Num of commands:%d\n", command_num);
    printf("Total length of all commands:%d\n", total_command_len);
    printf("Average length of all commands:%f\n", ((double)(total_command_len) / command_num));
    printf("number of commands that include pipe: %d\n", pipec);
    printf("number of commands that include redirection: %d\n", redic);
    printf("See you next time !\n");
    return 0;
}

void write_to_log(const char *input, const char *path)
{
    int saved_stdout = dup(1);
    int fd = open(path, O_RDWR | O_CREAT | O_APPEND, S_IRWXG | S_IRWXO | S_IRWXU);
    if (fd < 0)
        error("Open Failed!\n");

    int value = dup2(fd, STDOUT_FILENO);
    if (value < 0)
        error("Dup Failed!\n");
    fprintf(stdout, "%s\n", input);

    close(fd);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}
//error function
void error(char *msg)
{
    perror(msg);
    exit(1);
}

// function for removing spaces from command
char *remove_space(char *command)
{
    char *tok = strtok(command, " ");
    return tok;
}
// Function where the piped system commands are executed
void exec_pipe(char **pre, char **post, char *path)
{
    printf("%d\n", pipec);
    pipec++;
    printf("%d\n", pipec);
    int i, j, countpre, countpost;
    i = 0;
    j = 0;
    countpost = 0;
    countpre = 0;
    while (pre[i] != NULL)
    {
        countpre++;
        i++;
    }
    while (pre[j] != NULL)
    {
        countpost++;
        j++;
    }

    // 0 is read end, 1 is write end
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0)
    {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0)
    {
        printf("\nCould not fork");
        return;
    }

    if (p1 == 0)
    {
        // Child 1 executing..
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(pre[0], pre) < 0)
        {
            printf("\nCould not execute command 1..");
            exit(0);
        }
        exit(0);
    }

    // Parent executing
    p2 = fork();

    if (p2 < 0)
    {
        printf("\nCould not fork");
        return;
    }

    // Child 2 executing..
    // It only needs to read at the read end
    if (p2 == 0)
    {
        if (!flag_redirect)
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(post[0], post) < 0)
            {
                printf("\nCould not execute command 2..");
                exit(0);
            }
            exit(0);
        }
        redirection(post, path, type);
        exit(0);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    // parent executing, waiting for two children
    wait(NULL);
    wait(NULL);
    //free all memory from father
    for (i = 0; i < countpre; i++)
        free(pre[i]);
    free(pre);
    for (j = 0; j < countpost; j++)
        free(post[j]);
    free(pre);
    if (path != NULL)
        free(path);
}

char **make_string(char *user_input)
{
    int i = 0;
    char copy[strlen(user_input)];
    char **string;
    char *temp;
    int num_of_args = 0;
    strcpy(copy, user_input); //we copy user_input because strtok changes the string its working on
    temp = strtok(user_input, " ");

    while (temp != NULL)
    {
        num_of_args++;            //counts the arguments  separated by " " in copy
        temp = strtok(NULL, " "); //breaks string copy into a series of arguments separated by " " (space)
    }

    string = (char **)malloc((num_of_args + 1) * sizeof(char *)); //memory alloc for string in the size of the words in copy
    assert(string);

    temp = strtok(copy, " ");
    while (temp != NULL)
    {
        string[i] = (char *)malloc((strlen(temp) + 1) * sizeof(char)); //allocate memory for each seperate string in **string
        assert(string[i]);
        strcpy(string[i], temp); //copy each string to **string
        temp = strtok(NULL, " ");
        i++;
    }

    string[i] = NULL; //put NULL in the end of string
    return string;
}

int string_checker(char *str, char **splitString)
{
    char arr_check[4] = {'<', '>', '>', '2'};
    int i = 0, j = 0, checker = 0,
        k = 0, l = 0, m = 0;

    while (j < strlen(str))
    {
        if (!flag_redirect)
        {
            for (i = 0; i < 4; i++)
            {
                if ((str[j] == arr_check[i]) && (str[j + 1] == '>'))
                {
                    flag_redirect = 1;
                    if (str[j] == '>')
                        i++;
                    j += 2;
                    type = i;
                    break;
                }
                else if ((str[j] == arr_check[i]) && (str[j + 1] != '>'))
                {
                    flag_redirect = 1;
                    type = i;
                    j++;
                    break;
                }
            }
            if (flag_redirect)
                continue;
        }
        if (!flag_pipe)
        {
            if (str[j] == '|')
            {
                flag_pipe = 1;
                j++;
                continue;
            }
        }
        if (!flag_redirect && !flag_pipe)
        {
            splitString[0][k] = str[j];
            j++;
            k++;
            continue;
        }
        if (!flag_pipe && flag_redirect)
        {
            splitString[1][l] = str[j];
            l++;
            checker = 1;
            j++;
            continue;
        }
        if (flag_pipe && !flag_redirect)
        {
            splitString[1][l] = str[j];
            l++;
            checker = 2;
            j++;
            continue;
        }
        if (flag_pipe && flag_redirect)
        {
            splitString[2][m] = str[j];
            m++;
            checker = 3;
            j++;
            continue;
        }
    }
    return checker; // returns checker which determines the action we will do based on found arguments.
}

void redirection(char **command, char *str, int type)
{
    int fd = 0, val = 0;
    printf("%d\n", redic);
    redic++;
    printf("%d\n", redic);
    switch (type)
    //{"<", ">", ">>", "2>"}
    {
    case 0: // CASE OF <

        if ((fd = open(str, O_RDWR | O_CREAT | O_APPEND, 0777)) < 0)
            error("Open has Failed!\n");

        if ((val = dup2(fd, STDIN_FILENO)) < 0)
            error("Dup has Failed!\n");
        close(fd);
        if (execvp(command[0], command) < 0)
            error("Execvp has Failed!\n");
        break;
    case 1: // CASE OF >
        if ((fd = open(str, O_RDWR | O_CREAT | O_TRUNC,0777)) < 0)
            error("Open has Failed!\n");
        if ((val = dup2(fd, STDOUT_FILENO)) < 0)
            error("Dup Failed!\n");
        close(fd);
        if (execvp(command[0], command) < 0)
            error("Execvp Failed!\n");
        break;
    case 2: // CASE OF >>

        if ((fd = open(str, O_RDWR | O_CREAT | O_APPEND,0777)) < 0)
            error("Open Failed!\n");

        if ((val = dup2(fd, STDOUT_FILENO)) < 0)
            error("Dup Failed!\n");
        close(fd);
        if (execvp(command[0], command) < 0){
            error("Execvp Failed!\n");}
        break;

    case 3: // CASE OF 2>

        if ((fd = open(str, O_RDWR | O_CREAT | O_APPEND,0777)) < 0)
            error("Open Failed!\n");
        if ((val = dup2(fd, STDERR_FILENO)) < 0){
            error("Dup has Failed!\n");}
        close(fd);
        if (execvp(command[0], command) < 0){
            perror("Execvp Failed!\n");}
        break;
    default:
     break;
    }
}