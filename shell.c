#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct command
{
	char buf[1024];
	char *argv[10];
	int argc;
	int pos;
	int op;
}command;

void normal(command *cmd)
{
	execvp(cmd->buf, cmd->argv);
	perror("exec");
	exit(1);
}

void redirect_out(command *cmd)
{	
	if(cmd->argv[cmd->pos + 1] == NULL) perror("command > [option]"), exit(1);
	cmd->argv[cmd->pos] = NULL;
	int fd;
	if((fd = open(cmd->argv[cmd->pos+1], O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) perror("open"), exit(1);
	dup2(fd, STDOUT_FILENO);
	close(fd);		
	execvp(cmd->buf, cmd->argv);
	perror("exec");
	exit(1);
}

void redirect_in(command *cmd)
{

	if(cmd->argv[cmd->pos + 1] == NULL) perror("command < [option]"), exit(1);
	cmd->argv[cmd->pos] = NULL;
	int fd;
	if((fd = open(cmd->argv[cmd->pos+1], O_RDONLY)) < 0) perror("open"), exit(1);
	dup2(fd, STDIN_FILENO);
	close(fd);
	execvp(cmd->buf, cmd->argv);
	perror("exec");
	exit(1);
}

void redirect_append(command *cmd)
{
	if(cmd->argv[cmd->pos + 1] == NULL) perror("command >> [option]"), exit(1);
	cmd->argv[cmd->pos] = NULL;
	int fd;
	if((fd = open(cmd->argv[cmd->pos+1], O_WRONLY|O_CREAT|O_APPEND, 0644)) < 0) perror("open"), exit(1);
	dup2(fd, STDOUT_FILENO);
	close(fd);
	execvp(cmd->buf, cmd->argv);
	perror("exec");
	exit(1);
}


void execute(command *cmd)
{
	pid_t pid;
	if((pid = fork()) < 0) perror("fork"), exit(1);
	if(pid == 0)
	{
		switch(cmd->op)
		{
			case 1: redirect_in(cmd);
			case 2: redirect_out(cmd);
			case 3: redirect_append(cmd);
			default: normal(cmd);
		}
	}
	wait(NULL);
}

void execute_pipe(command *cmd, command *cmd_next)
{
	int fd[2];
	pid_t pid1, pid2;

	if(pipe(fd) < 0)  perror("pipe"), exit(1);

	if((pid1 = fork()) < 0) perror("fork"), exit(1);
	if(pid1 == 0)
	{
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		switch(cmd->op)
		{
			case 1: redirect_in(cmd);
			case 2: redirect_out(cmd);
			case 3: redirect_append(cmd);
			default: normal(cmd);
		}
	}
	if((pid2 = fork()) < 0) perror("fork"), exit(1);
	if(pid2 == 0)
	{
		close(fd[1]);
		dup2(fd[0], STDOUT_FILENO);
		close(fd[0]);
		switch(cmd_next->op)
		{
			case 1: redirect_in(cmd_next);
			case 2: redirect_out(cmd_next);
			case 3: redirect_append(cmd_next);
			default: normal(cmd_next);
		}
	}
	close(fd[0]);
	close(fd[1]);
	wait(NULL);	
	wait(NULL);
}

void parser(command *cmd)
{
	cmd->argc = 0;
	int status = 0;
	cmd->pos = 0;
	cmd->op = 0;
	for(int i = 0; cmd->buf[i] != 0; ++i)
	{
		if(status == 0 && !isspace(cmd->buf[i]))
		{
	 		cmd->argv[cmd->argc++] = cmd->buf + i;
			status = 1;
		}
		if(isspace(cmd->buf[i]))
		{
			status = 0;
			cmd->buf[i] = 0;
		}
	}
	cmd->argv[cmd->argc] = NULL;
	for(int i = 0; i < cmd->argc; i++)
	{
		if(strcmp(cmd->argv[i], "<") == 0) {cmd->op = 1, cmd->pos = i;}
		else if(strcmp(cmd->argv[i], ">") == 0) {cmd->op = 2; cmd->pos = i;}
		else if(strcmp(cmd->argv[i], ">>") == 0) {cmd->op = 3; cmd->pos = i;}
	}
}


void check(command *cmd)
{
	int pipe = 0;
	command *cmd_next = (command*)malloc(sizeof(command));
	for(int i = 0; cmd->buf[i] != 0; ++i)
	{
		if(cmd->buf[i] == '|')
		{
			strcpy(cmd_next->buf, cmd->buf + i + 1);
			cmd->buf[i] = 0;
			pipe++;
		}
	}
	if(pipe == 0)
	{
		parser(cmd);
		execute(cmd);
	}
	else
	{
		parser(cmd);
		parser(cmd_next);
		execute_pipe(cmd, cmd_next);
	}
	return;
}


int main()
{
	command *cmd = (command *)malloc(sizeof(command));

	while(1)
	{
		printf("My shell# ");
		while(scanf("%[^\n]%*c", cmd->buf) == 0)
		{
			printf("My shell# ");
			while(getchar() != '\n');
		}

		check(cmd);
	}

	return 0;
}

