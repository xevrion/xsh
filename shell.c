#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void parse(char *cmd, char *args[]) {
	int i = 0;
	char *token = strtok(cmd, " ");
	while (token != NULL) {
		args[i++] = token;
		token = strtok(NULL, " ");
	}
	args[i] = NULL;
}

int main() {
	char *input = NULL;
	size_t len = 0;
	while (1) {
		printf("xsh> ");
		fflush(stdout);

		if (getline(&input, &len, stdin) == -1)
			break;

		input[strcspn(input, "\n")] = '\0';
		// system(input);
		//

		// pipe handling
		if (strchr(input, '|') != NULL) {
			char *cmd1 = strtok(input, "|");
			char *cmd2 = strtok(NULL, "|");

			char *args1[64], *args2[64];
			parse(cmd1, args1);
			parse(cmd2, args2);

			int fd[2];
			pipe(fd);

			pid_t p1 = fork();
			if (p1 == 0) {

				dup2(fd[1], 1);
				close(fd[0]);
				close(fd[1]);
				execvp(args1[0], args1);
				perror("exec cmd1");
				exit(1);
			}

			pid_t p2 = fork();
			if (p2 == 0) {

				dup2(fd[0], 0);
				close(fd[1]);
				close(fd[0]);
				execvp(args2[0], args2);
				perror("exec cmd2");
				exit(1);
			}

			close(fd[0]);
			close(fd[1]);
			wait(NULL); // wait for child 1
			wait(NULL); // wait for child 2

			continue;
		}

		// args handling
		char *args[64];
		char *token = strtok(input, " ");
		int i = 0;
		while (token != NULL) {
			args[i] = token;
			i++;
			token = strtok(NULL, " ");
		}

		args[i] = NULL;

		if (args[0] == NULL)
			continue; // empty line

		if (strcmp(args[0], "exit") == 0)
			break;

		if (strcmp(args[0], "cd") == 0) {
			chdir(args[1]);
			continue;
		}

		pid_t pid = fork();

		if (pid == 0) {
			// child
			execvp(args[0], args);
		} else {
			// parent
			wait(NULL);
		}
	}
	free(input);
	return 0;
}
