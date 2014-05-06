/*
* NAME
*   digenv - prints sorts and filters envoriment variables
* 
* SYNOPSIS
*   digenv [grep arguments]
* 
* DESCRIPTION
*   
* 
* Author
*   Paul Griffin, pgriffin@kth.se
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>

int execute(char ** programs, int isBackground);
char ** tokenize(char * input,int count);
void printStringList(char **list);
int processHasTerminated(int pid);


typedef struct node_t {
	struct node_t *next;
	int val;
} node_t;

node_t * addFirst(node_t *root, int val) ;
node_t * filterList(node_t *root, int(*filter)(int)) ;

node_t *childList = NULL;

int printFilter(int val) {
	printf("Pid: %d\n", val);
	return 1;
}

void printList(node_t *root) {
	filterList(root, &printFilter);	
}


void
handleSignal(int signum)
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
	char in[70]="";
	signal(SIGINT, handleSignal);
	while (1)
	{
		printf("> ");
		fgets(in,70,stdin);
		childList = filterList(childList, &processHasTerminated);
		char **args = tokenize(in,0);
		int lastArgIndex = -1;
		for (;args[lastArgIndex+1];lastArgIndex++);
		char *command=args[0];
		char *lastArg = args[lastArgIndex];
		
		int isBackground;
		if (command!=NULL)
		{
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
				execute(args, isBackground);
			}
		}
		free(args);
		

	}
	return 0;
}

/** execute
 *exit
 * execute returns 0 if ok error code if not
 *
 * Def:
 *     
 *
 */
int
execute(
	char* args[],
	int isBackground)	/* list of strings. The first string is the program to execute the rest are arguments to the program. End with Null */
{
	char * program = args[0];

	struct timeval startTime;
	struct timeval endTime;

	int childPid=fork();
	if (childPid==-1){ fprintf(stderr, "Could not fork\n"); return 1;}
	if (!childPid) {
		if (execvp(program,args)==-1) exit(1);
	} else {
		if (isBackground){
			childList = addFirst(childList, childPid);
		}else{
			gettimeofday(&startTime, NULL);
			int ret, statval;
			ret = wait(&statval); 	/* wait for child to exit */
			if (ret==-1) { fprintf(stderr, "Failed to get return value from child"); }
			if (statval!=0) { fprintf(stderr, "Error with code %d from %s\n",statval,program); }
			gettimeofday(&endTime, NULL);
			printf("Time elapsed: %ld\n", endTime.tv_sec - startTime.tv_sec);
		}
	}
	return 0;
}

void printStringList(char **list) {
	
	printf("Printing this beautiful list\n");
	for (;*list; list++) {
		printf("%s\n", *list);
	}
}

char ** tokenize(char * input,int count)
{
	char * tok = strtok(input," \t\n\0");
	char ** result;
	if (tok != NULL)
	{
		result = tokenize(NULL,count+1);
		result[count] = tok;
	}else{
		result = malloc(sizeof(char*)*(count+1));
		result[count] = NULL;

	}
	return result;
}

int processHasTerminated(int pid) {
	int statval;
	int ret;
    ret = waitpid(pid, &statval, WNOHANG);
	int hasExited = ret == 0 ? 1 : 0;
	if (!hasExited) printf("Process %d exited with status %d\n", pid, statval);
	return hasExited;
}

node_t * addFirst(node_t *root, int val) {
	node_t *newNode = malloc(sizeof(node_t));
	newNode->val = val;
	newNode->next = root;
	return newNode;
}

node_t * filterList(node_t *root, int(*filter)(int)) {
	node_t *current = root;
	node_t *prev = root;
	while (current) {
		if (!filter(current->val)) {
			if (current == root) {
				root = current->next;
				prev = root;
			}else{
				prev->next = current->next;
			}
			node_t *tmp = current;
			current = current->next;			
			free(tmp);
		}else{
			prev = current;
			current = current->next;			
		}

	}
	return root;
}
