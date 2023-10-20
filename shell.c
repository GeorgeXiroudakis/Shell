//csd5067  -  George Xiroudakis

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LEN_OF_HOSTE_NAME 256
#define MAX_LEN_OF_WORKING_DIR 1024
#define MAX_LEN_OF_PROMPT 1024

void display_prompt();
void revomeNewLines(char* input);
void revomeExtraSpaces(char* input);
int countInstances(char *str, char key);
int isInStr(char *str, char key);
void handle_instruction(char *instruction);
void handle_pipe(char *intraction);
void exceutePipes(char **commands);
void handle_redirect(char *instruction, char *redirectChar);
void ececute_inFile_redirect(char *command, char *fileName, int apend);
void ececute_outOfFile_redirect(char *command, char *fileName);


int main(int argc, char **argv){
	char prompt[MAX_LEN_OF_PROMPT];
	char **instructions;
	char *instruction;
	const char semicolon_str[] = ";";	
	int numOfInstructions = 0;

	//infinite loop to get prompts
	while(1){
		display_prompt();

		//get prompt    	
		fgets(prompt, sizeof(prompt), stdin);
		revomeNewLines(prompt);

		//mallloc space according to how many instructions we have
		instructions = (char **)malloc((countInstances(prompt, ';') + 1) * sizeof(char *));
		if(instructions == NULL){fprintf(stderr, "error in malloc for instructions"); exit(EXIT_FAILURE);}

		//tokenize the prompt in instruction with;
		instruction = strtok(prompt, semicolon_str);
		while( instruction != NULL ) {
			revomeExtraSpaces(instruction);
			//save each token/instruction
			instructions[numOfInstructions++] = instruction; 
			
			//advans strtok
			instruction = strtok(NULL, semicolon_str);
		}
		
		//iterate though the instructions given and 
		//excecute them one by one.
		for(int i = 0; i < numOfInstructions; i++){
			handle_instruction(instructions[i]);
		}

		//reset then num of intractions is each prompt
		numOfInstructions = 0;

		//free malloced memmory
		for(int i = numOfInstructions; i < 0; i--){
			free(instructions[i]);
		}
		free(instructions);
	}	

	return 0;
}



void display_prompt(){
	char hostName[MAX_LEN_OF_HOSTE_NAME];
	char cwd[MAX_LEN_OF_WORKING_DIR];
	
	//get host name
	if( gethostname(hostName, sizeof(hostName)) != 0 ){
		printf("\033[0;32m");
		fprintf(stderr, "error in gethostname function\n");
		exit(EXIT_FAILURE);
	}
	
	//get current working directory
	if ( getcwd(cwd, sizeof(cwd) ) == 0 ) {
		printf("\033[0;34m");
		fprintf(stderr, "error in getcwd function\n");
		exit(EXIT_FAILURE);

	}

	//print prompt
	
	printf("\033[0;32m");
	printf("csd5067-hy345sh@");

	//printf("\033[0;35m");
	printf("%s", hostName);

	printf("\033[0;30m");
	printf(":");

	printf("\033[0;34m");
	printf("%s", cwd);

	printf("\033[0m");
	printf("$ ");
}


//revoves the new line chars form a string 
//maybe i will need to remove other withspaces too 
void revomeNewLines(char* input){
	int i,j = 0;
	int input_len = strlen(input);

	//iterate over original input and 
	//if we keep it we overight the str
	//so i iterates the original and 
	//j overates the one we want on the original
	for(i = 0; i < input_len; i++){
		if(input[i] != '\n'){
			input[j++] = input[i];
		}
	}
	//add null terminator at the end of the str
	input[j] = '\0';
}

void revomeExtraSpaces(char* input){
	char *dest = input;
	
	//skip spaces in the start
	while(*input == ' ')input++;

    while (*input != '\0')
    {
        while (*input == ' ' && *(input + 1) == ' ')
            input++;  
       *dest++ = *input++;
    }
    *dest = '\0';
}


//counts how many time the key apears in the str
int countInstances(char *str, char key){
	int count = 0;

	while (*str != '\0'){
		if(*str == key) count++;

		str++;
	}

	return count;
}


//checks if the key apears in the str
int isInStr(char *str, char key){
	while (*str != '\0'){
		if(*str == key) return 1;

		str++;
	}

	return 0;
}


//checks if there is a redirect char in str
//return:
//		the address of the redirect char if it exists.
//		'\0' it str contains non. 
char* find_redirect_char(char *str){
	while (*str != '\0'){
		if( (*str == '<') || (*str == '>')) break;

		str++;
	}

	return str;
}


