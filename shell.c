#include<stdio.h>
#include<stdlib.h>

int main(){
	char *input = NULL;
	size_t len = 0;
	while(getline(&input, &len, stdin) != -1){
		system(input);
	}
	free(input);
	return 0;
}
