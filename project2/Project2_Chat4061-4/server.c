/* CSci4061 F2018 Assignment 2
* login: cselabs login name (login used to submit) : duong142
* date: 11/10/18
* name: Berni D, Xiangyu Zhang, Peigen Luo(for partner(s))
* id: 5329383, 5422919, 5418155 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "comm.h"
#include "util.h"

/* -----------Functions that implement server functionality -------------------------*/

/*
 * Returns the empty slot on success, or ERROR on failure
 */
int find_empty_slot(USER * user_list) {
	// iterate through the user_list and check m_status to see if any slot is EMPTY
	// return the index of the empty slot
  int i = 0;
	for(i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_EMPTY) {
			return i;
		}
	}
	return ERROR;
}

/* Check for valid user name - not same in user list
* Return 0 if same name - Invalid
* Return 1 if okay name
*/
int is_valid_user_name(char * user_name, USER * user_list) {
  for(int i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_FULL) {
        if (strcmp(user_name, user_list[i].m_user_id) == 0)
          return 0;
		  }
	}
  return 1;
}

/* Kick the user or disconnect the user before fork.
* Kill user process using user's pid
*/
void disconnect_user(int user_pid, char * user_name) {
  int status;
  pid_t endID;
  printf("Invalid user name <%s>. Disconnected!!!\n", user_name);
  if(kill( user_pid, SIGTERM) == ERROR)
    printf("Failed to send the interupted signal to the user <%s>.\n", user_name);

  endID = waitpid(user_pid,&status,0);
  if(endID == ERROR)
    printf("Failed to wait the user <%s> pid <%d>.\n", user_name, user_pid);
  else if (endID == user_pid) {
    if (WIFEXITED(status)) {
      printf ("Invalid user name <%s> disconnected normally.\n", user_name);
    }
  } else if (WIFSIGNALED(status)) {
      printf ("Invalid user name <%s> disconnected by KILL.\n", user_name);
  }
}

/*
 * list the existing users on the server shell
 */
int list_users(int idx, USER * user_list)
{
	// iterate through the user list
	// if you find any slot which is not empty, print that m_user_id
	// if every slot is empty, print "<no users>""
	// If the function is called by the server (that is, idx is ERROR), then printf the list
	// If the function is called by the user, then send the list to the user using write() and passing m_fd_to_user
	// return 0 on success
	int i, flag = 0;
	char buf[MAX_MSG] = {}, *s = NULL;

	/* construct a list of user names */
	s = buf;
	strncpy(s, "---connecetd user list---\n", strlen("---connecetd user list---\n"));
	s += strlen("---connecetd user list---\n");
	for (i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		flag = 1;
		strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
		s = s + strlen(user_list[i].m_user_id);
		strncpy(s, "\n", 1);
		s++;
	}
	if (flag == 0) {
		strcpy(buf, "<no users>\n");
	} else {
		s--;
		strncpy(s, "\0", 1);
	}

	if(idx < 0) {
		printf("%s",buf);
		printf("\n");
	} else {
		/* write to the given pipe fd */
		if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
			perror("writing to server shell");
	}

	return 0;
}

/*
 * add a new user
 */
int add_user(int idx, USER * user_list, int pid, int user_pid, char * user_id, int pipe_to_child, int pipe_to_parent)
{
	// populate the user_list structure with the arguments passed to this function
	// return the index of user added
    // No need to checkk error - assume values enter are correct or valid
    user_list[idx].m_pid = pid;
    user_list[idx].m_user_pid = user_pid;
    memset(user_list[idx].m_user_id, '\0', MAX_USER_ID);
    strcpy(user_list[idx].m_user_id, user_id);
    user_list[idx].m_fd_to_user = pipe_to_child;
    user_list[idx].m_fd_to_server = pipe_to_parent;
    user_list[idx].m_status = SLOT_FULL;

	return idx;
}

/*
 * find user index for given user name
 */
