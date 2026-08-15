#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include<fcntl.h>
#include <wchar.h>

void parse(char *cmd, char *args[]) {
	int i = 0;
	char *token = strtok(cmd, " ");
	while (token != NULL) {
		args[i++] = token;
		token = strtok(NULL, " ");
	}
	args[i] = NULL;
}

// trim spaces off both ends of a filename
char *trim(char *s) {
	while (*s == ' ') s++;

	char *end = s + strlen(s);
	while (end > s && end[-1] == ' ') end--;
	*end = '\0';

	return s;
}

// pull "> file" / ">> file" / "< file" off the end of cmd and hook up the fds.
// runs in the child, so a bad open just kills that child.
// returns -1 if a file could not be opened.
int redirect(char *cmd) {
	char *out = strchr(cmd, '>');
	char *in = strchr(cmd, '<');

	if (out != NULL) {
		int append = (out[1] == '>'); // ">>" not ">"
		char *file = out + (append ? 2 : 1);

		*out = '\0'; // cut the redirection off the command
		file = trim(file);

		int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
		int f = open(file, flags, 0644);
		if (f == -1) { perror("open"); return -1; }
		dup2(f, 1);
		close(f);
	}

	if (in != NULL) {
		*in = '\0';
		char *file = trim(in + 1);

		int f = open(file, O_RDONLY);
		if (f == -1) { perror("open"); return -1; }
		dup2(f, 0);
		close(f);
	}

	return 0;
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

		// pipe handling (N-pipes)
		if (strchr(input, '|') != NULL) {
			char *cmds[64];
			int n = 0;

			char *cmd = strtok(input, "|");
			while (cmd != NULL) {
				cmds[n++] = cmd;
				cmd = strtok(NULL, "|");
			}

			int in = 0; // read end of the previous pipe, stdin for the first cmd

			for (int j = 0; j < n; j++) {
				int fd[2];
				if (j < n - 1)
					pipe(fd);

				pid_t p = fork();
				if (p == 0) {
					if (in != 0) {
						dup2(in, 0);
						close(in);
					}
					if (j < n - 1) {
						dup2(fd[1], 1);
						close(fd[0]);
						close(fd[1]);
					}
					// a stage's own < / > beats the pipe
					if (redirect(cmds[j]) == -1)
						exit(1);

					char *args[64];
					parse(cmds[j], args);
					execvp(args[0], args);
					perror("exec");
					exit(1);
				}

				if (in != 0)
					close(in);
				if (j < n - 1) {
					close(fd[1]);
					in = fd[0]; // next cmd reads from here
				}
			}

			for (int j = 0; j < n; j++)
				wait(NULL); // wait for every child

			continue;
		}

		// args handling
		// builtins get checked on a copy, since redirect() has to chop up
		// the real input inside the child
		char copy[1024];
		strncpy(copy, input, sizeof(copy) - 1);
		copy[sizeof(copy) - 1] = '\0';

		char *args[64];
		char *token = strtok(copy, " ");
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
			if (redirect(input) == -1)
				exit(1);

			parse(input, args);
			execvp(args[0], args);
			perror("exec");
			exit(1);
		} else {
			// parent
			wait(NULL);
		}
	}
	free(input);
	return 0;
}