// excecutes an instruction by greating a new prosses
//args:
//	intraction: string of intraction to handle
void handle_instruction(char *instruction){
	int status;
	char instruction_copy[MAX_LEN_OF_PROMPT];
    strcpy(instruction_copy, instruction);
	char *redirectChar;

	//checking if we got a pipe
	if(isInStr(instruction, '|')){
		handle_pipe(instruction);	
		return;
	}

	//cheking if we got redirect
	redirectChar = find_redirect_char(instruction);
	if(*redirectChar != '\0'){
		handle_redirect(instruction, redirectChar);
		return;
	}

	//size for MAX_LEN_OF_ARGS_IN_A_instruction string + 1 for the name of the instruction
	char **args = (char **)malloc(sizeof(char *) * 2);
	if(args == NULL){fprintf(stderr, "error in malloc for args"); exit(EXIT_FAILURE);}
	int current_args = 0;	
	char *space_str = " ";
	char *arg = strtok(instruction_copy, space_str);

	while(arg !=  NULL){
		args = (char **)realloc(args, (current_args + 2) * sizeof(char *));
		if(args == NULL){fprintf(stderr, "error in realloc for args"); exit(EXIT_FAILURE);}

		args[current_args++] = arg;
		arg = strtok(NULL, space_str);
	}
	args[current_args] = NULL;

	//check if the insraction is quit or chdir wich dont need a separate process
	if( (strcmp(args[0], "quit") == 0) || (strcmp(args[0], "exit") == 0))exit(EXIT_SUCCESS);
	if( (strcmp(args[0], "chdir") == 0) || (strcmp(args[0], "cd") == 0) ) {chdir(args[1]); return;}

	//fork a nea process
	pid_t pid = fork();

	//child process
	if(pid == 0){
		execvp(args[0], args);

		fprintf(stderr, "%s: command not found\n", args[0]);
		exit(EXIT_FAILURE);

	}
	//parent process
	else if(pid > 0){
		//wait for child prosses
		waitpid(-1, &status, 0);

	}else{
		fprintf(stderr, "error in fork");
		exit(EXIT_FAILURE);
	}
	
	// for (int i = 0; i < current_args; i++) {
    //     free(args[i]);
    // }
	// free(args);
}


void handle_pipe(char *instruction){
	int countOfPipeInstructions = 0;	

	char **pipeInstructions = (char **)malloc((countInstances(instruction, '|') + 2) * sizeof(char *)); // +2 because
																										//one | means two comants two | means three commant e.x.
																										// and one for the NULL at the end 
	if(pipeInstructions == NULL){fprintf(stderr, "error in malloc for pipeInstrtuctions"); exit(EXIT_FAILURE);}
	char *line_str = "|";
	char *pipeInstruction = strtok(instruction, line_str);

	while(pipeInstruction !=  NULL){
		pipeInstructions[countOfPipeInstructions++] = pipeInstruction;
		pipeInstruction = strtok(NULL, line_str);
	}
	pipeInstructions[countOfPipeInstructions] = NULL;

	exceutePipes(pipeInstructions);	

	for (int i = countOfPipeInstructions; i < 0; i--) {
    	free(pipeInstructions[i]);
	}
	free(pipeInstructions);

	return;
}


void exceutePipes(char **commands){
	int status;

	//the input and output ends file dicriptors of the pipes
	int pipefd[2];
	pid_t pid;
	//a file dicriptor to save the output of the prevous
	//commant to make the input for the next one
	int prevFd = 0;

	//for tokenizing in args
	char **args = (char **) malloc(sizeof(char *) * 2) ;
	char commands_copy[MAX_LEN_OF_PROMPT];
	int current_args = 0;
	char *space_str = " ";
	char *arg = NULL;

	for(int i = 0; commands[i] != NULL; i++){
		//open pipe
		if(pipe(pipefd) == -1){
			fprintf(stderr, "error in pipe creation\n");
			exit(EXIT_FAILURE);
		}

		//create new process to ececute the commant
		if( ( pid = fork() ) == -1){
			fprintf(stderr, "error in fork in excecute pipes\n");
			exit(EXIT_FAILURE);
		}

		//child process
		if(pid == 0){
			//close the read end of the pipe as we dont need it
			close(pipefd[0]);

			//if its not the first one
			if(i != 0){
				//we dup its std input as the prev_fd(the output of the previous commant)
				//STDIN_FILENO resolves to 0 from unistd.h
				dup2(prevFd, STDIN_FILENO);
				//close the prevfd as we dont need it anymore in this process
				close(prevFd);
			}

			//if its not the last commant
			if(commands[i + 1] != NULL){
				//we dup its std output to the to the writing end of the pipe
				//STDOUT_FILENO resolves to 1 from unistd.h
				dup2(pipefd[1], STDOUT_FILENO);
			}

			//either way close the writing end of the pipe as we dont need it anymore
			//in this process
			close(pipefd[1]);

    		strcpy(commands_copy, commands[i]);

			//for if there is redirection in the pipe
			
			//cheking if we got redirect
			char *redirectChar = find_redirect_char(commands_copy);
			if(*redirectChar != '\0'){
				handle_redirect(commands_copy, redirectChar);
				exit(EXIT_SUCCESS);
			}

			// Tokenize the command into separate arguments

			//reinisialize current_args for the new values
			current_args = 0;

			arg = strtok(commands_copy, space_str);
            while (arg != NULL) {
				args = (char **)realloc(args, (current_args + 2) * sizeof(char *));
				if(args == NULL){fprintf(stderr, "error in realoc for args in pipe"); exit(EXIT_FAILURE);}

                args[current_args++] = arg;
                arg = strtok(NULL, space_str);
            }
            args[current_args] = NULL;

			//execute the commant
			execvp(args[0], args);

			fprintf(stderr, "%s: command not found\n", args[0]);
			exit(EXIT_FAILURE);
		}
		//parent proccess
		else{
			//closing the writing pipe as we dont need it in this process
			close(pipefd[1]);

			//wait for child process
			waitpid(pid, &status, 0);

			//save the reading end of the pipe to the prev file discriptor 
			prevFd = pipefd[0];

		}
	}

	for (int i = current_args; i < 0; i--){
    	free(args[i]);
	}
	free(args);

	return;
}