int find_user_index(USER * user_list, char * user_id)
{
	// go over the  user list to return the index of the user which matches the argument user_id
	// return ERROR if not found
	int i, user_idx = ERROR;

	if (user_id == NULL) {
		fprintf(stderr, "NULL name passed.\n");
		return user_idx;
	}
	for (i=0;i<MAX_USER;i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		if (strcmp(user_list[i].m_user_id, user_id) == 0) {
			return i;
		}
	}

	return ERROR;
}

/*
 * given a command's input buffer, extract name
 */
int extract_name(char * buf, char * user_name)
{
	char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 2) {
        strcpy(user_name, tokens[1]);
        return 0;
    }

    return ERROR;
}

int extract_text(char *buf, char * text)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    char * s = NULL;
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 3) {
        //Find " "
        s = strchr(buf, ' ');
        s = strchr(s+1, ' ');

        strcpy(text, s+1);
        return 0;
    }

    return ERROR;
}

/*
 * Kill a user
 */
void kill_user(int idx, USER * user_list) {
	// kill a user (specified by idx) by using the systemcall kill()
	// then call waitpid on the user

  int status;
  pid_t endID, user_pid;

  if(idx < 0 || idx >= MAX_USER )       // error checking
    printf("Invalid index: %d. ", idx);
  else
  {
    user_pid = user_list[idx].m_user_pid;
    if(kill(user_pid, SIGTERM) == ERROR)  // kill a user
      printf("Failed to send the kill signal to the user %s. ", user_list[idx].m_user_id);

    endID = waitpid(user_pid,&status,0);  // waitpid on the user
    if(endID == ERROR)
      printf("Failed to wait the user %s.\n", user_list[idx].m_user_id);
    else if (endID == user_pid) {
      if (WIFEXITED(status)) {
        printf ("User <%s> ended normally.\n", user_list[idx].m_user_id);
      } else if (WIFSIGNALED(status)) {
        printf ("User <%s> ended because of KILL.\n", user_list[idx].m_user_id);
      }
    }
  }
}


/*
 * Kill a child
 */
void kill_child(int idx, USER * user_list) {
	// kill a child (specified by idx) by using the systemcall kill()
	// then call waitpid on the child

  int status;
  pid_t endID, child_pid;

  if(idx < 0 || idx >= MAX_USER ) // error checking
    printf("Invalid index: %d. ", idx);
  else
  {
    child_pid = user_list[idx].m_pid;
    if(kill(child_pid, SIGINT) == ERROR)   //kill a child
      printf("Failed to send the kill signal to the child %s. ", user_list[idx].m_user_id);

    endID = waitpid(child_pid,&status,0);   // waitt on the child
    if(endID == ERROR)
      printf("Failed to wait the child %s.\n", user_list[idx].m_user_id);
    else if (endID == child_pid) {
      if (WIFEXITED(status)) {
        printf ("Child <%s> ended normally.\n", user_list[idx].m_user_id);
      } else if (WIFSIGNALED(status)) {
        printf ("Child <%s> ended because of KILL.\n", user_list[idx].m_user_id);
      }
    } // end wait
  } // end kill
}

/*
 * Perform cleanup actions after the used has been killed
 */
void cleanup_user(int idx, USER * user_list)
{
	// m_pid should be set back to ERROR
	// m_user_id should be set to zero, using memset()
	// close all the fd
	// set the value of all fd back to ERROR
	// set the status back to empty

  user_list[idx].m_pid = ERROR;
  memset(user_list[idx].m_user_id, 0, sizeof(user_list[idx].m_user_id) );
  if (close(user_list[idx].m_fd_to_user) == ERROR)
    perror("Failed to close the fd to user.\n");
  if (close(user_list[idx].m_fd_to_server) == ERROR)
    perror("Failed to close the fd to server.\n");
  user_list[idx].m_fd_to_user = ERROR;
  user_list[idx].m_fd_to_server = ERROR;
  user_list[idx].m_status = SLOT_EMPTY;
  printf("CLEAN!\n");
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list) {
	// should kill_user()              ->> kill child is easier
	// then perform cleanup_user()

  kill_child(idx, user_list); // lilterally kill child instead of user, but this program act similar
  // kill_user(idx, user_list);  // OPTIONAL
  cleanup_user(idx, user_list);
}

