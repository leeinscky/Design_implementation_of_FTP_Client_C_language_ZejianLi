#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#define BUF_SIZE 1024
#define SERVICE_READY 220
#define NEED_PASSWORD 331
#define LOGIN_SUCS 230
#define CONTROL_CLOSE 221
#define PATHNAME_CREATE 257
#define PASV_MODE 227
#define NO_SUCH_FILE 550
#define GET 1
#define PUT 2
#define PWD 3
#define LS 4
#define CD 5
#define HELP 6
#define QUIT 7
#define DELETE 8
#define RENAME 9
#define MKDIR 10
#define ASCII 11
#define BINARY 12

int transmit=0; 
struct sockaddr_in server;  // Control connection socket
struct hostent* hent;       // server host hostent* pointer
char user[20];              // username
char pass[20];              // password
int data_port;              // data connection port

/*erro report*/
void errorReport(char* err_info) {
    printf("# %s\n", err_info);
}

/*Send command with socket number, command identifier and command parameters*/
void sendCommand(int sock_fd, const char* cmd, const char* info) {
    char buf[BUF_SIZE] = {0};
    strcpy(buf, cmd);
    strcat(buf, info);
    strcat(buf, "\r\n");
    if (send(sock_fd, buf, strlen(buf), 0) < 0)
        errorReport("Send command error!");
}

/*The control connection gets a reply from the server and returns a reply code*/
int getReplyCode(int sockfd) {
   int r_code, bytes;
    char buf[BUF_SIZE] = {0}, nbuf[5] = {0};
    if ((bytes = read(sockfd, buf, BUF_SIZE - 2)) > 0) {
        r_code = atoi(buf);//Converts the string to an integer, returning the converted integer
        buf[bytes] = '\0';
        printf("%s", buf);
    }
    else
        return -1;
    if (buf[3] == '-') {
        char* newline = strchr(buf, '\n');//The position of '\n' appears for the first time
        if (*(newline+1) == '\0') {
            while ((bytes = read(sockfd, buf, BUF_SIZE - 2)) > 0) {
                buf[bytes] = '\0';
                printf("%s", buf);
                if (atoi(buf) == r_code)
                    break;
            }
        }
    }
    if (r_code == PASV_MODE) {
        char* begin = strrchr(buf, ',')+1;
        char* end = strrchr(buf, ')');
        strncpy(nbuf, begin, end - begin);
        nbuf[end-begin] = '\0';
        data_port = atoi(nbuf);
        buf[begin-1-buf] = '\0';
        end = begin - 1;
        begin = strrchr(buf, ',')+1;
        strncpy(nbuf, begin, end - begin);
        nbuf[end-begin] = '\0';
        data_port += 256 * atoi(nbuf);
    }

    return r_code;
}

