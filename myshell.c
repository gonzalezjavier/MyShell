/****************************************************************
 * Name        :                                                *
 * Class       :  CSC 415                                       *
 * Date        :                                                *
 * Description :  Writting a simple shell program               *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

/* CANNOT BE CHANGED */
#define BUFFERSIZE 256
/* --------------------*/
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT)
#define FOREVER for(;;)
#define DELIMS " \t\n"

int main(int myargc, char** myargv)
{
    char buffer[BUFFERSIZE];
    char cwd[BUFFERSIZE];
    int arg_count;
    char* arg_vector[32];
    char* temp_arg;
    int pipe_flag;
    int bg_flag;
    int over_out;
    int app_out;
    int read_in;
    
    
    FOREVER
    {
        //resets values for next loop
        pipe_flag=0;
        bg_flag=0;
        over_out=0;
        app_out=0;
        read_in=0;
        //Prints the prompt and reads from user
        printf("%s ", PROMPT);
        if (!fgets(&buffer[0], BUFFERSIZE, stdin))
        {
            perror("error reading from user");
            exit(-1);
        }
        printf("\n");
        
        //this parses the buffer and stores the tokens into the arg_vector
        //this also checks if a pipe, redirection out, or input from file was called for
        arg_count=0;
        temp_arg = strtok(buffer, DELIMS);
        while (temp_arg != NULL)
        {
            if (strcmp(temp_arg, "|")==0) {
                arg_vector[arg_count++] = NULL;
                temp_arg = strtok(NULL, DELIMS);
                pipe_flag = arg_count;
                continue;
            } else if(strcmp(temp_arg, ">>")==0){
                //append
                arg_vector[arg_count++] = NULL;
                temp_arg = strtok(NULL, DELIMS);
                app_out=arg_count;
                continue;
            } else if(strcmp(temp_arg, ">")==0){
                //overwrite
                arg_vector[arg_count++] = NULL;
                temp_arg = strtok(NULL, DELIMS);
                over_out=arg_count;
                continue;
            } else if(strcmp(temp_arg, "<")==0){
                //take input from file
                arg_vector[arg_count++] = NULL;
                temp_arg = strtok(NULL, DELIMS);
                read_in=arg_count;
                continue;
            }
            arg_vector[arg_count++] = temp_arg;
            temp_arg = strtok(NULL, DELIMS);
        }
        arg_vector[arg_count] = NULL;
        
        //this checks if the input was pwd and skips the rest
        if (arg_count==1 && strcmp(arg_vector[0],"pwd")==0) {
            printf("%s\n",getcwd(cwd, sizeof(cwd)));
            continue;
        }
        
        
        //this changes the directory and skips the rest
        if (strcmp(arg_vector[0],"cd")==0 && arg_count==2) {
            chdir(arg_vector[1]);
            continue;
        }
        
        
        //this checks to run in bg or not
        if (strcmp(arg_vector[arg_count-1], "&")==0) {
            arg_vector[arg_count-1]=NULL;
            arg_count--;
            bg_flag++;
        }
        
        
        //creating the id for forking
        pid_t id;
        
        //this will complete the executions for append file
        if (app_out>0) {
            id = fork();
            if(id > 0) {
                //parent
                if (bg_flag!=1) {
                    wait(NULL);
                }
            } else if (id == 0) {
                //child
                int fd = open(arg_vector[app_out], O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
                close(1);
                dup(fd);
                execvp(arg_vector[0], &arg_vector[0]);
                perror("");
                exit(0);
            } else {
                perror("The fork process has failed\n");
                exit(-1);
            }
            
            
        } //for overwrite file
        else if(over_out>0) {
            id = fork();
            if(id > 0) {
                //parent
                if (bg_flag!=1) {
                    wait(NULL);
                }
            } else if (id == 0) {
                //child
                int fd = creat(arg_vector[over_out], S_IRUSR | S_IWUSR);
                close(1);
                dup(fd);
                execvp(arg_vector[0], &arg_vector[0]);
                perror("");
                exit(0);
            } else {
                perror("The fork process has failed\n");
                exit(-1);
            }
            
            
        } //reading from file
        else if(read_in > 0) {
            id = fork();
            if(id > 0) {
                //parent
                if (bg_flag!=1) {
                    wait(NULL);
                }
            } else if (id == 0) {
                //child
                int fd = open(arg_vector[read_in], O_RDONLY, S_IRUSR);
                close(0);
                dup(fd);
                execvp(arg_vector[0], &arg_vector[0]);
                perror("");
                exit(0);
            } else {
                perror("The fork process has failed\n");
                exit(-1);
            }
        }
        //for pipes
        else if (pipe_flag > 0) {
            int pipe_fds[2];
            int return_val = pipe(pipe_fds);
            if (return_val == -1) {
                perror("Pipe was not made");
                exit(-1);
            }

            id = fork();
            if (id == 0) {
                //child
                close(0);
                dup(pipe_fds[0]);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
                execvp(arg_vector[pipe_flag], &arg_vector[pipe_flag]);
                perror("");
                exit(0);
            } else if(id > 0) {
                //parent
                close(1);
                dup(pipe_fds[1]);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
                execvp(arg_vector[0], &arg_vector[0]);
                
            } else {
                perror("The fork process has failed\n");
                exit(-1);
            }
        }
        //this completes the normal execvp()
        else {
            id = fork();
            if(id > 0) {
                //parent
                if (bg_flag!=1) {
                    wait(NULL);
                }
            } else if (id == 0) {
                //child
                execvp(arg_vector[0], &arg_vector[0]);
                perror("");
                exit(0);
            } else {
                perror("The fork process has failed\n");
                exit(-1);
            }
        }
    }
    return 0;
}

