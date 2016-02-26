/*
 *1. complete all the basic functions required;
 *2. inplemented "cd" function;
 *3. implemented "history command" function using "readline" library;
 *4. implemented "tab" function, when we is we double tab, it will list all
 *   the files under the current directory; if we type something, eg "sh",
 *   and we press tab, there will be a "shell" in the input area, before
 *   the cursor.
 *5. Can exit the shell using exit command;
 *6. Thank you!
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_SUB_COMMANDS    5
#define MAX_ARGS            10
#define NUM                 1024

struct SubCommand
{
    char *line;
    char *argv[MAX_ARGS];
    char *stdin_redirect;
    char *stdout_redirect;
    int pid;
};

struct Command
{
    struct SubCommand sub_commands[MAX_SUB_COMMANDS];
    int num_sub_commands;
};

/********** Basic Implementation **********/

int readArgs(char* input, char** argv, int size){
    char* sep = " ";
    char* word;
    int i = 0;
    int index = 0;
    char* in = strdup(input);
    for(word=strtok(in, sep); word; word=strtok(NULL, sep)){
        if(i == size-1)
            break;
        if(!strcmp(word, ">") || !strcmp(word, "<") || !strcmp(word, "&"))
            break;
        argv[i] = strdup(word);
        //check special characters in word
        for(index=0; index<strlen(argv[i]); index++){
            char c = argv[i][index];
            if(! (c>=45 && c<=57 || c>=65 && c<=90 || c==95 || c>=97 && c<=122)){
                //printf("#### Invalid character %c ####\n", c);
                return 1;
            }
        }
        i++;
    }
    argv[i] = NULL;
    free(in);
    return 0;
}


