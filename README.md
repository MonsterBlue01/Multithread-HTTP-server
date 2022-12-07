# Assignment 4

#### File:
* ***bind.c & bind.h***:
    These files contains function `create_listen_socket`. This is used to create a socket and bind it to the port.
* ***filelinkedlist.c & filelinkedlist.h***:
    These two files are used to create a list to contains the **path** in the requests. I originally wanted to use a barrier instead, but I think it works better. This is to prevent two requests accessing the same file at the same time. This also acts as a barrier during the final check to see if there are any remaining outstanding requests.
* ***httpserver.c & httpserver.h***:
    These two files are the main body of the entire program. There is a function that converts a string into a port number. There are also connection handlers. But you can't find the connection handler in the file. Since my program design is too different from the previous single-threaded server, I divided it into **producer** (that is, the function of dispatcher thread) and **customer** (that is, the function of worker thread). The last function is `sigterm_handler`. This is used to deal with the situation encountered when the server encounters the `SIGTERM` command (that is, `signal()`).
* ***LICENSE***:
    It's used to protect my right.
* ***Makefile***:
    This is used to compile several files together.
* ***samplelog.txt***:
    This file is the sample of what Audit Log should be after run the **test.sh**. I will use this to compare what program get after run the **test.sh**.
* ***queue.c & queue.h***:
    These two files contain definitions for thread-safe queues. This is an important part of forming a multi-threaded server.
* ***test.sh***:
    This is used to test the operation of the program.
* ***writeup.tex & writeup.pdf***:
    It contains the design of my program and more details than here. **(Please check these two files if you want to know more about this program)**
* ***README.md***:
    This is file you are reading.
* ***.gitnore***
    This is the file that filter the unnecessary files (but I think it's not so important)

#### Defect:
1. The file does not work perfectly for requests with message bodies that contain binary characters (that is, non-ASCII characters)
2. After running enough examples, you will find that in a few cases, PUT will add some extra messy content to the content of the message body. (I guess it may be related to illegal access to memory, but I still can't find out why).