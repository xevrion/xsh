#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>

int main(){
	char *input = NULL;
	size_t len = 0;
	while(getline(&input, &len, stdin) != -1){
		input[strcspn(input, "\n")] = '\0';
		// system(input);
		//

		// args handling
		char *args[64];
		char *token = strtok(input, " ");
		int i =0;
		while(token != NULL){
			args[i] = token;
			i++;
			token = strtok(NULL, " ");
		}

		args[i] = NULL;

		if(args[0] == NULL) continue; // empty line

		if(strcmp(args[0], "exit") == 0) break;

		if(strcmp(args[0], "cd") == 0){
			chdir(args[1]);
			continue;
		}


		pid_t pid = fork();

		if(pid==0){
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