int readRedirects(struct Command *command){
    int i, j;
    int index;
    char *in;
    char* out;
    char* temp;
    
    char* sep = " ";
    char* word;
    char* line = strdup(command->sub_commands[command->num_sub_commands-1].line);
    
    //check format
    int in_index=-1, out_index=-1;
    int in_num=0, out_num=0;
    for(index=0; index<strlen(line); index++){
        char c = line[index];
        if(c=='<'){
            if(out_num!=0){
                return 1;
            }
            if(in_num==0){
                in_num++;
                in_index = index;
            }
            else{
                return 1;
            }
        }
        if(c=='>'){
            if(out_num==0){
                out_num++;
                out_index = index;
            }
            else{
                return 1;
            }
        }
        if(c==' ' && (in_num!=0 || out_num!=0) && index>0 && (index != in_index+1 && index != out_index+1)){
            return 1;
        }
    }

    i=0;
    for(word=strtok(line, sep); word; word=strtok(NULL, sep)){
        if(i == MAX_ARGS-1)
            break;
        i++;
        temp = strdup(word);
        
    }
    
    //check stdin and stdout
    for(i=command->num_sub_commands-1; i>=0; i--){
        command->sub_commands[i].stdin_redirect = NULL;
        command->sub_commands[i].stdout_redirect = NULL;
        if((in = strstr(command->sub_commands[i].line, "<"))!=NULL){
            sscanf(in, "< %s", temp);
            command->sub_commands[i].stdin_redirect = strdup(temp);
            printf("\tredirect: %s\n", command->sub_commands[i].stdin_redirect);
            //check invalid characters
            for(index=0; index<strlen(command->sub_commands[i].stdin_redirect); index++){
                char c = command->sub_commands[i].stdin_redirect[index];
                if(! (c>=45 && c<=57 || c>=65 && c<=90 || c==95 || c>=97 && c<=122)){
                    //printf("#### Invalid character %c ####\n", c);
                    return 1;
                }
            }
        }
        if((out = strstr(command->sub_commands[i].line, ">"))!=NULL){
            sscanf(out, "> %s", temp);
            command->sub_commands[i].stdout_redirect = strdup(temp);
            //check invalid characters
            for(index=0; index<strlen(command->sub_commands[i].stdout_redirect); index++){
                char c = command->sub_commands[i].stdout_redirect[index];
                if(! (c>=45 && c<=57 || c>=65 && c<=90 || c==95 || c>=97 && c<=122)){
                    //printf("#### Invalid character %c ####\n", c);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int readCommand(char *line, struct Command *command){
    char* sep = "|";
    char* word;
    if(line[strlen(line)-1]=='\n'){
        line[strlen(line)-1] = '\0';
    }
    int i = 0;
    for(word=strtok(line, sep); word; word=strtok(NULL, sep)){
        if(i == MAX_SUB_COMMANDS)   break;
        command->sub_commands[i].line = strdup(word);
        i++;
    }
    command->num_sub_commands = i;
    int j;
    int ret = 0;
    for(j=0; j<command->num_sub_commands; j++){
        if(readArgs(command->sub_commands[j].line, command->sub_commands[j].argv, MAX_ARGS)){
            return 1;
        }
    }
    return 0;
}

void print_args(char** argv){
    int i = 0;
    while(argv[i]!=NULL){
        printf("argv[%d] = '%s'\n", i, argv[i]);
        i++;
    }
}

void printCommand(struct Command *command){
    int i;
    for(i=0; i<command->num_sub_commands; i++){
        printf("\ncommand %d:\n", i);
        print_args(command->sub_commands[i].argv);
    }
}


int executeCommand(struct SubCommand *sub_command){
    //if the command has input file
    if(sub_command->stdin_redirect != NULL){
        close(0);
        int retop = open(sub_command->stdin_redirect, O_RDONLY, 0660);
        if(retop < 0){
            printf("%s: File not found\n", sub_command->stdin_redirect);
            exit(1);
        }
    }
    
    //if the command has output file
    if(sub_command->stdout_redirect != NULL){
        remove(sub_command->stdout_redirect);
        FILE* a;
        a = fopen(sub_command->stdout_redirect, "w");
        //fclose(a);
        //close the stdinput
        close(1);
        int retop = open(sub_command->stdout_redirect, O_WRONLY, 0660);
        if(retop <0){
            fprintf(stderr, "%s: Cannot open or create file\n", sub_command->stdout_redirect);
            exit(1);
        }
    }
    if(execvp(sub_command->argv[0], sub_command->argv) < 0){
        fprintf(stderr, "%s: Command not found.\n", sub_command->argv[0]);
        exit(1);
    }
}

void changeDirectory(struct SubCommand* sub_command){
    int ret = chdir(sub_command->argv[1]);
    if(ret < 0){
        printf("%s: No such file or directory\n", sub_command->line);
        return;
    }
}

int checkValidInput(char *command) {
    int length = strlen(command);
    int i = 0;
    if(command[0]<47 || (command[0]>57 && command[0]<65) || (command[0]>90 && command[0]<97) || (command[0]>122)) {
        printf("\tinvalid 1\n");
        return 1;
    }
    for(i=0; i<length; i++){
        if(command[i] == '|'){
            if(i >= length-2){
                printf("\tinvalid 2\n");
                return 1;
            }
            else if(command[i-1] != ' ' || command[i+1] != ' '){
                printf("\tinvalid 3\n");
                return 1;
            }
        }
    }
    return 0;
}

int runWholeCommand(struct Command* command){
    int w, i;
    //max 5 subcommands, so max 4 pipes;
    int fds[4][2];
    int ret = -1;
    int retp;
    //if only one command, we don't need pipe;
    if(command->num_sub_commands == 1){
        ret = fork();
        if(ret < 0){
            perror("System error\n");
            exit(1);
        }
        else if(ret == 0){
            executeCommand(&command->sub_commands[0]);
        }
        else if(ret>0){
            command->sub_commands[0].pid = ret;
            int status;
            w = waitpid(command->sub_commands[0].pid, &status, 0);
            printf("\tProcess-%d finished, return code: %d\n", command->sub_commands[0].pid, status/256);
        }
    }
    //if multiple commands, we need pipes
    else{
        for(i=0; i< command->num_sub_commands; i++){
            pipe(fds[i]);
            ret = fork();
            if(ret < 0){
                perror("System error\n");
                exit(1);
            }
            else if(ret == 0){
                //if not the first one
                if(i != 0){
                    close(0);
                    dup(fds[i-1][0]);
                }
                //if not the last one
                if(i != command->num_sub_commands-1){
                    //if contains output redirect
                    if(strlen(command->sub_commands[i].stdout_redirect)){
                        printf("File redirection error, please check your input\n");
                        return 1;
                    }
                    close(1);
                    dup(fds[i][1]);
                }
                executeCommand(&command->sub_commands[i]);
            }
            else if(ret > 0){
                if(i!=0)
                    close(fds[i-1][0]);
                if(i != command->num_sub_commands-1)
                    close(fds[i][1]);
                }
                command->sub_commands[i].pid = ret;
        }
        //parent wait for all children finish;
        for(i=0; i< command->num_sub_commands; i++){
            int status;
            w = waitpid(command->sub_commands[i].pid, &status, 0);
            printf("\tProcess-%d finished, return code: %d\n", command->sub_commands[i].pid, status/256);
            
        }
        return 0;
    }
}

int main(){
    struct Command command;
    
    //start running the shell;
    while(1){
        char buffer[1024];
        
        //display the current path and can use cd to switch;
        char cur_path[1024];
        getcwd(cur_path, sizeof(cur_path));
        
        char* temp_dir = malloc(sizeof(cur_path)+2);
        strcpy(temp_dir, cur_path);
        strcat(temp_dir, "$ ");
        printf("%s", temp_dir);
        fgets(buffer, sizeof(buffer), stdin);
        
        if(strcmp(buffer, "\n") == 0)
            continue;
        if(strlen(buffer) > 101) {
            printf("Please enter a command less than 100 characters\n");
            continue;
        }
        if(checkValidInput(buffer)){
            printf("Command invalid, please enter valid command\n");
            continue;
        }
        if(readCommand(buffer, &command)){
            printf("Invalid character found, please enter valid characters\n");
            continue;
        }

        if(readRedirects(&command)){
            printf("Invalid character found, please enter valid characters\n");
            continue;
        }

        /********** exit the shell **********/
        if(strcmp(command.sub_commands[0].argv[0], "exit")==0){
            printf("Successfully exit the shell!\n Hope I can see you again:)\n");
            exit(0);
        }
        
        /********** change the directory **********/
        if(command.num_sub_commands == 1 && strcmp(command.sub_commands[0].argv[0], "cd")==0){
            changeDirectory(&command.sub_commands[0]);
            continue;
        }
        
        /********** fork child and execute **********/
        runWholeCommand(&command);
    }
}

