#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <error.h>
#include <dirent.h>
#include <time.h>


#define MAXLINE 1024
#define MAXARGS 128

struct command
{
    int argc;               //count of arguments
    char *argv[MAXARGS];    //pointer to list of arguments 
};

//global variables
struct command cmd;
char *cmdline;
char prompt[MAXLINE];
char pwd[MAXLINE];
int bgcount=0;

void handler(int sig)
{
  pid_t pid;
  pid = wait(NULL);
  if(pid!=-1)
    printf("\nProcess with pid %d exitted normally.\n",pid);
  bgcount--;

}


void makeprompt()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    //char *name;
    //gethostname(name,MAXLINE);
    if (pw)
        strcpy(prompt,pw->pw_name);
    strcat(prompt,"@");
    struct utsname u;
    uname (&u);
    strcat(prompt,u.nodename);
    strcat(prompt,":");
    char HOME[100];
    
    strcpy(HOME,getenv("HOME"));
    // printf("%s\t",HOME);
    // printf("%s\n",pwd);

    if(strlen(HOME)<=strlen(pwd))
    {
        sprintf(prompt,"%s~",prompt);
        for(int i=strlen(HOME);i<strlen(pwd);i++)
        {
            sprintf(prompt,"%s%c",prompt,pwd[i]);
        }
    }
    else
    {
        strcat(prompt,pwd);
    }
    int length_of_prompt = strlen(prompt);
    prompt[length_of_prompt] = '$';
    prompt[++length_of_prompt] = '\0';
}

void strip() //similar to the strip function in Python
{
    for(int i=0;i<cmd.argc;i++)
    {
        if(cmd.argv[i]==NULL)
            break;
        int len=strlen(cmd.argv[i]);
        // printf("%d\t",len);
        for(int j=len-1;j>=0;j--)
        {
            if(cmd.argv[i][j] == ' ')
                cmd.argv[i][j]='\0';
            else   
                break;
        }
    }
}

// Shell Custom Commands
int shell_reminder()
{
    //printf("Evaluating RemindMe\n");
    sleep(atoi(cmd.argv[1]));
    printf("\n");
    for(int i=2;i<cmd.argc;i++)
    {
        printf("%s ",cmd.argv[i]);
    }
    printf("\n");
    return 0;
}

int shell_clock()
{
    int n=5,t=1;
    for(int i=0;i<cmd.argc;i++)
    {
        if(cmd.argv[i][0]=='-')
        {
            if(cmd.argv[i][1]=='n')
                n = atoi(cmd.argv[++i]);
            else if(cmd.argv[i][1]=='t')
                t = atoi(cmd.argv[++i]);
        }
    }
    for(int i=1;i<=n;i++)
    {
        time_t curtime;
        struct tm *loc_time;
        curtime = time (NULL);
        loc_time = localtime (&curtime);
        char buf[50];
        strftime (buf,50, "%d %b %Y, %T\n", loc_time);
        fputs(buf,stdout);
        sleep(t);
    }
    return 0;
}

//Shell Inbuilt Commands in here

int shell_execvp()
{
    int ret = execvp(cmd.argv[0],cmd.argv);
    if(ret==0)
        printf("Exitted successfully\n");
    else
        printf("An error occurred\n");
    return ret;
}

int shell_echo()
{
    for(int i=1;i<cmd.argc;i++)
    {
        printf("%s ",cmd.argv[i]);
    }
    printf("\n");
    return 0;
}

int shell_pinfo()
{   
    pid_t pid = getpid();
    char dir[100];
    if(cmd.argc==1)
        sprintf(dir,"/proc/%d/status",pid);
    else
        sprintf(dir,"/proc/%s/status",cmd.argv[1]);
    
    int fd = open(dir,O_RDONLY);
    if(fd == -1)
    {
        printf("Process doesnot exist\n");
        return 0;
    }
    char buf[4096];
    read(fd,buf,4096);
    char *line;
    line = strtok(buf,"\n");
    line = strtok(0,"\n");
    line = strtok(0,"\n");
    printf("Process %s\n",line);
    line = strtok(0,"\n");
    line = strtok(0,"\n");
    line = strtok(0,"\n");
    printf("Process %s\n",line);
    
    if(cmd.argc==1)
        sprintf(dir,"/proc/%d/statm",pid);
    else
        sprintf(dir,"/proc/%s/statm",cmd.argv[1]);
    
    fd=open(dir,O_RDONLY);
    read(fd,buf,4096);
    line = strtok(buf," ");
    printf("Virtual Memory: %s\n",line);

    if(cmd.argc==1)
        sprintf(dir,"/proc/%d/exe",pid);
    else
        sprintf(dir,"/proc/%s/exe",cmd.argv[1]);
    int len = readlink(dir,buf,sizeof(buf)-1);
    buf[len]='\0';
    printf("Executable Path : %s\n",buf);
    return 0;
}

