#include<stdio.h>

int main(){
	char *input = NULL;
	size_t len = 0;
	getline(&input, &len, stdin);
	puts(input);
	return 0;
}
