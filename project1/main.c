/* CSci4061 F2018 Assignment 1
* login: cselabs login name (login used to submit) : duong142
* date: 09/23/18
* name: Berni D, Xiangyu Zhang, Peigen Luo(for partner(s))
* id: 5329383, 5422919, 5418155 */

// This is the main file for the code
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include "util.h"

#define MAX_TOKEN 256
#define CODE_1 1
/*-------------------------------------------------------HELPER FUNCTIONS PROTOTYPES---------------------------------*/
void show_error_message(char * ExecName);
//Write your functions prototypes here
// Warmup phrase 1: show infomation of list target
void show_targets(target_t targets[], int nTargetCount);

/* Input:
 *  - command: command line which is a string (Not a C type)/ array char
 * Purpose: executing any command line which is pass as "char command[]" variable
 * Return: integer: the status of execvp() after function called.
 */
int  execute(char command[]);

/* Input:
 *  - ts: original target lists
 *  - TargetIndex: the index of the current target need to be built
 *  - nTargetCount - number of target objects in Makefile
 *
 * Purpose: build the target from the bottom (leaf node) of the DAG target list
 * In other words, find the leaf targets, then call the execute()
 *
 * Return: none
 */
void build (target_t *ts, int TargetIndex,  int nTargetCount);
/*-------------------------------------------------------END OF HELPER FUNCTIONS PROTOTYPES--------------------------*/


/*-------------------------------------------------------HELPER FUNCTIONS--------------------------------------------*/

//This is the function for writing an error to the stream
//It prints the same message in all the cases

//Write your functions here

void build (target_t *ts, int TargetIndex,  int nTargetCount)
{
//**************************************************
// FINDING LEAFS PART
//**************************************************

  // Reach to each dependency and find where is the leaf of the tree to execute the command from the bottom to the top
  for (int i = 0; i < ts[TargetIndex].DependencyCount; i++)
  {
    int index = find_target(ts[TargetIndex].DependencyNames[i], ts, nTargetCount);
    if (index != ERROR) {
      // CASE: dependency is a target - e.g parse.o, cal.o
      build (ts, index, nTargetCount);
    }
    else if (does_file_exist(ts[TargetIndex].DependencyNames[i]) != ERROR) {
      // CASE: dependency is a target - e.g parse.o, cal.o
      // do nothing because at this point we found a leaf => go to execute
    } else {
        // CASE: dependency is no found. e.g aaaa.h, gdsgdsf.l
        printf("*** No rule to make target %s or Filename does not exist.  Stop.\n", ts[TargetIndex].DependencyNames[i]);   // ERROR HANDLING: no file
        exit(ERROR);
    }
  }

//*************************************************
// EXECUTE PART:
//*************************************************

//------------------------------------------------------------------------------------------------------------
// CASE 1: the target is not exist - run it as the first time , e.g main.o is not exist -> run > gcc -o main.c
//------------------------------------------------------------------------------------------------------------

  if (does_file_exist(ts[TargetIndex].TargetName) == ERROR) {
    // check the target is need to build or not - sometimes the target is run already but it does not has a file, e.g echo "Hi"
    if (ts[TargetIndex].Status == NEEDS_BUILDING || ts[TargetIndex].Status == UNFINISHED) {
      if (strlen(ts[TargetIndex].Command) > 0) {   // CHECK: length of command - command exist
        printf ("%s\n" , ts[TargetIndex].Command);   // print out the command
        int code =  execute(ts[TargetIndex].Command);  // execute the command
        //  CHECK: status of execute
        if (code) {
          // ERROR HANDLING: exec fail
          printf("*** FAILED: building target: %s\n", ts[TargetIndex].TargetName);
          ts[TargetIndex].Status == UNFINISHED;
          exit(ERROR);
        } else {
          // SUCCESS: execute
          ts[TargetIndex].Status = FINISHED;
        }
      } else {
        // CHECK: length of command <= 0 - command not exist
        ts[TargetIndex].Status = FINISHED;
      }
    }
  /*End CASE 1*/
  } else {

//------------------------------------------------------------------------------------------------------------
// CASE 2:  the target is not exist - run it as n-th times
//------------------------------------------------------------------------------------------------------------

    /* --------------CHECK: the finished time of the current file and its dependencies -----------------*/
    for (int i = 0; i < ts[TargetIndex].DependencyCount; i++) {
      int check = compare_modification_time( ts[TargetIndex].TargetName, ts[TargetIndex].DependencyNames[i]);
      if (check == NEEDS_BUILDING) {  // the current file need to be built
          ts[TargetIndex].Status = NEEDS_BUILDING;
          break;
      }
      else if (check == ERROR) {
          //  1 of the file does not exist  => run it any way
          if (ts[TargetIndex].Status != FINISHED) {
            ts[TargetIndex].Status = NEEDS_BUILDING;
            break;
          }
      } else {
          // 2 files have identical timestamp or target is modified later than dependency;
          ts[TargetIndex].Status = FINISHED;
      }
    }
    /********************* check the target is need to build or not *********************************/
    if (ts[TargetIndex].Status == NEEDS_BUILDING || ts[TargetIndex].Status == UNFINISHED) {
      if (strlen(ts[TargetIndex].Command) > 0) {    // CHECK: length of command - command exist
        // print out the command
        printf ("%s\n" , ts[TargetIndex].Command);
        // execute the command
        int code = execute(ts[TargetIndex].Command);
        //  CHECK: status of execute
        if (code == CODE_1) {
          printf("*** FAILED: building target: %s\n", ts[TargetIndex].TargetName); // ERROR HANDLING: exec fail
          ts[TargetIndex].Status = UNFINISHED;
          exit(ERROR);
        } else {
          // SUCCESS: execute
          ts[TargetIndex].Status = FINISHED;
        }
      } else {
        // CHECK: length of command <= 0 - command not exist
        ts[TargetIndex].Status = FINISHED;      // NO NEED to execute
      }
    } /*End Build target*/
  }   /*End CASE 2*/
}     /* End func */


