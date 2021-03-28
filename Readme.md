## Compiling and Running the code
- Run 'sh build.sh'
- To start the Server ./out/Server
- To start the Client ./out/Client
- To start the user-creation functionalities ./out/Users

## Note:
  - The submission contains log files of User1 and auth.txt just for refernce
  - None of the commands are case-sensitive (cd and CD are considered as same)
  - prev.zip contains my previous submission (just for refernce)

## Featues 
  - Whenever a new client is connected, server creates a new "detached" thread.
    - When  a  detached  thread  terminates,  its resources  are  automatically released
      back to the system without the need for another thread to join with 
      the terminated thread.
  - So this code can ideally support any number of clients when the resources are sufficient
  - Even if the client abruptly closes the connection, the Server can detect 
    that and terminates thread after removing the user from the activeUsersList
  - All file paths are sanitized on the Server side. 
    - So, a Client can only access the files on the Server that are present in his directory
    - Example: If filepath= ../OtherUser/SuperSecret.txt,
               then sanitized filepath=./SuperSecret.txt 
    - So, even if User1 enters GET ../OtherUser/SuperSecret.txt, 
      he can't access SuperSecret.txt stored by OtherUser.
      (If User1's directory contains SuperSecret.txt, he will get that file from the server)

## Client-side Functionalities:
- Authentication
  - Use username and password to login
  - One user can't be active from two different programs
  - Each user has a different directory on the server side
  - User creation/deletion can be done by running Users.cpp on the server side

- LIST
  - displays all the files present in the client's directory on the server side

- PUT {filepath}
  - Store a file in the Server/{ClientName} directory
  - The filepath can be relative/absolute.
  - If a file with the same filename exists in the client's folder on the Server side, the server will ask the client to choose from Abort, Overwrite, and Append. (Append is available only in the character mode, in binary mode, choosing append will Abort the operation)

- GET {filepath}
  - Fetch a file from Server
  - If the file is not available, the program will throw an error, and continue execution

- DELETE {filepath}
  - Delete a file from Server if it's present
  - Asks for confirmation before deletion

- cd {dir_path}
  - Functions like the 'cd' in Bash
  - Changes the current directory (on the local machine)
  - Comes in handy when user wants to navigate between folders while transferring files

- ls
  - Displays all the files in the curent directory 
    (SHOW is used to display all the files uploaded by the client to the server)

- EXIT
  - Terminate the FTP client

## User-creation functionalities:

- create {username} {password}
- remove {username}
- NOTE:
  - It is advisable to run Users.cpp on the server side only 
    after terminating the server application.
  - The create/remove command will automatically create/remove appropriate directories.
  - The username must not contain '/'.