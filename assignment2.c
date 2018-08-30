#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <error.h>
#include <dirent.h>


#define MAXLINE 1024
#define MAXARGS 128

struct command
{
    int argc;               //count of arguments
    char *argv[MAXARGS];    //pointer to list of arguments
    int bg; 
};

//global variables
struct command cmd;
char *cmdline;
char prompt[MAXLINE];
char pwd[MAXLINE];

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

//Shell Inbuilt Commands in here

int shell_cd()
{
    if(cmd.argc==1)
    {
        if(chdir(getenv("HOME"))==0)
        {
            getcwd(pwd,MAXLINE);
            makeprompt();
            return 1;
        }
    }
    char new_dir[MAXLINE];
    for(int i=1;i<cmd.argc;i++)
    {
        strcpy(new_dir,cmd.argv[i]);
    }
    if(chdir(cmd.argv[1])==0)
    {    
        getcwd(pwd,MAXLINE);
        makeprompt();
        return 1;
    }
}

int shell_pwd()
{
    printf("%s\n",pwd);
    return 0;
}

int shell_ls()
{
    int a=0, l=0,c;
    for(int i=1;i<cmd.argc;i++)
    {
        if(cmd.argv[i][0]!='-')
            continue;

        for(int j=0;j<strlen(cmd.argv[i]);j++)
        {
            if(cmd.argv[i][j] == 'a')
            {    a=1;
                printf("a");
            }
            if(cmd.argv[i][j] == 'l')
            {
                l=1;
                printf("l");
            }

        }
    }
    char *pointer=NULL;
    DIR *dp=NULL;
    struct dirent *sd=NULL;
    pointer = cmd.argv[cmd.argc-1];
    if(cmd.argv[cmd.argc-1][0]=='-' || cmd.argc==1)
    {
        pointer = ".";
        printf("This directory\n");    
    }
    else
        pointer = &cmd.argv[cmd.argc-1][0];
    dp=opendir((const char*)pointer);
    while((sd=readdir(dp))!=NULL)
    {
        if(sd->d_name[0]=='.')
        {
            if(a==0)
                continue;
            if(l==1)
            {
                struct stat buf;
                stat(sd->d_name,&buf);
                printf( (S_ISDIR(buf.st_mode)) ? "d" : "-");
                printf( (buf.st_mode & S_IRUSR) ? "r" : "-");
                printf( (buf.st_mode & S_IWUSR) ? "w" : "-");
                printf( (buf.st_mode & S_IXUSR) ? "x" : "-");
                printf( (buf.st_mode & S_IRGRP) ? "r" : "-");
                printf( (buf.st_mode & S_IWGRP) ? "w" : "-");
                printf( (buf.st_mode & S_IXGRP) ? "x" : "-");
                printf( (buf.st_mode & S_IROTH) ? "r" : "-");
                printf( (buf.st_mode & S_IWOTH) ? "w" : "-");
                printf( (buf.st_mode & S_IXOTH) ? "x" : "-");
                printf("\t%ld",buf.st_nlink);
                printf("\t%s",getpwuid(buf.st_uid)->pw_name);                
                printf("\t%s",getgrgid(buf.st_gid)->gr_name);                
                printf("\t%ld",buf.st_size);
                printf("\t%s\n",sd->d_name);
                    
            }
            else
            {
                printf("\t%s\n",sd->d_name);
            }
        }
        else
        {
            if(l==1)
            {
                struct stat buf;
                struct passwd pw;
                stat(sd->d_name,&buf);
                printf( (S_ISDIR(buf.st_mode)) ? "d" : "-");
                printf( (buf.st_mode & S_IRUSR) ? "r" : "-");
                printf( (buf.st_mode & S_IWUSR) ? "w" : "-");
                printf( (buf.st_mode & S_IXUSR) ? "x" : "-");
                printf( (buf.st_mode & S_IRGRP) ? "r" : "-");
                printf( (buf.st_mode & S_IWGRP) ? "w" : "-");
                printf( (buf.st_mode & S_IXGRP) ? "x" : "-");
                printf( (buf.st_mode & S_IROTH) ? "r" : "-");
                printf( (buf.st_mode & S_IWOTH) ? "w" : "-");
                printf( (buf.st_mode & S_IXOTH) ? "x" : "-");
                printf("\t%ld",buf.st_nlink);
                printf("\t%s",getpwuid(buf.st_uid)->pw_name);  
                printf("\t%s",getgrgid(buf.st_gid)->gr_name);                

                printf("\t%ld",buf.st_size);
                printf("\t%s\n",sd->d_name);

            }
            else
            {
                printf("\t%s\n",sd->d_name);
            }

        }
        

    }
    printf("\n"); 
}

int parse()
{
    if(cmdline == NULL)
    {
        printf("CommandLine is NULL");
        return 0;
    }
    
    cmd.argv[0]= strtok(cmdline," ");
    for(int i=0;cmd.argv[i]!=NULL;i++)
    {
        cmd.argc++;     //number of arguments including the command itself
        cmd.argv[i+1] = strtok(0," ");
        printf("%s\t",cmd.argv[i+1]);
    }
    printf("%d",cmd.argc);
    printf("\n0th argument is %s\n",cmd.argv[0]);

    if(cmdline[strlen(cmdline)-1] == '&')
        return 1;
    else 
        return 0;

}


int eval()
{
    int bg;

    printf("Evaluating '%s'\n",cmdline);

    // parse the line to fit values into cmd structure
    bg = parse();

    strip();    // strips off trailing whitespaces

    //if errors in parsing
    if(bg == 1)
        return 1;
    // if command is empty, ignore
    if(cmd.argv[0]==NULL)   
        return 1;

    if(!strcmp(cmd.argv[0],"ls"))
    {
        printf("Evaluating list\n");
        shell_ls();
    }
    else if(!strcmp(cmd.argv[0],"cd"))
    {
        printf("Evaluating change dir\n");
        shell_cd();
    }
    else if(!strcmp(cmd.argv[0],"pwd"))
    {
        shell_pwd();
    }
    else if(!strcmp(cmd.argv[0],"exit"))
    {
        printf("Quitting\n");
        return 0;
    }
    return 1;
}

void makeprompt()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
        strcpy(prompt,pw->pw_name);
    strcat(prompt,"@");
    strcat(prompt,pwd);
    int length_of_prompt = strlen(prompt);
    prompt[length_of_prompt] = '>';
    prompt[++length_of_prompt] = '\0';
}

int main(int argc, char **argv)
{
    strcpy( pwd,getenv("HOME"));
    int ret = chdir(pwd);
    if(ret!=0)
        printf("chdir didnot work");
    makeprompt();
    

    cmdline = malloc(MAXLINE);
    if (cmdline == NULL) {
        printf("No memory\n");
        return 1;
    }

    do
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

              // evaluates the given command. Also parses it before that
    }while(eval(cmdline));
    return 0;
}
