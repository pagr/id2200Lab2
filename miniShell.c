/*
* NAME
*   miniShell - A program that is a small shell
* 
* SYNOPSIS
*   miniShell
* 
* DESCRIPTION
*   A small command shell with two builtin commands, ls and exit. It reads commands
*   from standard input and returns response to standard output.
* 
* Author
*   Paul Griffin, pgriffin@kth.se
*   Sami Purmonen, purmonen@kth.se
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>



/* we included the Node type in the main file so it would be 
 * easier for you to compile
 */
 
/*
 * Node is a cell in which the List adt stores an element
 * and links to the next cell.
 *
 */
typedef struct Node {
	struct Node *next; /* Next node in linked list */
	int val;           /* Value stored in this node */
} Node;

/* List to store all child processes*/
Node *childProcessList = NULL;
/* Input command can at most be 70 letters long including newline */
#define commandMaxLength 70

/** addFirst
 * 
 * Adds val first to the root list
 *
 * Def: [val, root,...]
 * 
 *
 */
Node * 
addFirst(
	Node *root, /* The list to add too */
	int val) 	/* value to add */
{
	Node *newNode = malloc(sizeof(Node));
	if (!newNode) {fprintf(stderr,"Failed to allocate ram"); exit(1);};
	newNode->val = val;
	newNode->next = root;
	return newNode;
}

/** filerList
 *
 * Iterates across the list and removes elements where filer(node.value) is true
 *
 * Def: [node | node in root if filter(node)]
 *
 */
Node * 
filterList(
	Node *root, 		/* The list */
	int(*filter)(int)) 	/* A function that returns true to apply on all nodes */
{
	Node *current = root;
	Node *prev = root;
	/* Iterate until next node is null */
	while (current) {
		if (!filter(current->val)) {
			if (current == root) {
				/* handle first node specially*/
				root = current->next;
				prev = root;
			}else{
				prev->next = current->next;
			}
			Node *tmp = current;
			current = current->next;			
			free(tmp);
		}else{
			prev = current;
			current = current->next;			
		}
	}
	return root;
}

/** executeProgram
 *
 * executeProgram returns 0 if ok error code if not
 *
 * Def: executeProgram forks and executePrograms args[0] in the new process
 *      the main process the either waits for it to complete or
 *      adds it to a list of processes to check later.
 *     
 *
 */
int
executeProgram(
	char* args[],		/* list of strings. The first string is the program to execute the rest are arguments to the program. End with Null */
	int isBackground)	/* To wait for program or add to the list */
{
	char * program = args[0];
	
	/* we need to measure the execution time of the process if we run it in the foreground */
	struct timeval startTime;
	struct timeval endTime;
	int timeError=0;
	
	if (gettimeofday(&startTime, NULL)==-1) timeError=1;
	int childPid=fork();
	if (childPid==-1){ fprintf(stderr, "Could not fork\n"); return 1;}
	if (!childPid) {
		if (execvp(program,args)==-1) exit(1);
	} else {
		if (isBackground){
			printf("Executing %d in background\n",childPid);
			childProcessList = addFirst(childProcessList, childPid);
		}else{
			printf("Executing %d in foreground\n",childPid);
			int ret, childExitStatus;
			ret = wait(&childExitStatus); 	/* wait for child to exit */
			if (ret==-1) { fprintf(stderr, "Failed to get return value from child"); }
			

			if (gettimeofday(&endTime, NULL)==-1) timeError=1;
			if (timeError){
				printf("Foreground process %d executed in (error getting time) ms with return value %d\n",childPid, childExitStatus);			
			}else{
				long ms=(endTime.tv_sec - startTime.tv_sec)*1000 + (endTime.tv_usec - startTime.tv_usec)/1000;
				printf("Foreground process %d executed in %ld ms with return value %d\n",childPid, ms, childExitStatus);			
			}
		}
	}
	return 0;
}

/** processHasTerminated
 * 
 * Checks if a process has terminated. If it has it also prints some information.
 * 
 * Def: return 1 if process has terminated else 0
 */
int 
processHasTerminated(
	int pid) /*The process id to check*/
{
	int statval;
	int ret;
    ret = waitpid(pid, &statval, WNOHANG);
	int hasExited = ret == 0 ? 1 : 0;
	if (!hasExited) printf("Background process %d exited with status %d\n", pid, statval);
	return hasExited;
}


/** tokenizeHelper
 * 
 * tokenizeHelper takes a string and splits it on whitespace, newline and \0
 * recursively runs strtok and saves the result on the stack. When we 
 * reach the end of the string it creates a list and tails back and 
 * saves the values to this list.
 *
 * The returnvalue is allocated with malloc.
 * Def: inputString.split(/\t|\n|\0| /g)
 */
char ** 
tokenizeHelper(
	char * input,	/* a string to split */
	int count)		/* internal variable, set to 0 om first iteration */
{
	char * tok = strtok(input," \t\n\0");
	char ** result;
	if (tok != NULL)
	{
		result = tokenizeHelper(NULL,count+1);
		result[count] = tok;
	}else{
		/*there are no more tekons in the strig. Create lsit and return*/
		result = malloc(sizeof(char*)*(count+1));
		/* The last element is null so that we know how long the lsit is*/
		result[count] = NULL;
	}
	return result;
}

/** tokenize
 * 
 * tokenize takes a string and splits it on whitespace, newline and \0
 * Internally it uses tokenizeHelper
 *
 */
char ** 
tokenize(
	char * input)	/* a string to split */
{
	return tokenizeHelper(input,0);
}

/* handleSignal
*
* handleSignal is an empty function because we dont want to do anything when we recieve a signal
*
* Def: Do nothing on signal
*/
void
handleSignal(
	int signum)/*The signal to handle*/
{
	
}

/** main
 *
 * main returns 0 if ok error code if not
 *
 *
 * Def:
 *   
 *
 */
int
main(
	int argc,	/* Number of arguments passed in argv*/
	char *argv[])	/* Command line arguments*/
{

	char in[commandMaxLength+1]="";
	
	/* Make sure we don't break on Ctrl-C */
	signal(SIGINT, handleSignal);
	char prompt[] = "> ";
	while (1)
	{

		printf("%s",prompt);
		fgets(in, commandMaxLength+1, stdin); /* Read command */
		
		/* In each iteration we remove zombies */
		childProcessList = filterList(childProcessList, &processHasTerminated);

		/* Split input string into list of tokens allocated with malloc */
		char **args = tokenize(in);

		/* Last arg is needed for detecting background symbol & */
		int lastArgIndex = -1;
		for (;args[lastArgIndex+1];lastArgIndex++);

		/* First token in args is the actual command */
		char *command=args[0];
		char *lastArg = args[lastArgIndex];
		int isBackground;
		if (command!=NULL)
		{
			/* CD is a built in comand to change directory
			 * if it does not exist go to users home directory
                         */
			if (!strcmp(command,"cd")) {
				if (chdir(args[1])) {
					chdir(getenv("HOME"));
				}
			} else if (!strcmp(command,"exit")) {
				free(args);
				return 0;
			} else {
				isBackground = lastArg[strlen(lastArg)-1] == '&';
				if (isBackground) {
					lastArg[strlen(lastArg)-1] = 0; 
				}
				executeProgram(args, isBackground);
			}
		}
		free(args);
		

	}
	return 0;
}






