/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readcmd.h"
#include "csapp.h"

void run_cmd(struct cmdline *cl);
void run_pipe2(struct cmdline *cl);
void run_pipe(struct cmdline *cl);

int main(){
	while (1){
		struct cmdline *l;
		// int i;
		// int j;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l){
			printf("exit\n");
			exit(0);
		}

		if (l->err){
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->seq && l->seq[0] && l->seq[0][0] && (strcmp(l->seq[0][0], "quit") == 0)){
			printf("exit\n");
			exit(0);
		}

		if(l->seq[0] && l->seq[1] == NULL){
			run_cmd(l);
		} else if(l->seq[0] && l->seq[1]){
			run_pipe(l);
		}

		// if (l->in)
		// 	printf("in: %s\n", l->in);
		// if (l->out)
		// 	printf("out: %s\n", l->out);

		// /* Display each command of the pipe */
		// for (i = 0; l->seq[i] != 0; i++)
		// {
		// 	// char **cmd = l->seq[i];
		// 	// printf("seq[%d]: ", i);
		// 	//  for (j=0; cmd[j]!=0; j++) {
		// 	//  	printf("%s ", cmd[j]);
		// 	//  }
		// 	printf("\n");
		// 	run_cmd(l);
		// }
	}
}

void run_cmd(struct cmdline *cl){
	int fd1 = -1, fd2 = -1;

	char **cmd = cl->seq[0];
	if (cl->in != NULL){
		fd1 = open(cl->in, O_RDONLY, 0);
	}
	if (cl->out != NULL){
		fd2 = open(cl->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd2 == -1){
			fprintf(stderr, "%s: permission denied.\n", cl->out);
		}
	}
	pid_t pid = Fork();
	if (pid == 0){
		if (fd1 != -1){
			Dup2(fd1, STDIN_FILENO); 
			Close(fd1);
		}
		if (fd2 != -1){
			Dup2(fd2, STDOUT_FILENO);
			Close(fd2);
		}
		if (execvp(cmd[0], cmd) == -1)
		{
			fprintf(stderr, "%s: Command not found.\n", cmd[0]);
			exit(127);
		}
	}
	else{
		if (fd1 != -1) close(fd1);
		if (fd2 != -1) close(fd2);

		waitpid(pid, NULL, 0);
	}
}

void run_pipe2(struct cmdline *cl){
	int fd1 = -1, fd2 = -1;

	char **cmd1 = cl->seq[0];
	char **cmd2 = cl->seq[1];

	int p[2];

	if (cl->in != NULL){
		fd1 = Open(cl->in, O_RDONLY, 0);
	}
	if (cl->out != NULL){
		fd2 = Open(cl->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd2 == -1){
			fprintf(stderr, "%s: permission denied.\n", cl->out);
		}
	}

	pipe(p);

	pid_t pid = Fork();
	if (pid == 0){
		if (fd1 != -1){
			Dup2(fd1, STDIN_FILENO);
			Close(fd1);
		}
		Dup2(p[1], STDOUT_FILENO);
		Close(p[0]);
		Close(p[1]);

		if (execvp(cmd1[0], cmd1) == -1){
			fprintf(stderr, "%s: Command not found.\n", cmd1[0]);
			exit(2);
		}
	}

	else{
		pid_t pid2 = Fork();
		if (pid2 == 0){
			if (fd2 != -1){
				Dup2(fd2, STDOUT_FILENO);
				Close(fd2);
			}
			Dup2(p[0], STDIN_FILENO);
			Close(p[0]);
			Close(p[1]);

			if (execvp(cmd2[0], cmd2) == -1)
			{
				fprintf(stderr, "%s: Command not found.\n", cmd2[0]);
				exit(2);
			}
		}
		Close(p[0]);
		Close(p[1]);

		if (fd1 != -1) Close(fd1);
		if (fd2 != -1) Close(fd2);
		waitpid(pid, NULL, 0);
		waitpid(pid2, NULL, 0);
	}
}

void run_pipe(struct cmdline* cl){
	//Count commands.
	int n =0;
	while(cl->seq[n]) n++;

	//Variables to stock possible file descriptors.
	int fd_in = -1, fd_out = -1;

	//Check if redirection.
	if(cl->in != NULL) fd_in = open(cl->in, O_RDONLY, 0);
	if(cl->out != NULL) fd_out = open(cl->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	int prev_read = -1;
	pid_t* pids = malloc(n * sizeof(pid_t));
	if(pids == NULL) unix_error("malloc");

	for(int i = 0; i < n; i++){

		//Pipe creation.
		int p[2] = {-1, -1};
		if(i < n - 1){
			pipe(p);
		}

		pid_t pid = fork();
		if(pid == 0){
			//If its the first command, use fd_in as input.
			if( i == 0 && fd_in != -1) dup2(fd_in, STDIN_FILENO);
			//if command > 0, redirect output to next command's input
			else if( i > 0 && prev_read != 1) dup2(prev_read, STDIN_FILENO);

			//If output redirection for the last command, use fd_out as output.
			if(i == n - 1 && fd_out != -1) dup2(fd_out, STDOUT_FILENO);
			//else if not the last command, redirect command output to the pipe.
			else if(i < n - 1) dup2(p[1], STDOUT_FILENO);

			if (p[0] != -1) close(p[0]);
            if (p[1] != -1) close(p[1]);
            if (prev_read != -1) close(prev_read);
            if (fd_in != -1) close(fd_in);
            if (fd_out != -1) close(fd_out);

			execvp(cl->seq[i][0], cl->seq[i]);
			fprintf(stderr, "%s : command not found", cl->seq[i][0]);
			exit(127);
		}
		//Save the child pid for the coming waitpid.
		pids[i] = pid;
		if(prev_read != -1) Close(prev_read);
		if(p[1] != -1) Close(p[1]);
		//Save pipe entry in prev_read for the next command.
		prev_read = p[0];
	}
	if(prev_read != -1) Close(prev_read);
	if(fd_in != -1) Close(fd_in);
	if(fd_out != -1) Close(fd_out);
	
	//Wait for all the child to finish.
	for(int i = 0; i < n; i++) waitpid(pids[i], NULL, 0);
	free(pids);
}