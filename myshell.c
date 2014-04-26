#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define BUFFER_SIZE 10 // a buffer to store the history
int location = 0;
char history[MAX_LINE][BUFFER_SIZE];

/* the signal handler function */
void handle_SIGTSTP() {
	int i;
	write(STDOUT_FILENO,"\n",1);
	for(i=location; i-location<BUFFER_SIZE; i++)
	{
		printf("%i %s \n", i-location+1,history[i-location]);
	}
}

void addHistory(char Buff[], int Len)
{
	int i;
	if(Len > 1)
	{
		for(i=0; i<Len && i<MAX_LINE-2; i++)
		{
			history[location%BUFFER_SIZE][i]=Buff[i];
		}
		history[location%BUFFER_SIZE][i]='\0';
	}
	location++;
}

/**
* setup() reads in the next command line, separating it into distinct tokens
* using whitespace as delimiters. setup() sets the args parameter as a
* null-terminated string.
*/

void setup(char inputBuffer[], char *args[],int *background)
{
	int length, /* # of characters in the command line */
	i,      /* loop index for accessing inputBuffer array */
	start,  /* index where beginning of next command parameter is */
	ct;     /* index of where to place the next parameter into args[] */

	ct = 0;

	/* read what the user enters on the command line */
	length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 
	addHistory(inputBuffer,length);

	start = -1;
	if (length == 0)
	exit(0);            /* ^d was entered, end of user command stream */
	if (length < 0){
		perror("error reading the command");
		exit(-1);           /* terminate with error code of -1 */
	}

	/* examine every character in the inputBuffer */
	for (i=0;i<length;i++)
	{
		switch (inputBuffer[i])
		{
		case ' ':
		case '\t' :               /* argument separators */
			if(start != -1){
				args[ct] = &inputBuffer[start];    /* set up pointer */
				ct++;
			}
			inputBuffer[i] = '\0'; /* add a null char; make a C string */
			start = -1;
			break;
		case '\n':                 /* should be the final char examined */
			if (start != -1){
				args[ct] = &inputBuffer[start];    
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; /* no more arguments to this command */
			break;
			default :             /* some other character */
			if (start == -1)
			start = i;
			if (inputBuffer[i] == '&')
			{
				*background  = 1;
				start = -1;
				inputBuffer[i] = '\0';
			}
		}
	}   
	args[ct] = NULL; /* just in case the input line was > 80 */
}

int main(void)
{
	char inputBuffer[MAX_LINE];      /* buffer to hold the command entered */
	int background;              /* equals 1 if a command is followed by '&' */
	char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */
	int myPid = getpid();		/* get pid*/
	int promptcount = 0;
	const char* whispString = "whisper";
	const char* exitString = "exit";
	const char* repeat = "r";
	int i;
	int j = 1;
	printf("Welcome to keshell.  My pid is %d\n", myPid);
	/* set up the signal handler */
	struct sigaction handler;
	handler.sa_handler = handle_SIGTSTP;
	handler.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &handler, NULL);
	/* wait for <control> <Z> */
	while (1){            /* Program terminates normally inside setup */
		background = 0;
		promptcount++;
		setbuf(stdout, NULL); // because my prompt statement is not newline terminated I need to set stdout buffer to auto flush.
		printf("keshell[%d]-->",promptcount);
		setup(inputBuffer,args,&background);       /* get next command */
		if(!(strcmp(whispString,args[0])))
		{
			char cPrint;
			while(args[j]){
				for(i = 0; i < (strlen(args[j])); i++)
				{
					cPrint = args[j][i];
					putchar(tolower(cPrint));
				}
				putchar(' ');
				j++;
			}
			j = 1;
			putchar('\n');
		}
		else if(!(strcmp(exitString,inputBuffer)))
		{
			char exitStr[100];
			sprintf(exitStr, "ps -p %d -o pid,ppid,pcpu,pmem,etime,user,command", myPid);
			system(exitStr);
			exit(0);
		}
		else if(!(strcmp(repeat,args[0])))
		{
			printf("go fuck yourself");
		}
		else
		{
			pid_t pid;
			pid = fork();
			int childPID = getpid();
			if(pid < 0){
				fprintf(stderr, "Fork Failed");
				return 1;
			}
			else if (pid == 0) 
			{
				char* bgFlag;
				
				if(background)
				{
					bgFlag = "TRUE";
				}
				else
				{
					bgFlag = "FALSE";
				}
				printf("[Child pid = %d, background = %s]\n", childPID, bgFlag);
				if((strcmp(args[0], "cd"))==0)
				{
					chdir(args[1]);
				}
				execvp(args[0],args);
				putchar('\n');
				exit(0);
			}
			else
			{
				if(!background)
				{
					waitpid(0);
					printf("Child process complete\n");
				}
			}
			
			/* the steps are:
	(0) if built-in command, handle internally
	(1) if not, fork a child process using fork()
	(2) the child process will invoke execvp()
	(3) if background == 0, the parent will wait,
			otherwise returns to the setup() function. */
		}
	}
}
