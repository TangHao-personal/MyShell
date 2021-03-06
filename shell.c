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
	printf("executing normal\n");
	execvp(cmd->buf, cmd->argv);
	perror("exec");
	exit(1);
}

void redirect_out(command *cmd)
{	
	printf("executing redirect_out\n");
	if(cmd->argv[cmd->pos + 1] == NULL) perror("command > [option]"), exit(1);
	cmd->argv[cmd->pos] = NULL;
	int fd;
	if((fd = open(cmd->argv[cmd->pos+1], O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) perror("open"), exit(1);
	dup2(fd, STDOUT_FILENO);
	close(fd);		
	execvp(cmd->buf, cmd->argv);
	printf("problem in redirect_out\n");
	perror("exec");
	exit(1);
}

void redirect_in(command *cmd)
{
	printf("executing redirect_in\n");
	if(cmd->argv[cmd->pos + 1] == NULL) perror("command < [option]"), exit(1);
	cmd->argv[cmd->pos] = NULL;
	int fd;
	if((fd = open(cmd->argv[cmd->pos+1], O_RDONLY)) < 0) perror("open"), exit(1);
	dup2(fd, STDIN_FILENO);
	close(fd);
	execvp(cmd->buf, cmd->argv);
	printf("problem in redirect_in\n");
	perror("exec");
	exit(1);
}

void redirect_append(command *cmd)
{
	printf("executing redirect_append\n");
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

void pwd()
{
	printf("executing pwd\n");
	char buf[80];
    getcwd(buf, sizeof(buf));
	printf("%s\n", buf);
}

void cd(command * cmd)
{
	printf("executing cd\n");
	if(cmd->argv[cmd->pos + 1] == NULL) perror("cd [option]"), exit(1);
	chdir(cmd->argv[cmd->pos + 1]);
	pwd();
}

void execute(command *cmd)
{
	if(cmd->op == 0) exit(0);
	pid_t pid;
	if((pid = fork()) < 0) perror("fork"), exit(1);
	if(pid == 0)
	{
		switch(cmd->op)
		{
			case 1: redirect_in(cmd);break;
			case 2: redirect_out(cmd);break;
			case 3: redirect_append(cmd);break;
			case 4: pwd();break;
			case 5: cd(cmd);break;
			default: normal(cmd);break;
		}
	}
	if(pid > 0){
		wait(NULL);
		exit(1);
	}
}

void pipe_exec(command *cmd)
{
	switch(cmd->op)
	{
		case 1: redirect_in(cmd);break;
		case 2: redirect_out(cmd);break;
		case 3: redirect_append(cmd);break;
		case 4: pwd();break;
		case 5: cd(cmd);break;
		default: normal(cmd);break;
	}
}

void execute_pipe(command *cmd, command *cmd_next)
{
	int fd[2];
	if(pipe(fd) < 0) perror("pipe"),exit(1);
	pid_t pid;
	if((pid = fork()) < 0) perror("fork"), exit(1);
	if(pid == 0)
	{
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		execute(cmd);
		exit(1);
	}
	if(pid > 0)
	{
		int status;
		waitpid(pid, &status, 0);
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		execute(cmd_next);
	}
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
	if(strcmp(cmd->argv[0], "exit") == 0) {cmd->op = 0; return;}
	if(strcmp(cmd->argv[0], "pwd") == 0) {cmd->op = 4; cmd->pos = 0; return;}
	if(strcmp(cmd->argv[0], "cd") == 0) {cmd->op = 5; cmd->pos = 0; return;}
	for(int i = 0; i < cmd->argc; i++)
	{
		if(strcmp(cmd->argv[i], "<") == 0) {cmd->op = 1, cmd->pos = i;break;}
		else if(strcmp(cmd->argv[i], ">") == 0) {cmd->op = 2; cmd->pos = i;break;}
		else if(strcmp(cmd->argv[i], ">>") == 0) {cmd->op = 3; cmd->pos = i;break;}
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
			cmd->buf[i++] = 0;
			while(isspace(cmd->buf[i])) i++;
			strcpy(cmd_next->buf, cmd->buf + i);		
			pipe = 1;
			break;
		}
	}
	if(pipe == 0)
	{
		printf("executing single command...\n");
		parser(cmd);
		execute(cmd);
	}
	else
	{
		printf("executing pipe command...\n");
		parser(cmd);
		parser(cmd_next);
		pid_t pid;
		if((pid = fork()) < 0) perror("fork"), exit(1);
		if(pid == 0)
		{
			execute_pipe(cmd, cmd_next);
			exit(1);
		}
		if(pid > 0)
			wait(NULL);

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

