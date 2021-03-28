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

## User-creation functionalities:


## Note:
  - None of the commands are case-sensitive (cd and CD are considered as same)
  - All file paths are sanitized on the Server side. 
    - So, A Client can only access the files on the Server that are present in his directory
    - Example: If filepath= ../OtherUser/SuperSecret.txt,
                sanitized filepath=./SuperSecret.txt 
    - So, even if User1 gives ../OtherUser/SuperSecret.txt as a filepath, 
      he can't access SuperSecret.txt stored by OtherUser.