int execute(char command[])
{
     pid_t  pid;
     int    status;

     if ((pid = fork()) < 0) {              /* fork a child process           */
          printf("*** ERROR: forking child process failed\n");
          exit(ERROR);
     }
     else if (pid == 0) {                    /* inside the child process:         */
          char *token[MAX_TOKEN];
          parse_into_tokens(command,token, " ");
          int x = execvp(*token, token);      /* execute the command  */
          if (x) {                          /*ERROR HANDLING if get into this case*/
               printf( "*** ERROR: exec failed!!! Invalid Command\n");
               exit(ERROR);
          }
     }
     else {                                 /* inside the parent process:      */
          wait(&status);
          if (WEXITSTATUS(status))
          {                                   /*ERROR HANDLING: child error if get into this case*/
              printf("Child exited with ERROR code = %d\n", WEXITSTATUS(status));
          }
          return WEXITSTATUS(status);     // return the status of child to print out the error message if have
     }
}


void show_error_message(char * ExecName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", ExecName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	exit(0);
}


//Phase1: Warmup phase for parsing the structure here. Do it as per the PDF (Writeup)
void show_targets(target_t targets[], int nTargetCount)
{
	//Write your warmup code here
	for (int i = 0; i < nTargetCount; i++)         // Walk through  the list of target
	{
		printf ("TargetName: %s\n", targets[i].TargetName);           // Printing out information
		printf ("DependencyCount: %d\n", targets[i].DependencyCount);
		printf ("DependencyNames: ");

		for (int k = 0; k < MAX_NODES; k++)
		{
			if ( strcmp(targets[i].DependencyNames[k],""))
				if ( strcmp(targets[i].DependencyNames[k+1],""))
					printf("%s, ", targets[i].DependencyNames[k]);
				else
					printf("%s", targets[i].DependencyNames[k]);
			else break;
		}
		printf ("\n");
		printf ("Command: %s\n", targets[i].Command);
	}

}

