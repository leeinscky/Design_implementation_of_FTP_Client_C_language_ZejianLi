# Design_implementation_of_FTP_Client_C_language_ZejianLi

Programming environment implemented by this project: this project will be executed in Linux environment 
(BUPT Ubuntu operating system is selected here), programmed in C language, and compiled with GCC compiler.

Detailed requirements breakdown is described below:

1. It is well known that in general the FTP protocol using TCP port number 21 to communicate, so in our project, 
do not have to show the input this port the ftp server can be identified by IP address, such as:
./ftpcli 10.3.255.85
./ftpcli 127.0.0.1

2. The FTP client via TCP protocol used to communicate with the server
communication process is FTP commands, so our task is to tell the user's language
translation for FTP commands, to communicate with the FTP server FTP client
connects FTP server using TCP protocol. It can receive the FTP replies coming from
server and translate it into natural language, and change the user’s commands into
FTP command before sending.

3. The first thing is to realize user authentication

4. Second needs to implement basic FTP operations, specific for Support user’s
interactive operation and provide the following commands: list (ls), directory
operation (pwd, cd, mkdir), upload a file (put), download a file (get) , delete a file
(delete), renaming a file (rename), transfer mode (ascii, binary), and exit (quit) and
for data connection, it is required to implement passive mode.

5. Be able to handle errors: invalid commands, missing parameters, requested file already existed.

6. Stable and friendly to users.