/*Connect to the server, with IP address and port number as parameters, 
and return the number whether the connection was successful*/
int connectToHost(char* ip, char* pt) {
    int sockfd;
    int port = atoi(pt);
    if (port <= 0 || port >= 65536)
        errorReport("Invalid Port Number!");
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((server.sin_addr.s_addr = inet_addr(ip)) < 0) {//Convert network addresses to binary digits
        if ((hent = gethostbyname(ip)) != 0)//Get network data from the host name
            memcpy(&server.sin_addr, hent->h_addr, sizeof(&(server.sin_addr)));//Copy h_addr to parameter 1
        else
            errorReport("Invalid Host!");
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errorReport("Create Socket Error!");
    if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
        errorReport("Cannot connect to server!");
    printf("Successfully connect to server: %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    return sockfd;
}

/*users login*/
int userLogin(int sockfd) {
   memset(user, 0, sizeof(user));
    memset(pass, 0, sizeof(pass));
    char buf[BUF_SIZE];
    printf("\n Username: ");
    fgets(buf, sizeof(buf), stdin);
    if (buf[0] != '\n')
        strncpy(user, buf, strlen(buf) - 1);
    else
        strncpy(user, "anonymous", 9);
    sendCommand(sockfd, "USER ", user);
    if (getReplyCode(sockfd) == NEED_PASSWORD) {
        memset(buf, 0, sizeof(buf));
        
        strcpy(pass,getpass("password:"));
        sendCommand(sockfd, "PASS ", pass);
        if (getReplyCode(sockfd) != LOGIN_SUCS) {
            printf("Password wrong. ");
            return -1;
        }
        else {
            printf("User %s login successfully!\n", user);
            return 0;
        }
    }
    else {
        printf("User not found! ");
        return -1;
    }
}


int cmdToNum(char* cmd) {
    cmd[strlen(cmd)-1] = '\0';
    if (strncmp(cmd, "get", 3) == 0)
        return GET;
    if (strncmp(cmd, "put", 3) == 0)
        return PUT;
    if (strcmp(cmd, "pwd") == 0)
        return PWD;
    if (strcmp(cmd, "ls") == 0)
        return LS;
    if (strncmp(cmd, "cd", 2) == 0)
        return CD;
    if (strcmp(cmd, "?") == 0 || strcmp(cmd, "help") == 0)
        return HELP;
	 if (strncmp(cmd, "delete",6) == 0)
	        return DELETE ;
	 if (strncmp(cmd, "rename",6) == 0)
	        return RENAME ;
	 if (strncmp(cmd, "mkdir",5) == 0)
	        return MKDIR ;
	 if (strcmp(cmd, "binary") == 0)
        return BINARY;
	 if (strcmp(cmd, "ascii") == 0)
        return ASCII;
	 if (strcmp(cmd, "quit") == 0)
        return QUIT;
    return -1;  // No command
}

/*Download a file from the server*/
void cmd_get(int sockfd, char* cmd) {
    int i = 0, data_sock, bytes;
    char filename[BUF_SIZE], buf[BUF_SIZE];
    while (i < strlen(cmd) && cmd[i] != ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    while (i < strlen(cmd) && cmd[i] == ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    strncpy(filename, cmd+i, strlen(cmd+i)+1);
	if (transmit==0){
		sendCommand(sockfd, "TYPE ", "I");
   		getReplyCode(sockfd);
   		printf("by BINARY..\n");
	}
    else{
    	sendCommand(sockfd, "TYPE ", "A");
   		getReplyCode(sockfd);
   		printf("by ASCII..\n");
	}
    sendCommand(sockfd, "PASV", "");
    if (getReplyCode(sockfd) != PASV_MODE) {
        printf("Error!\n");
        return;
    }
    server.sin_port = htons(data_port);
    if ((data_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errorReport("Create socket error!");

    if (connect(data_sock, (struct sockaddr*)&server, sizeof(server)) < 0)
        errorReport("Cannot connect to server!");
    printf("Data connection successfully: %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    sendCommand(sockfd, "RETR ", filename);
    if (getReplyCode(sockfd) == NO_SUCH_FILE) {
        close(sockfd);
        return;
    }

    FILE* dst_file;
    if ((dst_file = fopen(filename, "wb")) == NULL) {
        printf("Error!");
        close(sockfd);
        return;
    }
    while ((bytes = read(data_sock, buf, BUF_SIZE)) > 0){
    	fwrite(buf, 1, bytes, dst_file);
    	sleep(1);
	}
        

    close(data_sock);
    getReplyCode(sockfd);
    fclose(dst_file);
}

/*Upload a file to the server*/
void cmd_put(int sockfd, char* cmd) {
    int i = 0, data_sock, bytes;
    char filename[BUF_SIZE], buf[BUF_SIZE];
    while (i < strlen(cmd) && cmd[i] != ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    while (i < strlen(cmd) && cmd[i] == ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    strncpy(filename, cmd+i, strlen(cmd+i)+1);

    sendCommand(sockfd, "PASV", "");
    if (getReplyCode(sockfd) != PASV_MODE) {
        printf("Error!");
        return;
    }
    FILE* src_file;
    if ((src_file = fopen(filename, "rb")) == NULL) {
        printf("Error!");
        return;
    }
    server.sin_port = htons(data_port);
    if ((data_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        errorReport("Create socket error!");
    }
    if (connect(data_sock, (struct sockaddr*)&server, sizeof(server)) < 0)
        errorReport("Cannot connect to server!");
    printf("Data connection successfully: %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    sendCommand(sockfd, "STOR ", filename);
    if (getReplyCode(sockfd) == NO_SUCH_FILE) {
        close(data_sock);
        fclose(src_file);
        return;
    }
    while ((bytes = fread(buf, 1, BUF_SIZE, src_file)) > 0)
        send(data_sock, buf, bytes, 0);

    close(data_sock);
    getReplyCode(sockfd);
    fclose(src_file);
}
/*make a new directory*/
void cmd_mkdir(int sockfd, char* cmd) {
    int i = 0;
    char buf[BUF_SIZE];
    while (i < strlen(cmd) && cmd[i] != ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    while (i < strlen(cmd) && cmd[i] == ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    strncpy(buf, cmd+i, strlen(cmd+i)+1);
    sendCommand(sockfd, "MKD ", buf);
    getReplyCode(sockfd);
}

//delete
void cmd_delete(int sockfd, char* cmd) {
    int i = 0;
    char buf[BUF_SIZE];
    while (i < strlen(cmd) && cmd[i] != ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    while (i < strlen(cmd) && cmd[i] == ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    strncpy(buf, cmd+i, strlen(cmd+i)+1);
    sendCommand(sockfd, "DELE ", buf);
    getReplyCode(sockfd);
	}
//rename
void cmd_rename(int sockfd, char* cmd, char* wbuf) {
    int i = 0;
    char buf[BUF_SIZE],buf1[BUF_SIZE];
    while (i < strlen(cmd) && cmd[i] != ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    while (i < strlen(cmd) && cmd[i] == ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    strncpy(buf1, cmd+i, strlen(cmd+i)+1);
    sprintf(buf, "RNFR %s\r\n", buf1);
    sendCommand(sockfd, buf, "");
    sprintf(buf, "RNTO %s\r\n", wbuf);
    sendCommand(sockfd, buf, "");
    getReplyCode(sockfd);
	}



/*Displays the current server directory*/
void cmd_pwd(int sockfd) {
    sendCommand(sockfd, "PWD", "");
    if (getReplyCode(sockfd) != PATHNAME_CREATE)
        errorReport("Wrong reply for PWD!");
}

/*Lists the remote server directory*/
void cmd_ls(int sockfd) {
    int data_sock, bytes;
    char buf[BUF_SIZE] = {0};
    sendCommand(sockfd, "PASV", "");
    if (getReplyCode(sockfd) != PASV_MODE) {
        printf("Error!");
        return;
    }
    server.sin_port = htons(data_port);
    if ((data_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        errorReport("Create socket error!");
    if (connect(data_sock, (struct sockaddr*)&server, sizeof(server)) < 0)
        errorReport("Cannot connect to server!");
    printf("Data connection successfully: %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

    sendCommand(sockfd, "LIST ", "-al");
    getReplyCode(sockfd);
    printf("\n");
    // A data connection gets the data transferred by the server
    while ((bytes = read(data_sock, buf, BUF_SIZE - 2)) > 0) {
        buf[bytes] = '\0';
        printf("%s", buf);
    }
    printf("\n");
    close(data_sock);
    getReplyCode(sockfd);
}

/*Change remote server directory*/
void cmd_cd(int sockfd, char* cmd) {
    int i = 0;
    char buf[BUF_SIZE];
    while (i < strlen(cmd) && cmd[i] != ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    while (i < strlen(cmd) && cmd[i] == ' ') i++;
    if (i == strlen(cmd)) {
        printf("Command error: %s\n", cmd);
        return;
    }
    strncpy(buf, cmd+i, strlen(cmd+i)+1);
    sendCommand(sockfd, "CWD ", buf);
    getReplyCode(sockfd);
}
/*BINARY mode*/
void cmd_binary(int sockfd) {
	transmit=0;
	printf("now BINARY..\n");
}
/*ASCII mode*/
void cmd_ascii(int sockfd) {
	transmit=1;
	printf("now ASCII..\n");

}
/*HELP information*/
void cmd_help() {
	printf("==--------------------help----------------------==\n");
    printf(" get <>\t \tget a file from server.\n");
    printf(" put <>\t \tsend a file to server.\n");
    printf(" pwd \t \tget the present directory on server.\n");
    printf(" ls \t\t list the directory on server.\n");
    printf(" cd <>\t\t change the directory on server.\n");
    printf(" help\t \thelp you know how to use the command.\n");
    printf(" rename <><>\t rename a to b.\n");
    printf(" delete <>\t delete a file.\n");
    printf(" mkdir <>\t make a dictionary.\n");
    printf(" binary \t \t transfer mode to binary.\n");
    printf(" ascii \t \t transfer mode to ASCII.\n");
    printf(" quit \t \tquit client.\n");
    printf("==----------------------------------------------==\n");
}

/*quit the operation*/
void cmd_quit(int sockfd) {
    sendCommand(sockfd, "QUIT", "");
    if (getReplyCode(sockfd) == CONTROL_CLOSE)
        printf("Logout.\n");
}

/*Run the server*/
void run(char* ip, char* pt) {
    int  sockfd = connectToHost(ip, pt);
    if (getReplyCode(sockfd) != SERVICE_READY)
        errorReport("Service Connect Error!");
    while (userLogin(sockfd) != 0)      // Call the login function "userLogin"
        printf("Please try again.\n");
    int isQuit = 0;
    char buf[BUF_SIZE];
    char wbuf[BUF_SIZE];
    while (!isQuit) {
        printf("Please input client command: ");
        fgets(buf, sizeof(buf), stdin);
        switch (cmdToNum(buf)) {
            case GET:
                cmd_get(sockfd, buf);
                break;
            case PUT:
                cmd_put(sockfd, buf);
                break;
            case DELETE:
                cmd_delete(sockfd, buf);
                break;
            case RENAME:
            	
                printf("Please enter the new name:\n");
            	fgets(wbuf, sizeof(wbuf), stdin);
                cmd_rename(sockfd, buf,wbuf);
                break;
            case MKDIR:
                cmd_mkdir(sockfd, buf);
                break;
            case PWD:
                cmd_pwd(sockfd);
                break;
            case LS:
                cmd_ls(sockfd);
                break;
            case CD:
                cmd_cd(sockfd, buf);
                break;
            case HELP:
                cmd_help();
                break;
            case BINARY:
                cmd_binary(sockfd);
                break;
            case ASCII:
                cmd_ascii(sockfd);
                break;
            case QUIT:
                cmd_quit(sockfd);
                isQuit = 1;
                break;
            default:
                cmd_help();
                break;
        }
    }
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        printf("Usage: %s <host> [<port>]\n", argv[0]);
        exit(-1);
    }
    else if (argc == 2)
        run(argv[1], "21");
    else
        run(argv[1], argv[2]);
}
