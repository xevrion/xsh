#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include<fcntl.h>
#include <signal.h>

// ANSI colors. \033 is ESC; the terminal eats "ESC[...m" instead of
// printing it. RESET turns styling back off, else it bleeds onto
// everything printed after.
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define RED "\033[1;31m"
#define RESET "\033[0m"

// perror, but red. colors go to stderr so they wrap the whole message.
void err(char *msg) {
	fputs(RED, stderr);
	perror(msg);
	fputs(RESET, stderr);
}

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
		if (f == -1) { err("open"); return -1; }
		dup2(f, 1);
		close(f);
	}

	if (in != NULL) {
		*in = '\0';
		char *file = trim(in + 1);

		int f = open(file, O_RDONLY);
		if (f == -1) { err("open"); return -1; }
		dup2(f, 0);
		close(f);
	}

	return 0;
}

int main() {
	char *input = NULL;
	size_t len = 0;

	// Ctrl-C at the prompt shouldn't kill the shell. children undo this
	// with SIG_DFL after fork, else they'd inherit the ignore and become
	// uninterruptible.
	signal(SIGINT, SIG_IGN);

	while (1) {
		// rebuilt every loop, so it follows cd
		char cwd[1024];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			char *home = getenv("HOME");
			int n = home ? strlen(home) : 0;

			// /home/me/Coding -> ~/Coding, but don't turn
			// /home/melissa into ~lissa
			if (n > 0 && strncmp(cwd, home, n) == 0 &&
			    (cwd[n] == '/' || cwd[n] == '\0'))
				printf(GREEN "xsh " BLUE "~%s" RESET "> ", cwd + n);
			else
				printf(GREEN "xsh " BLUE "%s" RESET "> ", cwd);
		} else {
			printf(GREEN "xsh" RESET "> "); // cwd gone (deleted?), fall back
		}
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
					signal(SIGINT, SIG_DFL); // undo the shell's ignore

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
					err("exec");
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
		// builtins get checked on a copy, since redirect() has to chop up the real input inside the child
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
			// bare "cd" or "cd ~" both mean $HOME. the shell expands
			// ~, not the kernel, so chdir("~") would just fail.
			char *dir = args[1];
			if (dir == NULL || strcmp(dir, "~") == 0)
				dir = getenv("HOME");

			if (dir == NULL || chdir(dir) == -1)
				err("cd");
			continue;
		}

		pid_t pid = fork();

		if (pid == 0) {
			// child
			signal(SIGINT, SIG_DFL); // undo the shell's ignore

			if (redirect(input) == -1)
				exit(1);

			parse(input, args);
			execvp(args[0], args);
			err("exec");
			exit(1);
		} else {
			// parent
			wait(NULL);
		}
	}
	free(input);
	return 0;
}
