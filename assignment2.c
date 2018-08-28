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
        QUIT, JOBS, BG, FG
    } builtin; 
};

//global variables
struct command cmd;
char *cmdline;
char prompt[MAXLINE];

void strip() //similar to the strip function in Python
{
    for(int i=0;i<cmd.argc;i++)
    {
        if(cmd.argv[i]==NULL)
            break;
        int len=strlen(cmd.argv[i]);
        printf("%d\t",len);
        for(int j=len-1;j>=0;j--)
        {
            if(cmd.argv[i][j] == ' ')
                cmd.argv[i][j]='\0';
            else   
                break;
        }
    }
}

int shell_ls()
{
    ;;
}

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
    
    cmd.argv[0]= strtok(cmdline,"- ");
    for(int i=0;cmd.argv[i]!=NULL;i++)
    {
        cmd.argc++;     //number of arguments including the command itself
        cmd.argv[i+1] = strtok(0,"- ");
        printf("%s\t",cmd.argv[i+1]);
    }
    printf("%d",cmd.argc);
    printf("\n0th argument is %s\n",cmd.argv[0]);

    if(cmdline[strlen(cmdline)-1] == '&')
        return 1;
    else 
        return 0;

}


void eval()
{
    int bg;

    printf("Evaluating '%s'\n",cmdline);

    // parse the line to fit values into cmd structure
    bg = parse();

    strip();    // strips off trailing whitespaces

    //if errors in parsing
    if(bg == 1)
        return;
    // if command is empty, ignore
    if(cmd.argv[0]==NULL)   
        return;

    if(!strcmp(cmd.argv[0],"ls"))
    {
        printf("Evaluating list\n");
        shell_ls();
    }
    else if(!strcmp(cmd.argv[0],"cd"))
    {
        printf("Evaluating change dir\n");
    }
}

int main(int argc, char **argv)
{
    strcpy(prompt,getenv("HOME"));
    int length_of_prompt = strlen(prompt);
    prompt[length_of_prompt] = '>';
    prompt[++length_of_prompt] = '\0';

    cmdline = malloc(MAXLINE);
    if (cmdline == NULL) {
        printf("No memory\n");
        return 1;
    }

    while(1)
    {
        printf("%s",prompt);
        cmd.argc = 0;
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
