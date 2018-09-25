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
#include <errno.h>

#define MAXLINE 1024
#define MAXARGS 128

struct command
{
    int argc;               //count of arguments
    char *argv[MAXARGS];    //pointer to list of arguments 
};

struct proc{
    int id;
    char name[1024];
    int state;
};

struct proc current_p;
struct proc proc_list[1024];
int bgcount=0;

//global variables
struct command cmd;
char *cmdline;
char prompt[MAXLINE];
char pwd[MAXLINE];
pid_t currentpid=-1;

// global functions
void makeprompt();

// signmal handlers
void sigint_handler (int sig) 
{
    printf ("\n");
    makeprompt();
    printf("%s",prompt);
    fflush (stdout);
}

void exit_handler(int sig)
{
    printf("__in__\n");
    if(sig == SIGINT)
    {
        //int killable = 1;
        for(int i=0; i<bgcount; i++)
        {
            if(kill(proc_list[i].id, 0) == 0)
            {
                if(proc_list[i].state==1)
                {
                    kill(proc_list[i].id, SIGINT);
                    //killable = 0;
                }
            }
        }
        //if(killable == 1)
        //    kill(current_p.id, SIGINT);
            
        signal(SIGINT, SIG_IGN);
    }
    
    if(sig == SIGSTOP);
        signal(SIGSTOP, SIG_IGN);

    if(sig == SIGTSTP);
    {
        if(current_p.id != -1)
        {
            strcpy(proc_list[bgcount].name, current_p.name);
            proc_list[bgcount].id = current_p.id;
            proc_list[bgcount].state = 0;
            printf("Current Process id = %d\n", current_p.id);
            bgcount++;
            printf("Total # of Processes : %d\n", bgcount);
            kill(current_p.id, SIGTSTP);

            strcpy(current_p.name, "__!!!>>>%^%^");
            current_p.id = -1;
            current_p.state = -1;
            signal(SIGTSTP, SIG_IGN);
        }
    }
    fflush(stdout);
    return;
}

