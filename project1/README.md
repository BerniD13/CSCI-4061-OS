### CSci4061 F2018 Assignment 1 Report
> - login: cselabs login name (login used to submit) : duong142
> - date: 09/23/18
> - name: Berni D, Xiangyu Zhang, Peigen Luo
> - id: 5329383, 5422919, 5418155

### Program Purpose
Make is a useful utility which builds executable programs or libraries from source files based on an input makefile which includes information on how to build targets (e.g. executable programs or libraries).
The makefile specifies a dependency graph that governs how targets should be built. In this assignment, we write a simple version of Make (make4061) which:
> - reads the makefile,
> - follows the specification in the file to construct the dependence graph and
> - builds the targets using fork, exec and wait in a controlled fashion just like standard Make in Linux using the graph.

Our make4061 will use the dependence graph (we have provided code to generate it from the input makefile) to build any targets (e.g. compile files) in the proper order. The command to finally build the target can be executed once all of its dependencies have been resolved (i.e. those targets have been built and so on).

### Program Description
Our make4061 can be used to analyzing dependencies of targets, determining which targets are eligible to be built. Dependency targets will be built ﬁrst. As targets in makeﬁle are built, out make4061 program will determine which targets in the makeﬁle have become eligible to be compiled (built), and this process continues until ﬁnal target is built. If out program encounters an error in processing the makeﬁle, a useful error message (e.g. the name of the current target for which the error occurred) will be printed to the screen and the program terminated.

### Primary Functionality
- [x] show targets and all its dependencies
- [x] search through DAG graph run the leaf nodes of the graph.
- [x] execute command for each targets
- [x] treat differently to the files with different status.
- [x] check and compare time to decide if the target file needs Execution
- [x] use fork() and execvp() to execute command.
- [x] use wait() to catch the child process status
- [x] error handling: error file name or code, wrong command, empty command, kill process...
- ...

### Testing make4061
- Run make to compile and generate the "make4061" executing file
- Copy "make4061" to testcase folder
- Run as usual using "./make4061"

### Work Description
We were doing this project under Berni's leading. He told us what should we do,
and how we maybe implement things. He gave us the structure of make4061,
and then Peigen and Xiangyu wrote basic parts of build() and execute() function.
Then we start to try out test cases that professor gave and also some cases we
made of ourselves. For example a target file without dependencies and commands.
This part was mostly done by Berni which he spent more than 30 hours in total to
get all details working in right way.
Xiangyu wrote most additional test cases and Peigen writes this README file.