/*
 * send personal message
 */
void send_p2p_msg(int idx, USER * user_list, char *buf)
{

	// get the target user by name using extract_name() function
	// find the user id using find_user_index()
	// if user not found, write back to the original user "User not found", using the write()function on pipes.
	// if the user is found then write the message that the user wants to send to that user.

  char user_name[MAX_USER_ID];
  memset(user_name, '\0', MAX_USER_ID);

  char err_msg1[MAX_MSG] = "No user indicated\n"; // Error 1
  char err_msg2[MAX_MSG] = "No text indicated\n"; // Error 2
  char err_msg3[MAX_MSG] = "User not found\n";  // Error 3
  char err_msg4[MAX_MSG] = "Hm? Interesting. Do you like chat to yourself? Let's chat to someone else\n"; // Error 4
  int check_name = extract_name(buf, user_name);
  if (check_name == ERROR) // check Error 1
  {
    write(user_list[idx].m_fd_to_user, err_msg1, sizeof(err_msg1));
  } else {
    char text[MAX_MSG];
    memset(text, '\0', MAX_MSG);
    int check_text = extract_text(buf,text);
    if (check_text == ERROR) {    // check Error 2
      write(user_list[idx].m_fd_to_user, err_msg2, sizeof(err_msg2));
    } else {
      int user_idx = find_user_index(user_list, user_name);
      if (user_idx == ERROR) {  // check Error 3
        write(user_list[idx].m_fd_to_user, err_msg3, sizeof(err_msg3));
      } else {                  // Found user idx
        if (user_idx != idx) {  // NO MORE ERROR
          char real_msg[MAX_MSG];   // Set up real msg include user name
          memset(real_msg, '\0', MAX_MSG);
          sprintf(real_msg, "%s: %s", user_list[idx].m_user_id,text); // May lost some chars if try to type msg over 244 chars - No very important for now
          write(user_list[user_idx].m_fd_to_user, real_msg, sizeof(real_msg)); // Write msg to pipe server-child
        } else {  // Error 4
          write(user_list[idx].m_fd_to_user, err_msg4, sizeof(err_msg4));
        } // end error 4
      } // end check Error 3
    } // end check Error 2
  } // end check Error 1
} // end void send_p2p_msg(...)

/*
 * broadcast message to all users
 */
int broadcast_msg(USER * user_list, char *buf, char *sender)
{
	//iterate over the user_list and if a slot is full, and the user is not the sender itself,
	//then send the message to that user
	//return zero on success

  char real_msg[MAX_MSG];
  memset(real_msg, '\0', MAX_MSG);
  if (strcmp(sender, "SERVER") == 0)
    sprintf(real_msg, "Notice: %s",buf);    // May lost some chars because size of real_msg
  else
    sprintf(real_msg, "%s: %s",sender, buf);  // May lost some chars because size of real_msg
  for(int i=0;i<MAX_USER;i++) {
    if(user_list[i].m_status == SLOT_FULL && strcmp(user_list[i].m_user_id , sender) != 0) { // send everybody except the sender
      write(user_list[i].m_fd_to_user, real_msg, sizeof(real_msg));
    }
  }
	return 0;
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list) {
	// go over the user list and check for any empty slots
	// call cleanup user for each of those users.

  int i = 0;
  for(i=0;i<MAX_USER;i++) {
    if(user_list[i].m_status == SLOT_EMPTY)
      cleanup_user(i, user_list);
  }
}

int extract_first_word(char * buf, char * first_word) // OPTIONAL dont use it
{
  char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 1) {
        strcpy(first_word, tokens[0]);
        return 0;
    }
    return ERROR;
}