/*-------------------------------------------------------END OF HELPER FUNCTIONS-------------------------------------*/


/*-------------------------------------------------------MAIN PROGRAM------------------------------------------------*/
//Main commencement
int main(int argc, char *argv[])
{
  target_t targets[MAX_NODES];
  int nTargetCount = 0;

  /* Variables you'll want to use */
  char Makefile[64] = "Makefile";
  char TargetName[64];

  /* Declarations for getopt. For better understanding of the function use the man command i.e. "man getopt" */
  extern int optind;   		// It is the index of the next element of the argv[] that is going to be processed
  extern char * optarg;		// It points to the option argument
  int ch;
  char *format = "f:h";
  char *temp;

  //Getopt function is used to access the command line arguments. However there can be arguments which may or may not need the parameters after the command
  //Example -f <filename> needs a finename, and therefore we need to give a colon after that sort of argument
  //Ex. f: for h there won't be any argument hence we are not going to do the same for h, hence "f:h"
  while((ch = getopt(argc, argv, format)) != -1)
  {
	  switch(ch)
	  {
	  	  case 'f':
	  		  temp = strdup(optarg);
	  		  strcpy(Makefile, temp);  // here the strdup returns a string and that is later copied using the strcpy
	  		  free(temp);	//need to manually free the pointer
	  		  break;

	  	  case 'h':
	  	  default:
	  		  show_error_message(argv[0]);
	  		  exit(1);
	  }

  }

  argc -= optind;
  if(argc > 1)   //Means that we are giving more than 1 target which is not accepted
  {
	  show_error_message(argv[0]);
	  return -1;   //This line is not needed
  }

  /* Init Targets */
  memset(targets, 0, sizeof(targets));   //initialize all the nodes first, just to avoid the valgrind checks

  /* Parse graph file or die, This is the main function to perform the toplogical sort and hence populate the structure */
  if((nTargetCount = parse(Makefile, targets)) == -1)  //here the parser returns the starting address of the array of the structure. Here we gave the makefile and then it just does the parsing of the makefile and then it has created array of the nodes
	  return -1;


  //Phase1: Warmup-----------------------------------------------------------------------------------------------------
  //Parse the structure elements and print them as mentioned in the Project Writeup
  /* Comment out the following line before Phase2 */

	/*DONE - original code:*/
	//show_targets(targets, nTargetCount);

  //End of Warmup------------------------------------------------------------------------------------------------------

  /*
   * Set Targetname
   * If target is not set, set it to default (first target from makefile)
   */
  if(argc == 1)
	strcpy(TargetName, argv[optind]);    // here we have the given target, this acts as a method to begin the building
  else
  	strcpy(TargetName, targets[0].TargetName);  // default part is the first target

  /*
   * Now, the file has been parsed and the targets have been named.
   * You'll now want to check all dependencies (whether they are
   * available targets or files) and then execute the target that
   * was specified on the command line, along with their dependencies,
   * etc. Else if no target is mentioned then build the first target
   * found in Makefile.
   */

  //Phase2: Begins ----------------------------------------------------------------------------------------------------
  /*Your code begins here*/

  for (int i = 0; i < MAX_NODES; i++)     // INITIALIZE: Set up the status for all target in target list
  {
      targets[i].Status = NEEDS_BUILDING;
  }

  int target_location = find_target(TargetName, targets, nTargetCount);       // FINDING: Location of the target
  if (target_location != ERROR)                                               // Location Found => move to build
    build (targets, target_location, nTargetCount);
  else                                   //Location NOT Found => ERROR HANDLING: Target does not exist in Makefile
    printf("*** No rule to make target '%s'. Stop.\n", TargetName);

  /*End of your code*/
  //End of Phase2------------------------------------------------------------------------------------------------------

  return 0;
}
/*-------------------------------------------------------END OF MAIN PROGRAM------------------------------------------*/
