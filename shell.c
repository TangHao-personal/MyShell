#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void execute(char *buf, char *argv[10])
{
	pid_t pid = fork();
	if(pid == 0)
	{
		for(int i = 0; argv[i] != 0; ++i)
		{
			if(strcmp(argv[i], ">") == 0)
			{
				if(argv[i + 1] == NULL) perror("command > [option]"), exit(1);
				int fd = open(argv[i + 1], O_WRONLY|O_TRUNC|O_CREAT, 0644);
				if(fd < 0) perror("open"), exit(1);
				argv[i] = NULL;
				dup2(fd, STDOUT_FILENO);
				close(fd);
				break;
			}
			if(strcmp(argv[i], ">>") == 0)
			{
				if(argv[i + 1] == NULL) perror("command >> [option]"), exit(1);
				int fd = open(argv[i + 1], O_WRONLY|O_APPEND|O_CREAT, 0644);
				if(fd < 0) perror("open"), exit(1);
				argv[i] = NULL;
				dup2(fd, STDOUT_FILENO);
				close(fd);
				break;
			}
			if(strcmp(argv[i], "<") == 0)
			{
				if(argv[i + 1] == NULL) perror("command < [option]"), exit(1);
				int fd = open(argv[i + 1], O_RDONLY);
				if(fd < 0) perror("read"), exit(1);
				argv[i] = NULL;
				dup2(fd, STDIN_FILENO);
				close(fd);
				break;
			}
		}
		execvp(buf, argv);
		perror("exec");
		exit(1);
	}
	wait(NULL);
}

void parser(char *buf)
{
	int argc = 0;
	char *argv[10] = {};
	int status = 0;
	for(int i = 0; buf[i] != 0; ++i)
	{
		if(status == 0 && !isspace(buf[i]))
		{
	 		argv[argc++] = buf + i;
			status = 1;
		}
		if(isspace(buf[i]))
		{
			status = 0;
			buf[i] = 0;
		}
	}
	argv[argc] = NULL;
	execute(buf, argv);
}

int main()
{
	char buf[1024] = {};
	memset(buf, 0x00, sizeof(buf));

	while(1)
	{
		printf("My shell# ");
		while(scanf("%[^\n]%*c", buf) == 0)
		{
			printf("My shell# ");
			while(getchar() != '\n');
		}

		parser(buf);
	}

	return 0;
}

