#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/wait.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h> 
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<poll.h>
#include<time.h>
#include<stdbool.h>
#include<vector>

using namespace std;

#define MAXLINE 	2000
#define MAXMES  	1500
#define OPEN_MAX 	1024
#define NAME_LEN    10
#define TOPIC_LEN   50

struct user{
    char ipAddr[15];
	char nickname[NAME_LEN]="";
    char username[NAME_LEN]="";
    // char hostname[NAME_LEN]="";
    // char servername[NAME_LEN]="";
    // char realname[NAME_LEN]="";
    vector<int>  inChannel;
    int          numInChannel = 0;
};
struct channel{
    char name[NAME_LEN];
    char topic[TOPIC_LEN]="";
    int  numUser=0;
    vector<int> users;
};

void addListenPort(int* fd, uint16_t port);
void acceptConnect(int connfd, char* ipAddr);
int  findChannel(char* name);
int  addJoinChannel(int useri, char* name);
int  onChannel(int useri, int channeli);
void leaveChannel(int useri, int channeli, int onChanneli, bool quit);
void sendtoChannel(int useri, int channeli, int cut, bool sys, bool except);
void recCommand(int useri);
void dealError(int useri, int error);
void disconnect(int useri);
void namesList(int useri, int channeli);

#endif