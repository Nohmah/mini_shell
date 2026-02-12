/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readcmd.h"
#include "csapp.h"

void run_cmd(struct cmdline* cl);

int main()
{
	while (1) {
		struct cmdline *l;
		int i;
		//int j;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if(l->seq && l->seq[0] && l->seq[0][0] && (strcmp(l->seq[0][0], "quit") == 0)){
			printf("exit\n");
			exit(0);
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);

		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			//char **cmd = l->seq[i];
			//printf("seq[%d]: ", i);
			// for (j=0; cmd[j]!=0; j++) {
			// 	printf("%s ", cmd[j]);
			// }
			printf("\n");
			run_cmd(l);
		}

	}
}

void run_cmd(struct cmdline* cl){
	int fd1 = -1, fd2 =-1;

	char** cmd = cl->seq[0];
	if( cl->in != NULL){
		fd1 = Open(cl->in, O_RDONLY, 0);
	}
	if( cl->out != NULL){
		fd2 = Open(cl->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd2 == -1){
			fprintf(stderr, "%s: permission denied.\n", cl->out);

		}
	}
	pid_t pid = Fork();
	if(pid == 0){
		if (fd1 != -1){
			Dup2(fd1, STDIN_FILENO);
			Close(fd1);
		}
		if (fd2 != -1){
			Dup2(fd2, STDOUT_FILENO);
			Close(fd2);
		}
		if(execvp(cmd[0], cmd) == -1){
			fprintf(stderr, "%s: Command not found.\n", cmd[0]);
			exit(2);
		}
	}
	else{
		if (fd1 != -1){
			Close(fd1);
		}
		if (fd2 != -1){
			Close(fd2);
		}
		
		waitpid(pid, NULL, 0);
	}
}