int shell_cd()
{
    if(cmd.argc==1 || cmd.argv[1][0]=='~')
    {
        if(chdir(getenv("HOME"))==0)
        {
            getcwd(pwd,MAXLINE);
            makeprompt();
            return 0;
        }
    }
    char new_dir[MAXLINE]="";
    for(int i=1;i<cmd.argc;i++)
    {
        sprintf(new_dir,"%s%s ",new_dir,cmd.argv[i]);
    }
    new_dir[strlen(new_dir)-1]='\0';
    if(chdir(new_dir)==0)
    {    
        getcwd(pwd,MAXLINE);
        makeprompt();
        return 0;
    }
    else
    {
        printf("Directory doesnot exist\n");
        return 0;
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
            {    
                a=1;
            }
            if(cmd.argv[i][j] == 'l')
            {
                l=1;
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
        //printf("This directory\n");    
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
                time_t t = time(NULL);
                struct tm *tmp = localtime(&t);
                char outstr[200];
                if(tmp->tm_year == localtime(&buf.st_ctime)->tm_year)
                    strftime(outstr, sizeof(outstr), "%b %e %R", localtime(&buf.st_ctime));
                else
                    strftime(outstr, sizeof(outstr), "%b %e %Y", localtime(&buf.st_ctime));
                printf("\t%s", outstr);
                printf("\t%s\n",sd->d_name);
                    
            }
            else
            {
                printf("%s\t",sd->d_name);
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
                time_t t = time(NULL);
                struct tm *tmp = localtime(&t);
                char outstr[200];
                if(tmp->tm_year == localtime(&buf.st_ctime)->tm_year)
                    strftime(outstr, sizeof(outstr), "%b %e %R", localtime(&buf.st_ctime));
                else
                    strftime(outstr, sizeof(outstr), "%b %e %Y", localtime(&buf.st_ctime));
                printf("\t%s", outstr);
                printf("\t%s\n",sd->d_name);
            }
            else
            {
                printf("%s\t",sd->d_name);
            }

        }
        

    }
    printf("\n"); 
    return 0;
}

int parse()
{
    if(cmdline == NULL)
    {
        printf("CommandLine is NULL");
        return 0;
    }
    
    cmd.argv[0]= strtok(cmdline," \t");
    for(int i=0;cmd.argv[i]!=NULL;i++)
    {
        cmd.argc++;     //number of arguments including the command itself
        cmd.argv[i+1] = strtok(0," \t");
    }
    if(cmd.argc==0)
    {
        return 0;
    }
    else if(cmd.argc>=1 && cmd.argv[cmd.argc-1][0]=='&')
    {
        //printf("removed &");
        --cmd.argc;
        cmd.argv[cmd.argc]=NULL;
        return 1;
    }
    else if(!strcmp(cmd.argv[0],"remindme"))
    {
        return 1;
    }
    else 
        return 0;

}


int eval()
{
    int bg;

    //printf("Evaluating '%s'\n",cmdline);

    // parse the line to fit values into cmd structure
    bg = parse();

    strip();    // strips off trailing whitespaces
    
    // if command is empty, ignore
    if(cmd.argv[0]==NULL)
    {
        printf("No command passed\n");   
        return 1;
    }
    if(!strcmp(cmd.argv[0],"ls"))
    {
        //printf("Evaluating list\n");
        
        return !shell_ls();
         
    }
    else if(!strcmp(cmd.argv[0],"cd"))
    {
        //printf("Evaluating change dir\n");
        return !shell_cd();
    }
    else if(!strcmp(cmd.argv[0],"pwd"))
    {
        return !shell_pwd();
    }
    else if(!strcmp(cmd.argv[0],"exit"))
    {
        //printf("Quitting\n");
        return 0;
    }
    else if(!strcmp(cmd.argv[0],"echo"))
    {
        return !shell_echo();
    }
    else if(!strcmp(cmd.argv[0],"pinfo"))
    {
        return !shell_pinfo();
    }
    else if(!strcmp(cmd.argv[0],"clock"))
    {
        return !shell_clock(); 
    }
    else
    {
        
        pid_t pid = fork();
        int status;
        if(pid == -1)
        {
            printf("Error forking\n");
        }
        else if(pid!=0)
        {
            if(bg!=1)
            {
                waitpid(pid,&status,WUNTRACED);    
            }
            else
            {
                waitpid(pid,&status,WNOHANG);
            }
        }
        else if(pid==0)
        {
            if(!strcmp(cmd.argv[0],"remindme"))
            {
                //printf("Executing Remindme");
                shell_reminder();
                exit(0);
            }
            else
            {
                if(shell_execvp()==0)
                    printf("Process terminated successfully\n");
                else
                    printf("An error ocurred\n");
                exit(0);
            }
        }
        
    }
    return 1;
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
        signal(SIGCHLD, handler);

        printf("%s ",prompt);
        cmd.argc = 0;
        if((fgets(cmdline,MAXLINE,stdin)==NULL) && ferror(stdin))
            printf("fgets error");
        
        if((strlen(cmdline)>0) && (cmdline[strlen(cmdline)-1] == '\n'))
            cmdline[strlen(cmdline) - 1] = '\0';

        //printf("%s\n",cmdline);

        if(feof(stdin))
        {
            printf("\n");
            exit(0);
        }

              // evaluates the given command. Also parses it before that
    }while(eval(cmdline));
    return 0;
}
