#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"

// OPTIONAL: signal handler for user
void handle_SIGINT(int sig_num) {
	printf("YOU ARE KICKED OUT FROM SERVER!\n");
	printf("The server is crashed or Invalid user name\n");
	exit(1);//termination
}


/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {

	int pipe_user_reading_from_server[2], pipe_user_writing_to_server[2];
	//signal(SIGTERM, handle_SIGTERM); // OPTIONAL
	//signal(SIGINT, handle_SIGINT);	// OPTIONAL
	// You will need to get user name as a parameter, argv[1].
	if(connect_to_server("Group_25", argv[1], pipe_user_reading_from_server, pipe_user_writing_to_server) == ERROR) {
		printf("Failed to connect to server. Possible reason is no connect make or slot full\n");
		exit(ERROR);
	}

	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
	printf("CONNECTED\n");


	// CLOSE: Approriate pipes
	close(pipe_user_reading_from_server[1]); //user writes
	close(pipe_user_writing_to_server[0]); //user receives
	// poll pipe retrieved and print it to sdiout
 	while(1)
	{
		char server_output[MAX_MSG];
		memset(server_output, '\0', MAX_MSG);
		// set up nonblocking pipe
		fcntl(pipe_user_reading_from_server[0], F_SETFL, fcntl(pipe_user_reading_from_server[0], F_GETFL)| O_NONBLOCK);
		//Reading from child-user pipe
		int nread = read(pipe_user_reading_from_server[0], server_output, sizeof(server_output));

		if (nread > 0) {	// read something - print in terminal
			printf ("%s\n", server_output);
		} else if (nread == BROKEN) {		// EXTRA CREDIT - clean up pipe
			printf("-----------------USER MSG: Broken CHILD-USER pipe. <%s> is disconnected-----------------\n", argv[1]);
			close(pipe_user_reading_from_server[0]);	//user writes
			close(pipe_user_writing_to_server[1]);	//user receives
			printf("-----------------USER MSG: Child-User <%s> pipe has been CLEAN-----------------\n", argv[1]);
			printf("USER EXIT!!!\n");
			exit(0);
		}

		fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); // nonblocking read from stdin
		char user_output[MAX_MSG];		// user input from terminal

		if (fgets(user_output,sizeof(user_output),stdin)!=NULL) {
			user_output[strcspn(user_output,"\n")] = '\0';    // pruning \n at the end of the command
			printf ("%s: %s\n", argv[1], user_output);				// print in user's terminal
			write(pipe_user_writing_to_server[1], user_output, sizeof(user_output));  // writting to child-user pipe
		}	// end fget
		usleep(SLEEP);	// Reduce CPU consumption
	} // end polling
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/
