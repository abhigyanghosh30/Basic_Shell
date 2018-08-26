#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <error.h>


#define MAXLINE 1024
#define MAXARGS 128

struct command
{
    int argc;               //count of arguments
    char *argv[MAXARGS];    //pointer to list of arguments
    enum builtin_t
    {
        NONE, QUIT, JOBS, BG, FG
    } builtin; 
};

//global variables
struct command cmd;
char *cmdline;
char prompt[] = "lnsh>";

void runBuiltinCommand(){}
void runSystemCommand(struct command *cmd, int bg)
{
    pid_t childPid;

    if((childPid = fork()) < 0)
        printf("fork() error");
    else if (childPid == 0)
    {
        if(execvp(cmd->argv[0],cmd->argv)< 0)
        {
            printf("%s: Command not found\n", cmd->argv[0]);
            exit(0);
        }
    }
    else
    {
        if(bg)
            printf("Child in background [%d]\n",childPid);
        else
            wait(&childPid);
    }
}

int parse()
{
    if(cmdline == NULL)
    {
        printf("CommandLine is NULL");
        return 0;
    }
    
    cmd.argv[0]= strtok(cmdline,"-");
    for(int i=0;cmd.argv[i]!=NULL;i++)
    {
        cmd.argc++;     //number of arguments including the command itself
        cmd.argv[i+1] = strtok(0,"-");
        printf("%s\t",cmd.argv[i+1]);
    }
    printf("%d",cmd.argc);
    printf("\n0th argument is %s\n",cmd.argv[0]);

    if(cmdline[strlen(cmdline)-1] == '&')
        return 1;
    else 
        return 0;

}

// int parse(char *cmdline, struct command *cmd)
// {
//     char array[MAXLINE]; //local copy of the command line
//     char delims[10]=" \t\r\n";
//     char *line = array;         //pointer that traverses the commandline
//     char *token;                //pointer to the end of the current argument
//     char *endline;              //pointer to the end of the commandline string
//     int is_bg;                  //to check if it's a background job

//     if(cmdline == NULL)
//         // error("command line is NULL\n");
    
//     (void)strncpy(line,cmdline,MAXLINE);
//     endline = line + strlen(line);
//     cmd->argc = 0;
//     while (line < endline)
//     {
//         line += strspn(line, delims);
//         if(line > endline)
//             break;
        
//         token = line + strcspn(line, delims);
//         *token = '\0';

//         cmd->argv[cmd->argc++] = line;

//         if(cmd->argc >= MAXARGS - 1)
//             break;
        
//         line = token + 1;

//     }
//     cmd->argv[cmd->argc] = NULL;

//     if(cmd->argc == 0)
//         return 1;
//     for(int i=0;i<cmd->argc;i++)
//         printf("%s",cmd->argv[i]);
//     //cmd->builtin = parseBuiltin(cmd);

//     if((is_bg = (*cmd->argv[cmd->argc - 1] == '&')) != 0)
//         cmd->argv[--cmd->argc] = NULL;

//     return is_bg;
// }


void eval()
{
    int bg;

    printf("Evaluating '%s'\n",cmdline);

    // parse the line to fit values into cmd structure
    bg = parse();

    //if errors in parsing
    if(bg == 1)
        return;
    // if command is empty, ignore
    if(cmd.argv[0]==NULL)   
        return;

    if(cmd.builtin == NONE)
        runSystemCommand(&cmd, bg);
    else
        runBuiltinCommand(&cmd, bg);

}

int main(int argc, char **argv)
{
    
    cmdline = malloc(MAXLINE);
    if (cmdline == NULL) {
        printf("No memory\n");
        return 1;
    }

    while(1)
    {
        printf("%s",prompt);

        if((fgets(cmdline,MAXLINE,stdin)==NULL) && ferror(stdin))
            printf("fgets error");
        
        if((strlen(cmdline)>0) && (cmdline[strlen(cmdline)-1] == '\n'))
            cmdline[strlen(cmdline) - 1] = '\0';

        printf("%s\n",cmdline);

        if(feof(stdin))
        {
            printf("\n");
            exit(0);
        }

        eval(cmdline);      // evaluates the given command. Also parses it before that
    }
    return 0;
}
