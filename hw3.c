/* CS361 Homework 3: Write your own shell
 * Name:   Shyam Patel
 * NetID:  spate54
 * Date:   Oct 15, 2018
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>


// redirect output to path and create it if it doesn't exist
void redirectOut(const char *pathname) {
    int fd = 0;                                 // file descriptor
    // creat() is equivalent to open() with flags O_CREAT|O_WRONLY|O_TRUNC
    //   O_CREAT  : create pathname if it doesn't exist
    //   O_WRONLY : write only access mode
    //   O_TRUNC  : truncate to length 0
    // i.e., calling creat(pathname, S_IRWXU) is the same as calling
    //   open(pathname, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU)
    // mode argument to permit user to access newly created output path
    //   S_IRWXU  : user   has  read, write and execute permission
    if ((fd = creat(pathname, S_IRWXU)) < 0) {  // create output path
        perror("Unable to write to the output path");
        exit(EXIT_FAILURE);
    }
    dup2(fd, 1);                                // duplicate fd to stdout (1)
    close(fd);                                  // close fd
}//end redirectOut()


// redirect input to path
void redirectIn(const char *pathname) {
    int fd = 0;                                 // file descriptor
    // flag O_RDONLY : read only access mode
    if ((fd = open(pathname, O_RDONLY)) < 0) {  // open input path
        perror("Unable to read from the input path");
        exit(EXIT_FAILURE);
    }
    dup2(fd, 0);                                // duplicate fd to stdin (0)
    close(fd);                                  // close fd
}//end redirectIn()


// create child process to execute arguments and report its status
void run(char *args[]) {
    int wstatus = 0;            // wstatus
    pid_t cpid;                 // child process ID
    if ((cpid = fork()) < 0) {  // create child process
        perror("Unable to create child process");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0)              // child process :
        execvp(args[0], args);  //   execute args
    else                        // parent process :
        wait(&wstatus);         //   wait for child to terminate
        // same as calling waitpid(-1, &wstatus, 0)

    redirectOut("/dev/tty");    // redirect output to console
    redirectIn("/dev/tty");     // redirect input  to console
    printf("pid:%d status:%d\n", cpid, WEXITSTATUS(wstatus));
}//end run()


// process command from input string : allow for input and output
// redirections and create array of arguments to execute
void process(const char *str) {
    char *tokens = (char*) malloc(600 * sizeof(char));  // allocate tokens str
    int i = 0, j = 0;                                   // indexes i, j

    for (i = 0; i < strlen(str); i++) {                 // iterate thru str :
        if (str[i] == '>' || str[i] == '<') {
            tokens[j++] = ' ';                          //   add leading space
            tokens[j++] = str[i];                       //   add > or <
            tokens[j++] = ' ';                          //   add trailing space
        }
        else
            tokens[j++] = str[i];                       //   add arg
    }//end for...
    tokens[j] = '\0';                                   // append null terminator
    i = 0, j = 0;                                       // reset indexes i, j

    char **args = (char**) malloc(20 * sizeof(char*));  // allocate args array
    char *arg   = strtok(tokens, " \t\r\n\f\v");        // get 1st arg
    while (arg) {                                       // iterate thru args :
        if (*arg == '>')
            redirectOut(strtok(NULL, " \t\r\n\f\v"));   //   > : redirect output
        else if (*arg == '<')
            redirectIn(strtok(NULL, " \t\r\n\f\v"));    //   < : redirect input
        else {
            args[j] = (char*) malloc(strlen(arg) + 1);  //   allocate element
            strcpy(args[j++], arg);                     //   copy arg to array
        }
        arg = strtok(NULL, " \t\r\n\f\v");              //   get next arg
    }
    args[j] = '\0';                                     // append null terminator

    if (strcmp(args[0], "exit") == 0)                   // user requested exit :
        exit(EXIT_SUCCESS);                             //   exit success

    run(args);                                          // execute command

    for (i = 0; args[i] != '\0'; i++)                   // iterate thru args :
        free(args[i]);                                  //   deallocate arg

    free(args);                                         // deallocate args
    free(tokens);                                       // deallocate tokens
}//end process()


// signal handler function for SIGINT (Ctrl-C)
void sigint_handler(int sig) {
    char msg[] = "\ncaught sigint\nCS361 > ";
    write(1, msg, sizeof(msg));
}//end sigint_handler()


// signal handler function for SIGTSTP (Ctrl-Z)
void sigtstp_handler(int sig) {
    char msg[] = "\ncaught sigtstp\nCS361 > ";
    write(1, msg, sizeof(msg));
}//end sigtstp_handler()


// main function for shell
int main() {
    signal(SIGINT,   sigint_handler);       // handle SIGINT  (Ctrl-C)
    signal(SIGTSTP, sigtstp_handler);       // handle SIGTSTP (Ctrl-Z)

    for (;;) {
        char buf[500];                      // command line buffer
        char *commands[50];                 // array of commands
        char *ptr = NULL;                   // pointer to buffer
        int i = 0, j = 0;                   // indexes i, j

        fflush(stdout);                     // flush standard out stream
        printf("CS361 > ");
        fgets(buf, sizeof(buf), stdin);     // read from standard in
        buf[strcspn(buf, "\r\n")] = '\0';   // remove trailing newline

        if (strcmp(buf, "exit") == 0)       // user requested exit :
            exit(EXIT_SUCCESS);             //   exit success

        while (ptr = strrchr(buf, ';')) {   // buffer has multiple commands :
            *ptr = '\0';                    //   replace semicolon with null
            commands[j++] = ptr + 1;        //   add command to array
        }
        commands[j] = buf;                  // add command to array

        for (i = j; i > -1; i--)            // iterate thru array backwards :
            process(commands[i]);           //   process command
    }//end loop

    return EXIT_SUCCESS;                    // exit success
}//end main()

