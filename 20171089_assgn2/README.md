# BASIC SHELL IN C

## Builtin Features

### cd -  Change the shell working directory

`cd [FILE]...`

### ls - list directory contents

`ls [OPTION]... [FILE]`
> List takes 2 flags `-l` and `-a`
> `-l` use  a long listing format
> `-a` do  not  ignore  entries starting with

### pwd - print  name  of  current/working directory

`pwd`

>Print the full filename of  the current working directory.

### echo - display a line of text

`echo [STRING]...`

> Echo the STRING(s) to  standard output.
> Supports space separated strings.

## Custom Commands

### pinfo - display process information

`pinfo [pid]`

> If no pid is provided, it gives information about the current process
> If pid is provided, it gives information about that process

### remindme - daemon to print scheduled messages

`remindme [TIME] [STRING]...`

> Prints the string after given time

### clock - daemon to print time

`clock -t [TIME_INTERVAL] -n [NUMBER_OF_TIMES]`

> Prints the system time for n times after a t second interval each