void handle_redirect(char *instruction, char *redirectChar){

	char *command;
	char *fileName;
	char redirectCharStr[2];
	redirectCharStr[0] = *redirectChar;
	redirectCharStr[1] = '\0';
	int instructionLen = strlen(instruction);
	char instructionCopy[MAX_LEN_OF_PROMPT];
	strcpy(instructionCopy, instruction);

	//check for valid instraction
	if( (countInstances(instruction, '<') > 1) || ( (*redirectChar == '>')  && (countInstances(instruction, '>') > 2) ) ){
		fprintf(stderr, "invalid use of redirection sympols. correct uses:\na < b : redirect b as input of b\n"
				"a > b overwright file b with output of a\na >> b apend on file b with output of a\n");
		
		return;
	}

	//extract the command from the instruction
	command = (char *)malloc(sizeof(char) * instructionLen);
	strncpy(command, strtok(instructionCopy, redirectCharStr), instructionLen);

	//extract fileName
	fileName = (char *)malloc(sizeof(char) * instructionLen);
	strncpy(fileName, strtok(NULL, redirectCharStr), instructionLen);
	revomeExtraSpaces(fileName);

	if(*redirectChar == '>') ececute_inFile_redirect(command, fileName, ( (*redirectChar == '>') && (*(redirectChar + 1) == '>') ));
	if(*redirectChar == '<') ececute_outOfFile_redirect(command, fileName);
	
	
	free(command);
	free(fileName);

	return;
}


void ececute_inFile_redirect(char *command, char *fileName, int apend){
	//save original stdin
	int original_stdin_fd = dup(STDIN_FILENO);

	//the input and output ends file dicriptors of the pipes
	int pipefd[2];

	//for fork
	pid_t pid;
	int status;

	//for tokenizing in args
	char **args = (char **) malloc(sizeof(char *) * 2) ;
	char commandCopy[MAX_LEN_OF_PROMPT];
	int current_args = 0;
	char *space_str = " ";
	char *arg = NULL;

	//for file
	FILE *file;
	char inputChar;


	//open pipe
	if(pipe(pipefd) == -1){
		fprintf(stderr, "error in pipe creation\n");
		exit(EXIT_FAILURE);
	}


	//create new process to ececute the commant
	if( ( pid = fork() ) == -1){
		fprintf(stderr, "error in fork in excecute redirect\n");
		exit(EXIT_FAILURE);
	}
	
	//child process
	if(pid == 0){
		//close the reading end of the pipe as we dont need it in this process
		close(pipefd[0]);

		//dup the ouput of the process to the write end of the pipe
		//STDOUT_FILENO resolves to 1 from unistd.h
		dup2(pipefd[1], STDOUT_FILENO);

		//close writing end of pipe as we dont need it anymore in this process
		close(pipefd[1]);

		// Tokenize the command into separate arguments
		strcpy(commandCopy, command);

		arg = strtok(commandCopy, space_str);
		while (arg != NULL) {
			args = (char **)realloc(args, (current_args + 2) * sizeof(char *));
			if(args == NULL){fprintf(stderr, "error in realoc for args in pipe"); exit(EXIT_FAILURE);}

			args[current_args++] = arg;
			arg = strtok(NULL, space_str);
		}
		args[current_args] = NULL;

		//execute the commant
		execvp(args[0], args);

		fprintf(stderr, "%s: command not found\n", args[0]);
		exit(EXIT_FAILURE);
	}
	//parent process
	else{
		//close the writing end of the pipe as we dont need it in this process
		close(pipefd[1]);

		//we dup its std input as the reading end of the pipe
		//STDIN_FILENO resolves to 0 from unistd.h
		dup2(pipefd[0], STDIN_FILENO);

		//open file and is we got > overwright or apend accoiding to if the is > or >>
		file = fopen(fileName, apend ? "a" : "w");
		if(file == NULL ){fprintf(stderr, "Failed to open the file: %s", fileName); exit(EXIT_FAILURE);}

		// Read characters from stdin wich is dupeed to the reading end of the pipe and write them to the file
		while (read(pipefd[0], &inputChar, 1) > 0) {
    		fputc(inputChar, file);
		}

		// Close the file
		fclose(file);

		//close the reading end of the pipe as we dont need it in this process
		close(pipefd[0]);

		//wait for child process
		waitpid(pid, &status, 0);
	}

	//free alocated space
	for (int i = current_args; i < 0; i--){
    	free(args[i]);
	}
	free(args);

	//Restore stdin to original
	dup2(original_stdin_fd, STDIN_FILENO);
	//Close the duplicated fd
	close(original_stdin_fd);

	return;
}


