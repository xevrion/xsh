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

		pid_t pid = fork();

		if(pid==0){
			// child
			execlp(input, input, NULL);
		} else {
			// parent
			wait(NULL);
		}
	}
	free(input);
	return 0;
}