void stop_handler(int sig)
{
    printf ("\n");
    makeprompt();
    printf("%s ",prompt);
    fflush (stdout);
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

    if(strlen(HOME) <= strlen(pwd))
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
int shell_setenv () {
    if (cmd.argc !=2 && cmd.argc != 3)
        return 1;
	if (cmd.argv[2] == NULL) 
        cmd.argv[2] = "";
	if (setenv(cmd.argv[1],cmd.argv[2],1) != 0)
    {
        perror("shell");
        return 1;
    }
	return 0;	
}

int shell_unsetenv () {
    if (cmd.argc != 2)
        return 1;
	if (unsetenv(cmd.argv[1]) != 0)
    {
        perror("shell");
        return 1;
    }
	return 0;
}

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

int shell_jobs()
{
    printf("Background Process Count : %d\n", bgcount);
    int on,idx=1;
    for(int i=0;i<bgcount;i++)
    {
       if(kill(proc_list[i].id, 0) == 0)
       {
            char dir[100];
            sprintf(dir,"/proc/%d/status",proc_list[i].id);
            int fd = open(dir, O_RDONLY);
            char buf[4096];
            read(fd,buf,4096);
            char *line;
            line = strtok(buf,"\n");
            line = strtok(0,"\n");
            line = strtok(0,"\n");
            //printf("Process %s\n",line);
            //printf("Status %c\n",line[7]); 
            char status[128];
            if(line[7] == 'T')
                strcpy(status, "STOPPED");
            else
                strcpy(status, "RUNNING");  
            printf("[%d]\t%s\t%s\t[%d]\n", idx, status, proc_list[i].name, proc_list[i].id);
            idx++;
       }
    }
    return 0;
}

int shell_kjob()
{
    if(cmd.argc != 3)
        return 1;
    int job_id = atoi(cmd.argv[1]);
    int sig_id = atoi(cmd.argv[2]);
    int cnt = 0, stat, flag=0;
    for(int i=0; i<bgcount; i++)
    {
        if(kill(proc_list[i].id, 0) == 0)
        {
            cnt++;
            if(i+1 == job_id)
            {
                flag = 1;
                kill(proc_list[i].id, sig_id);
                while(waitpid(-1, &stat, WNOHANG) > 0)
                    continue;
            }
        }
    }
    
    if(flag==1)
        return 0;
    return 1;
}

int shell_fg()
{
    if(cmd.argc != 2)
        return 1;
    int job=atoi(cmd.argv[1]);
    int cnt = 0, stat, flag=0;
    for(int i=0; i<bgcount; i++)
    {
        if(kill(proc_list[i].id, 0) == 0 && i+1 == job)
        {
            kill(proc_list[i].id, getpid());
            proc_list[i].state = 1;
            current_p = proc_list[i];
            flag=1;
            waitpid(proc_list[i].id, &stat, WUNTRACED);
        }
    }
    if(flag==1)
        return 0;
    return 1;
}

int shell_bg()
{
    if(cmd.argc != 2)
        return 1;
    int job = atoi(cmd.argv[1]);
    int cnt=0, stat, flag=0;
    for(int i=0; i<bgcount; i++)
    {
        if(kill(proc_list[i].id, 0) == 0 && i+1 == job)
        {
            kill(proc_list[i].id, SIGTSTP);
            flag=1;
        }
    }
    if(flag==1)
        return 0;
    return 1;
}

int shell_overkill()
{
    for(int i=0; i<bgcount; i++)
    {
        kill(proc_list[i].id, 9);
    }
    return 0;
}

int shell_quit()
{
    for(int i=0; i<bgcount; i++)
    {
        kill(proc_list[i].id, 9);
    }
    exit(0);
}

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
        printf("Process does not exist\n");
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
        if(cmd.argv[i]==NULL)
            continue;
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
    struct dirent *sd=NULL;signal(SIGCHLD, exit_handler);
        signal(SIGINT, sigint_handler);
        signal(SIGTSTP, stop_handler);
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

int parse(char *ctoken)
{
    if(ctoken == NULL)
    {
        printf("CommandLine is NULL");
        return 0;
    }
    cmd.argc=0;
    cmd.argv[0]= strtok(ctoken," \t");
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


int eval(char *token)
{
    int bg;
    cmd.argc = 0;
    //printf("Evaluating '%s'\n",token);
    
    if(token==NULL)
    {
        return 1;
    }
    
    bg = parse(token);
    strip();    // strips off trailing whitespaces
    
    // if command is empty, ignore
    if(cmd.argv[0]==NULL)
    {
        printf("No command passed\n");   
        return 1;
    }
    int inpo=dup(0),outo=dup(1);
    int inf=0,outc=0,appc=0,fd;
    char inpfile[64];
    char outfile[64];
    char appfile[64];

    for(int i=0;i<cmd.argc;i++)
    {
        if(strcmp(cmd.argv[i],"<")==0)
        {
            cmd.argv[i]=NULL;
            strcpy(inpfile,cmd.argv[i+1]);
            inf=1;
        }
        else if(strcmp(cmd.argv[i],">")==0)
        {
            cmd.argv[i]=NULL;
            strcpy(outfile,cmd.argv[i+1]);
            outc=1;
        }
        else if(strcmp(cmd.argv[i],">>")==0)
        {
            cmd.argv[i]=NULL;
            strcpy(appfile,cmd.argv[i+1]);
            appc=1;
        }
    }
    if(inf!=0)
    {
        fd=open(inpfile, O_RDONLY, 0);
        if(fd<0) 
            perror("Error opening input file");
        if(dup2(fd,0)<0)
            perror("Error in input");
        close(fd);

    }
    if(outc!=0)
    {
        fd=open(outfile, O_WRONLY | O_TRUNC | O_CREAT ,0644);
        if(fd<0) 
            perror("Error opening input file");
        if(dup2(fd,1)<0)
            perror("Error in output");
        close(fd);
    }
    if(appc!=0)
    {
        fd=open(appfile,O_WRONLY|O_APPEND|O_CREAT,0644);
        if(fd<0)
            perror("Error opening append file");
        if(dup2(fd,1)<0)
            perror("Error in output");
        close(fd);
    }

    // if(!strcmp(cmd.argv[0],"ls"))
    // {
    //     //printf("Evaluating list\n");
        
    //     return !shell_ls();
        
    // }
    if(!strcmp(cmd.argv[0],"cd"))
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
        exit(0);
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
    else if(!strcmp(cmd.argv[0],"setenv"))
    {
        return !shell_setenv(); 
    }
    else if(!strcmp(cmd.argv[0],"unsetenv"))
    {
        return !shell_unsetenv(); 
    }
    else if(!strcmp(cmd.argv[0],"jobs"))
    {
        return !shell_jobs();
    }
    else if(!strcmp(cmd.argv[0],"kjob"))
    {
        return !shell_kjob();
    }
    else if(!strcmp(cmd.argv[0],"fg"))
    {
        return !shell_fg();
    }
     else if(!strcmp(cmd.argv[0],"bg"))
    {
        return !shell_bg();
    }
     else if(!strcmp(cmd.argv[0],"overkill"))
    {
        return !shell_overkill();
    }
     else if(!strcmp(cmd.argv[0],"quit"))
    {
        return !shell_quit();
    }
    else
    {  
        int status;
        int is_reminder=0;
        if(!strcmp(cmd.argv[0],"remindme"))
        {
            is_reminder = 1;
        }

        pid_t pid = fork();

        if(pid == -1)
        {
            printf("Error forking\n");
        }

        else if(pid==0)
        {
            if(bg==1 || is_reminder==1)
                setpgid(0,0);
            if(!strcmp(cmd.argv[0],"remindme"))
            {
                is_reminder = 1;
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

        //if(pid != 0)
            
        
        if(bg==1 || is_reminder==1)
        {
            proc_list[bgcount].id = (int)pid;
            proc_list[bgcount].state = 0;
			strcpy(proc_list[bgcount].name, cmd.argv[0]);
	    	++bgcount;
    		waitpid(pid, &status, WNOHANG);
        }
        else
        {
            current_p.id = pid;
            current_p.state = 1;
            strcpy(current_p.name, cmd.argv[0]);
            waitpid(pid, &status, WUNTRACED);
        }

    }
    dup2(inpo,0);
    dup2(outo,1);
}

int pipehandler(char *command)
{
    // printf("Command:%s\n",command);
    char *commands[MAXARGS];
    int idk=0;
    char *token=strtok(command,"|");
    commands[idk]=token;
    while(commands[idk]!=NULL)
    {
        // printf("%d %s\t",idk,commands[idk]);
        commands[++idk]=strtok(NULL,"|");
        
    }
    if(idk==1)
    {
        return eval(commands[0]);
    }
    
    int pipearr[2];
    int fd = 0;
    for(int i=0;commands[i]!=NULL;i++)
    {
        pipe(pipearr);
        int pid = fork();
        if(pid == 0)
        {
            dup2(fd, 0);
            if(commands[i + 1] != NULL)
            {
                dup2(pipearr[1], 1);
            }
            close(pipearr[0]);
            eval(commands[i]);
            exit(2);
        }
        else{
            wait(NULL);
            close(pipearr[1]);
            fd = pipearr[0];
        }
    }
    fflush(stdout);
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
    //signal(SIGCHLD, exit_handler);
    // signal(SIGINT, sigint_handler);
    // signal(SIGTSTP, stop_handler);
    // int parity=0;
    do
    {
        signal(SIGINT, exit_handler);
        signal(SIGTSTP, exit_handler);
        //signal(SIGTSTP, SIG_IGN);
        int stat;
        while(waitpid(-1, &stat, WNOHANG) > 0);
        
        struct proc temp[1024];
        int idx, cnt = 0, hold;

        for(int i = 0; i < bgcount; ++i)
        {
            //printf("%s\n", proc_list[i].name);
            hold = kill(proc_list[i].id, 0);
            if(hold == -1 && errno == ESRCH)
            {
                printf("%s with pid %d exitted normally\n", proc_list[i].name, proc_list[i].id);
                continue;
            }
            temp[cnt++] = proc_list[i];
        }
        bgcount = cnt;
        for(int i = 0; i < bgcount; i++)
            proc_list[i] = temp[i];
        
        strcpy(current_p.name, "__!!!>>>%^%^");
        current_p.id = -1;
        current_p.state = -1;

        printf("%s ",prompt);
        
        if((fgets(cmdline,MAXLINE,stdin)==NULL) && ferror(stdin))
            perror("fgets error");
        
        if((strlen(cmdline)>0) && (cmdline[strlen(cmdline)-1] == '\n'))
            cmdline[strlen(cmdline) - 1] = '\0';

        //printf("%s\n",cmdline);

        if(feof(stdin))
        {
            printf("\n");
            exit(0);
        }
        
        char *commands[MAXARGS];
        int idk=0;
        char *token=strtok(cmdline,";");
        commands[idk]=token;
        while(commands[idk]!=NULL)
        {
            commands[++idk]=strtok(NULL,";");
        }
        for(int j=0;j<idk;j++)
        {
            pipehandler(commands[j]);
        }
        // evaluates the given command. Also parses it before that
        
    }while(1);
    return 0;
}