void ececute_outOfFile_redirect(char *command, char *fileName){
	//save original stdin
	int original_stdout_fd = dup(STDOUT_FILENO);

	//the input and output ends file dicriptors of the pipes
	int pipefd[2];

	//for fork
	pid_t pid;
//	int status;

	//for tokenizing in args
	char **args = (char **) malloc(sizeof(char *) * 2) ;
	char commandCopy[MAX_LEN_OF_PROMPT];
	int current_args = 0;
	char *space_str = " ";
	char *arg = NULL;

	//for file
	FILE *file;
	char inputChar;


	//open pipe
	if(pipe(pipefd) == -1){
		fprintf(stderr, "error in pipe creation\n");
		exit(EXIT_FAILURE);
	}

	//create new process to ececute the commant
	if( ( pid = fork() ) == -1){
		fprintf(stderr, "error in fork in excecute redirect\n");
		exit(EXIT_FAILURE);
	}

	//child process
	if(pid == 0){
		//close the writing end of the pipe as we dont need it in this process
		close(pipefd[1]);

		//dup the input of the process to the read end of the pipe
		//STDIN_FILENO resolves to 0 from unistd.h
		dup2(pipefd[0], STDIN_FILENO);

		//close reading end of pipe as we dont need it anymore in this process
		close(pipefd[0]);

		// Tokenize the command into separate arguments
		strcpy(commandCopy, command);

		arg = strtok(commandCopy, space_str);
		while (arg != NULL) {
			args = (char **)realloc(args, (current_args + 2) * sizeof(char *));
			if(args == NULL){fprintf(stderr, "error in realoc for args in pipe"); exit(EXIT_FAILURE);}

			args[current_args++] = arg;
			arg = strtok(NULL, space_str);
		}
		args[current_args] = NULL;

		//execute the commant
		execvp(args[0], args);

		fprintf(stderr, "%s: command not found\n", args[0]);
		exit(EXIT_FAILURE);
	}
	//parent process
	else{
		//close the reading end of the pipe as we dont need it in this process
		close(pipefd[0]);

		//we dup its std ouput as the writing end of the pipe
		//STDOUT_FILENO resolves to 1 from unistd.h
		dup2(pipefd[1], STDOUT_FILENO);

		//open file and is we got > overwright or apend accoiding to if the is > or >>
		file = fopen(fileName, "r");
		if(file == NULL ){fprintf(stderr, "Failed to open the file: %s", fileName); exit(EXIT_FAILURE);}

		//Read characters from the file and print them to the stdout wich is duped to the writing end of the pipe
		while ((inputChar = fgetc(file)) != EOF) {
        	fputc(inputChar, stdout);
    	}

		// Close the file
		fclose(file);

		//close the reading end of the pipe as we dont need it in this process
		close(pipefd[1]);

		//wait for child process
		//waitpid(pid, &status, 0);    //maybe its already done because we want on it for the input
	}

	//free alocated space
	for (int i = current_args; i < 0; i--){
    	free(args[i]);
	}
	free(args);

	//Restore stdout to original
	dup2(STDOUT_FILENO, original_stdout_fd);
	//Close the duplicated fd
	close(original_stdout_fd);

	return;
}