//takes in the filename of the file being executed, and prints an error message stating the commands and their usage
void show_error_message(char *filename)
{
// not required to use this func
}


/*
 * Populates the user list initially
 */
void init_user_list(USER * user_list) {

	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to ERROR
	//set the status to be EMPTY
	int i=0;
	for(i=0;i<MAX_USER;i++) {
		user_list[i].m_pid = ERROR;
		memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
		user_list[i].m_fd_to_user = ERROR;
		user_list[i].m_fd_to_server = ERROR;
		user_list[i].m_status = SLOT_EMPTY;
	}
}

// OPTIONAL: Signal handler for Server
void handle_SIGINT_parent(int sig_num) {
  printf("-----------------SERVER MSG: Interupted-----------------\n");
  exit(1);//termination
}
// OPTIONAL: Signal handler for Child
void handle_SIGINT_child(int sig_num) {
	printf("-----------------CHILD MSG: Interupted-----------------\n");
	exit(1);//termination
}
/* ---------------------End of the functions that implement Server functionality -----------------*/


/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{
	int nbytes;
  signal(SIGINT, handle_SIGINT_parent); // Error handle for server Ctrl C
	setup_connection("Group_25"); // Specifies the connection point as argument.

	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	char buf[MAX_MSG];
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
	print_prompt("-----Admin-----\n");

  int pipe_SERVER_reading_from_child[MAX_USER_ID][2];   // Initialize pipe for each chilld
  int pipe_SERVER_writing_to_child[MAX_USER_ID][2];     // Initialize pipe for each chilld
	while(1) {
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/

		// Handling a new connection using get_connection
		char user_id[MAX_USER_ID];                // LITERALLY: user name
    int user_pid = 0;                         // User process pid use to terminalize user process. OPTIONAL

    int idx = find_empty_slot(user_list);     // Find empty slot for new users

    if (idx != ERROR)       // Check max user
    {
      int pipe_child_writing_to_user[2];        // Initialize pipes for child-user
      int pipe_child_reading_from_user[2];      // Initialize pipes for child-user

      int i = ERROR;       // variable for checking connection
      i = get_connection(user_id, &user_pid, pipe_child_writing_to_user, pipe_child_reading_from_user);

      if (i != ERROR) {    // Success connected
        // Check valid user name (not same)
        if (is_valid_user_name(user_id, user_list) == 0 ) {
          disconnect_user(user_pid, user_id);           // Invalid user name -> disconnected or kick out :)
        } else {    // Valid user name - obtain connected

        // Create writting pipe: server-child
        if(pipe(pipe_SERVER_writing_to_child[idx]) == ERROR) {
          perror ("pipe_SERVER_writing_to_child error!");
          exit(ERROR);
        }
        // Create reading pipe: server-child
        if(pipe(pipe_SERVER_reading_from_child[idx]) == ERROR) {
          perror ("pipe_SERVER_reading_from_child error!");
          exit(ERROR);
        }

        /*CLOSE: corresponding pipe - child-user*/
        close(pipe_child_reading_from_user[1]); //user writes
        close(pipe_child_writing_to_user[0]);   //user receives

        pid_t pid;  // set up for child process
        pid = fork();
        if (pid > 0) {    // Server process
          /*CLOSE: corresponding pipe - child-user
          * Server complettely doesn't know pipe child-user after this step
          */
          close(pipe_child_reading_from_user[0]);
          close(pipe_child_writing_to_user[1]);
          // Add user to user list
          add_user(idx, user_list, pid, user_pid, user_id, pipe_SERVER_writing_to_child[idx][1], pipe_SERVER_reading_from_child[idx][0]);
        }

      /*******************************************************************************
       * Server Child process
       ******************************************************************************/
        if (pid == 0) {
          signal(SIGINT, handle_SIGINT_child);  // OPTIONAL - Set up Error handler for Child
          // OPTIONAL: close pipe child-user. Already close in server process. This is unnecessary
          close(pipe_child_reading_from_user[1]);   //user writes
          close(pipe_child_writing_to_user[0]);     //user receives
          // CLOSE: pipe child-server
          close(pipe_SERVER_reading_from_child[idx][0]);//server receives
          close(pipe_SERVER_writing_to_child[idx][1]);//sercver writes
          // Set-up nonblocking read from pipe
          fcntl(pipe_SERVER_writing_to_child[idx][0], F_SETFL, fcntl(pipe_SERVER_writing_to_child[idx][0], F_GETFL)| O_NONBLOCK);
          fcntl(pipe_child_reading_from_user[0], F_SETFL, fcntl(pipe_child_reading_from_user[0], F_GETFL)| O_NONBLOCK);
          // Child process: poli users and SERVER
          while(1)
          {
            char server_input[MAX_MSG];     // Save server input from SERVER pipe
            int numRead = read(pipe_SERVER_writing_to_child[idx][0],server_input,MAX_MSG);  // Read from server pipe
            if(numRead > 0) {
              write(pipe_child_writing_to_user[1], server_input, sizeof(server_input));   // Write to user pipe
            } else if (numRead == BROKEN) {                                // broken pipes
              printf("-----------------CHILD MSG: Broken SERVER-CHILD pipe. Server is disconnected-----------------\n");
              close(pipe_child_reading_from_user[0]);   // clean up
              close(pipe_child_writing_to_user[1]);
              printf("-----------------CHILD MSG: Child-User <%s> pipe has been CLEAN-----------------\n", user_id);
              return 0; // exit normally
            }

            char user_input[MAX_MSG];     // Save user input from USER pipe
            int numRead1 = read(pipe_child_reading_from_user[0],user_input,MAX_MSG);    // Read from user pipe
            if(numRead1 > 0) {
              write(pipe_SERVER_reading_from_child[idx][1], user_input, sizeof(user_input));  // Write to server pipe
            } else if (numRead1 == BROKEN) {             // broken pipes
              printf("-----------------CHILD MSG: Broken CHILD-USER pipe. <%s> is disconnected-----------------\n", user_id);
              close(pipe_child_reading_from_user[0]);   // clean up
              close(pipe_child_writing_to_user[1]);
              printf("-----------------CHILD MSG: Child-User <%s> pipe has been CLEAN-----------------\n", user_id);
              return 0; // exit normally
            }
            usleep(SLEEP); // Reduce CPU consumption
          }
          exit(1);      // OPTIONAL: never happenned
        }
        /*******************************************************************************
         * END Server Child process
         ******************************************************************************/
         // Server Parent process
         // CLOSE: Server - child pipes
         close(pipe_SERVER_reading_from_child[idx][1]); //server receives
         close(pipe_SERVER_writing_to_child[idx][0]); //sercver writes
         // set up nonblocking reading
         fcntl(pipe_SERVER_reading_from_child[idx][0], F_SETFL, fcntl(pipe_SERVER_reading_from_child[idx][0], F_GETFL) | O_NONBLOCK); // nonblocking read from stdin
      } // end check valid username
    } // end check connect
  } // end Check max user

// Server parent process
  char server_input[MAX_MSG];         // Save server input from stdin
  memset(server_input, '\0', MAX_MSG);    // Refresh buf
  // nonblocking read from stdin
  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

  // Poll stdin (input from the terminal) and handle admnistrative command
  // Using fgets or read. Choose fgets for now.
  if(fgets(server_input,sizeof(server_input),stdin) != NULL) {
    server_input[strcspn(server_input,"\n")] = '\0';    // pruning \n at the end of the command
    printf("Server: %s\n", server_input);               // Print out whatever server read

    char user_name[MAX_USER_ID];                        // set up for user_name p2p or kick
    memset(user_name, '\0', MAX_USER_ID);     // REFRESH buf
    int check_name;                           // a variable check for valid name
    // SWITCH: type of server's commmand
    switch (get_command_type(server_input)) {
      case LIST: list_users(PRINT,user_list); break;  // <\list>
      case KICK:                                      // <\kick> <usernname>
        check_name = extract_name(server_input, user_name);
        if (check_name == ERROR) {      // check can extract_name or not
          perror("User not found!\n");   // user printf or perror
        } else {
          int idx1 = find_user_index(user_list, user_name);
          if (idx1 == ERROR) {  // check valid user name in user list
            perror("User not found!\n");  // user printf or perror
          } else {
            char msg[MAX_MSG] = "You have been kicked from server"; // OPTIONAL: send a message to user tell he has been kicked
            write(user_list[idx1].m_fd_to_user, msg, sizeof(msg));  // OPTIONAL: send a message to user tell he has been kicked
            sleep(1);   // OPTIONAL: sleep for user read finally message before he restarts :)
            kick_user(idx1, user_list); // kick user
          }
        }
        break;
    //  case SEG:break;   // server dont have case SEG, only Ctrl+C
      case EXIT:
        for (int i = 0; i < MAX_USER; i++) {
          if (user_list[i].m_status == SLOT_FULL) {
            write(user_list[i].m_fd_to_user, server_input, sizeof(server_input));     // Send exit to pipe
            usleep(SLEEP);
            // kill_user(i, user_list);
            kill_child(i, user_list);   // Dont need to kill user. Only kill child then user will be automaticly closed
            cleanup_user(i, user_list); // close the server-child pipe
          }
        }
        printf ("PROGRAM EXIT!!!\n");
        exit(0);
        break;
      default:    // CASE <any-other-text> or BROADCAST
        broadcast_msg (user_list, server_input, "SERVER"); break;
    } // end of switch
  } // end of fgets
  /**************************************************************************
  * poll child processes and handle user commands
  **************************************************************************/
    for (int i = 0; i < MAX_USER; i++)
    {
        char user_input[MAX_MSG];         // variable to save users' command
        memset(user_input, '\0', MAX_MSG);  // REFRESH buf
        if (user_list[i].m_status == SLOT_FULL) {     // check real user in user_list
          int numRead1 = read(pipe_SERVER_reading_from_child[i][0],user_input,sizeof(user_input));  // read from child
          // CHECK: reading
          if(numRead1 > 0) {
            printf("%s: %s\n",user_list[i].m_user_id, user_input);    // printf user's command to SERVER terminal after reading from child
            // CHECK: type of command
            switch (get_command_type(user_input)) {
              case LIST: list_users(i,user_list); break;            // <\list>: send list users to the user who required list
              case P2P: send_p2p_msg(i,user_list, user_input); break;   // <\p2p>
              case SEG:                                                  // EXTRA CREDIT: <\seg>: seg fault from user
                kill_user(i, user_list);    // kill_user or kill_child act same, but nicer if kill user proccess beccause user make seg fault
                //kill_child(i, user_list); // OPTIONAL
                cleanup_user(i, user_list); // clean user, pipe
                break;
              case EXIT:    // <\exit>: user want exit
                kick_user(i, user_list);  // kick user -> then user process automatic exit because brokenn pipe (disconnected)
                break;
              default:     // <any_other_text>
                broadcast_msg (user_list, user_input, user_list[i].m_user_id); break; // never happenned
            } // end CHECK: type of command
          } else if (numRead1 == BROKEN) {       // broken pipe when read from child - child ended - handle EXTRA CREDIT as well
            printf("-----------------SERVER MSG: Clean Child-----------------\n");
            cleanup_user(i, user_list);
          }
        }
        usleep(SLEEP);    // Reduce CPU consumption
    } // end polling user's command
    usleep(SLEEP); // Reduce CPU consumption
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/
	} // end 	while(1)
}

/* --------------------End of the main function ----------------------------------------*/
