#include "function.h"

char				    buf[MAXLINE];
struct pollfd		    client[OPEN_MAX];
struct user			    userData[OPEN_MAX];
vector<struct channel>  channels;
char				    curTime[30];
char                    tmpName[NAME_LEN];
char                    command[10];
int					    maxi = 0, numCli = 0, numCha = 0;

int main(int argc, char** argv){
    int         listenfd, connfd, sockfd, nready;
    uint16_t    port = atoi(argv[1]);
    addListenPort(&listenfd, port);

    socklen_t			clilen;
    struct sockaddr_in	cliaddr;
    client[0].fd = listenfd;
	client[0].events = POLLRDNORM;
    for(int i=1; i<OPEN_MAX; i++)
        client[i].fd = -1;
    maxi=0;
    numCli=0;
    int n;
    char ipAddr[15]="";
    for(;;){
        nready = poll(client, maxi+1, -1);

        if (client[0].revents & POLLRDNORM){
            clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
            sprintf(ipAddr, "%s", inet_ntoa(cliaddr.sin_addr));
            acceptConnect(connfd, ipAddr);
        }
        for(int i=1; i<=maxi; i++){
            if((sockfd = client[i].fd)<0) continue;
            if(client[i].revents & (POLLRDNORM | POLLERR)){
                memset(buf, '\0', sizeof(buf));
                if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
					if (errno == ECONNRESET) disconnect(i);
					else perror("read error");
				} else if (n == 0) {
					disconnect(i);
				} else{
                    printf("buf: %s", buf);
                    recCommand(i);
                }
            }
        }
    }

}
void addListenPort(int* fd, uint16_t port){
    int reuse = 1;
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));
    
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);
	servaddr.sin_port        = htons(port);
	if(bind(*fd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
        perror("bind error");
    if(listen(*fd, 100)<0)
        perror("listen error");
}
void acceptConnect(int connfd, char* ipAddr){
    printf("New Client: %d\n", connfd);
    int i;
    for(i=1; i<OPEN_MAX-1; i++){
        if(client[i].fd<0){
            client[i].fd = connfd;
            client[i].events = POLLRDNORM | POLLERR;
            if(maxi<i) maxi = i;
            numCli++;
            break;
        }
    } 
    if (i >= OPEN_MAX-1){
        printf("too many client!\n");
        close(connfd);
        return;
    } 
    sprintf(userData[i].ipAddr, "%s", ipAddr);
}
int findChannel(char* name){
    // printf("search for ..%s..\n", name);
    for(int i=0; i<channels.size(); i++){
        if(strcmp(name, channels[i].name)==0){
            return i;
        }
    }
    return -1;
}
int addJoinChannel(int useri, char* name){
    int channeli = findChannel(name);
    if(channeli!=-1){
        userData[useri].inChannel.push_back(channeli);
        userData[useri].numInChannel++;
        channels[channeli].users.push_back(useri);
        channels[channeli].numUser++;
        return channeli;
    }
    struct channel mChannel;
    sprintf(mChannel.name,"%s", name);
    channels.push_back(mChannel);  
    userData[useri].inChannel.push_back(numCha);
    userData[useri].numInChannel++;
    channels[numCha].users.push_back(useri);
    channels[numCha].numUser++;
    return numCha++;
}
int onChannel(int useri, int channeli){
    for(int i=0; i<userData[useri].numInChannel; i++){
        if(userData[useri].inChannel[i]==channeli)
            return i; //onChanneli
    }
    return -1;
}
void leaveChannel(int useri, int channeli, int onChanneli, bool quit){
    if(!quit){
        sprintf(buf, ":%s PART :%s\n",
        userData[useri].nickname, channels[channeli].name);
        sendtoChannel(useri, channeli, 0, 1, 0);
    }
    userData[useri].inChannel.erase(userData[useri].inChannel.begin()+onChanneli);
    userData[useri].numInChannel--;

    for(int i=0; i<channels[channeli].numUser; i++){
        if(channels[channeli].users[i]==useri){
            channels[channeli].users.erase(channels[channeli].users.begin()+i);
            channels[channeli].numUser--;
            break;
        }
    }
}
void sendtoChannel(int useri, int channeli, int cut, bool sys, bool except){
    int uNamei;
    char tmpBuf[MAXMES];
    memset(tmpBuf, '\0', sizeof(tmpBuf));
    int lenPara=0;
    if(!sys){
        for(cut=cut+2;cut<strlen(buf), lenPara<MAXMES; cut++){
            if(buf[cut]==' ' || buf[cut]=='\n' || buf[cut]=='\r') break;
            tmpBuf[lenPara++]=buf[cut];
        }
        if(!strlen(tmpBuf)){
            dealError(useri, 412); //(412) ERR_NOTEXTTOSEND
            return;
        }
        // strcat(tmpBuf, "\r\n");
        sprintf(buf, ":%s PRIVMSG %s :%s\r\n", userData[useri].nickname,
        channels[channeli].name, tmpBuf);
    }
    
    // if(!sys && !strlen(tmpBuf)){
    //     dealError(useri, 412); //(412) ERR_NOTEXTTOSEND
    //     return;
    // }

    for(int i=0; i<channels[channeli].numUser; i++){
        if(except && i==useri) continue;
        uNamei = channels[channeli].users[i];
        if(!sys && uNamei==useri) continue;
        send(client[uNamei].fd, buf, strlen(buf), MSG_NOSIGNAL);
    }
}
void recCommand(int useri){
    memset(command, '\0', sizeof(command));
    int cut=0, lenName=0, numPara=0, lenPara=0;
    for(cut=0;cut<sizeof(command), cut<strlen(buf);cut++){
        if(buf[cut]=='\n' || buf[cut]==' ' || buf[cut]=='\r') break;
        command[cut]=buf[cut];
    }

    if(strcmp(command, "NICK")==0){
        lenName=0;
        bool valid=true;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1;cut<strlen(buf), lenName<NAME_LEN-1; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName]=buf[cut];
            lenName++;
        }
        if(strlen(tmpName)==0){
            dealError(useri, 431); //(431) ERR_NONICKNAMEGIVEN
            return;
        }
        // printf("Name Len: %d\n", lenName);
        int sockfd;
        for(int i=1; i<=maxi; i++){ //check the name unique
            if((sockfd = client[i].fd)<0) continue;
            if(strcmp(userData[i].nickname, tmpName)==0){
                valid=false;
                break;
            }
        }
        if(valid){
            if(strlen(userData[useri].nickname)>0){
                sprintf(buf, ":%s NICK %s\r\n",
                userData[useri].nickname, tmpName);
                if(userData[useri].inChannel.size()>0){
                    for(int i=0; i<userData[useri].inChannel.size(); i++)
                        sendtoChannel(useri, userData[useri].inChannel[i], 0, 1, 1);
                    send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                } else send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                
            }
            else if(strlen(userData[useri].username)>0 && !strlen(userData[useri].nickname)){                
                //Welcome Message                
                sprintf(buf, ":magic 001 %s :Welcome to the minimized IRC daemon!\r\n",
                userData[useri].nickname);
                send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 251 %s :There are %d users and 0 invisible on 1 server\r\n",
                userData[useri].nickname, numCli);
                send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                
                sprintf(buf, ":magic 375 %s :- magic Message of the day - \r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 375 %s :-『『『-----------------『『『 \r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|    (((___     (((___      |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|    |_O__     |_O__        |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|          <                |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|           ____/           |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|---------------------------|\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|       Life is hard        |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :-|    But we'll be okay      |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":magic 372 %s :- --------------------------- \r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
                sprintf(buf, ":mircd 376 %s :End of Message of the day\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            }
            sprintf(userData[useri].nickname,"%s", tmpName);
        } 
        else dealError(useri, 436); //(436) ERR_NICKCOLLISION
        return;

    } else if(strcmp(command, "USER")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1;cut<strlen(buf), lenName<NAME_LEN-1; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName]=buf[cut];
            lenName++;
        }
        if(strlen(tmpName)==0){
            dealError(useri, 461); //(461) ERR_NEEDMOREPARAMS
            return;
        }
            
        if(strlen(userData[useri].nickname)>0 && !strlen(userData[useri].username)){
            //welcome message
            sprintf(buf, ":magic 001 %s :Welcome to the minimized IRC daemon!\r\n",
                userData[useri].nickname);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);

            sprintf(buf, ":magic 251 %s :There are %d users and 0 invisible on 1 server\r\n",
            userData[useri].nickname, numCli);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);

            sprintf(buf, ":magic 375 %s :- magic Message of the day - \r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 375 %s :-『『『-----------------『『『 \r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|    (((___     (((___      |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|    |_O__     |_O__        |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|          <                |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|           ____/           |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|---------------------------|\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|       Life is hard        |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :-|    But we'll be okay      |\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":magic 372 %s :- --------------------------- \r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            sprintf(buf, ":mircd 376 %s :End of Message of the day\r\n",userData[useri].nickname); send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        }
        sprintf(userData[useri].username, "%s", tmpName);
        return;

        // numPara = 0;
        // lenName = 0;
        // char tmpNameArr[4][NAME_LEN];
        // memset(tmpNameArr, '\0', sizeof(tmpNameArr));
        
        // for(cut=cut+1; cut<strlen(buf), numPara<4;){
        //     lenName=0;
        //     for(;buf[cut]!=' ', buf[cut]!='\n', 
        //         cut<strlen(buf); cut++){
        //         tmpNameArr[numPara][lenName]=buf[cut];    
        //         lenName++;
        //     }
        //     if(strlen(tmpNameArr[numPara])!=0){
        //         printf("NAME %d: %s\n", numPara, &tmpName[numPara]);
        //         numPara++;
        //     }
        //     else break;
        //     printf("USER\n");
        // }
        // if(numPara<4) dealError(useri, 461); //(461) ERR_NEEDMOREPARAMS
        // else{
        //     sprintf(userData[useri].username, "%s", tmpNameArr[0]);
        //     sprintf(userData[useri].hostname, "%s", tmpNameArr[1]);
        //     sprintf(userData[useri].servername, "%s", tmpNameArr[2]);
        //     sprintf(userData[useri].realname, "%s", tmpNameArr[3]);
        // }
    } else if(strlen(userData[useri].nickname)==0 ||
              strlen(userData[useri].username)==0){
        dealError(useri, 451); //(451) ERR_NOTREGISTERED
        return;
    }
       
    if(strcmp(command, "PING")==0){
        char tmpBuf[MAXMES];
        numPara = 0;
        for(cut=cut+1; cut<strlen(buf); cut++){
            if(buf[cut]==' ' || buf[cut]=='\n' || buf[cut]=='\r')
                break;
            tmpBuf[numPara++] = buf[cut];
        }
        if(strlen(tmpBuf)==0){
            dealError(useri, 409); //(409) ERR_NOORIGIN
            return;
        }
        sprintf(buf, "PONG %s\r\n", tmpBuf);
        send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
    } else if(strcmp(command, "LIST")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1; cut<strlen(buf), lenName<NAME_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName++]=buf[cut];
        }
        
        if(strlen(tmpName)==0){
            sprintf(buf, ":magic 321 %s Channel :User Name\r\n",
            userData[useri].nickname);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            
            for(int i=0; i<numCha; i++){
                sprintf(buf, ":magic 322 %s %s %d :%s\r\n", userData[useri].nickname,
                channels[i].name, channels[i].numUser, channels[i].topic);
                send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            }
        } else{
            int channeli = findChannel(tmpName);
            if(channeli<0){
                dealError(useri, 403); //(403) ERR_NOSUCHCHANNEL
                return;
            }
            sprintf(buf, ":magic 321 %s Channel :User Name\r\n",
            userData[useri].nickname);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);

            sprintf(buf, ":magic 322 %s %s %d :%s\r\n", userData[useri].nickname,
            channels[channeli].name, channels[channeli].numUser, channels[channeli].topic);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        }
        sprintf(buf, ":magic 323 %s :End of List\r\n",
        userData[useri].nickname);
        send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        
    } else if(strcmp(command, "JOIN")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        // tmpName[0]='#';
        for(cut=cut+1; cut<strlen(buf), lenName<NAME_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName++]=buf[cut];
        }
        if(strlen(tmpName)==1){
            dealError(useri, 461); //(461) ERR_NEEDMOREPARAMS
            return;
        }
        int channeli = addJoinChannel(useri, tmpName);
        sprintf(buf, ":%s JOIN %s\r\n", userData[useri].nickname, tmpName);
        sendtoChannel(useri, channeli, 0, 1, 0);
        if(strlen(channels[channeli].topic)==0){
            sprintf(buf, ":magic 331 %s %s :No Topic is set\r\n",userData[useri].nickname, tmpName);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        } else{
            sprintf(buf, ":magic 332 %s %s :%s\r\n",userData[useri].nickname,
            tmpName, channels[channeli].topic);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        }
        
        namesList(useri, channeli);

    } else if(strcmp(command, "TOPIC")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1;cut<strlen(buf), lenName<NAME_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName]=buf[cut];
            lenName++;
        }
        if(strlen(tmpName)==0){
            dealError(useri, 461); //(461) ERR_NEEDMOREPARAMS
            return;
        }
        int channeli = findChannel(tmpName);
        if(channeli<0){
            dealError(useri, 403); //(403) ERR_NOSUCHCHANNEL
            return;
        }
        int onChanneli=onChannel(useri, channeli);
        if(onChanneli<0){
            dealError(useri, 442); //// (442) ERR_NOTONCHANNEL
            return;
        }

        char tmpBuf[TOPIC_LEN]="";
        lenPara=0;
        for(cut=cut+2;cut<strlen(buf), lenPara<TOPIC_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]=='\r') break;
            tmpBuf[lenPara]=buf[cut];
            lenPara++;
        }
        if(strlen(tmpBuf)==0){ //user ask the topic of channel
            if(strlen(channels[channeli].topic)==0){
                sprintf(buf, ":magic 331 %s %s :No topic is set\r\n",
                userData[useri].nickname, channels[channeli].name);
            } else{
                sprintf(buf, ":magic 332 %s %s :%s\r\n",
                userData[useri].nickname, channels[channeli].name, channels[channeli].topic);
            }
        } else{ //user set topic
            sprintf(channels[channeli].topic, "%s", tmpBuf);
            sprintf(buf, ":magic 332 %s %s :%s\r\n",
            userData[useri].nickname, channels[channeli].name, channels[channeli].topic);
        }
        send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        
    } else if(strcmp(command, "NAMES")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1;cut<strlen(buf), lenName<NAME_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName]=buf[cut];
            lenName++;
        }
        if(strlen(tmpName)==0){
            for(int i=0; i<channels.size(); i++){
                namesList(useri, i);
            }
            return;
        }
        int channeli = findChannel(tmpName);
        if(channeli<0){
            dealError(useri, 403); //(403) ERR_NOSUCHCHANNEL
            return;
        } else namesList(useri, channeli);
    } else if(strcmp(command, "PART")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1;cut<strlen(buf), lenName<NAME_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName]=buf[cut];
            lenName++;
        }
        if(strlen(tmpName)==0){
            dealError(useri, 461); //(461) ERR_NEEDMOREPARAMS
            return;
        }
        int channeli;
        if((channeli = findChannel(tmpName))<0){
            dealError(useri, 403); //(403) ERR_NOSUCHCHANNEL
            return;
        }
        int onChanneli;
        if((onChanneli=onChannel(useri, channeli))<0){
            dealError(useri, 442); //(442) ERR_NOTONCHANNEL
        }
        leaveChannel(useri, channeli, onChanneli, 0);
    } else if(strcmp(command, "USERS")==0){
        sprintf(buf, ":magic 392 %s :UserID              Terminal  Host\r\n", userData[useri].nickname);
        send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        int sockfd;
        for(int i=1; i<=maxi; i++){
            if((sockfd = client[i].fd)<0) continue;
            sprintf(buf, ":magic 393 %s :%-20s-         %s\r\n", 
            userData[useri].nickname, userData[i].nickname, userData[i].ipAddr);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
        }
        sprintf(buf, ":magic 394 %s :End of users\r\n", userData[useri].nickname);
        send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
    } else if(strcmp(command, "PRIVMSG")==0){
        lenName=0;
        memset(tmpName, '\0', sizeof(tmpName));
        for(cut=cut+1;cut<strlen(buf), lenName<NAME_LEN; cut++){
            if(buf[cut]=='\n' || buf[cut]==' '|| buf[cut]=='\r') break;
            tmpName[lenName]=buf[cut];
            lenName++;
        }
        if(strlen(tmpName)==0){
            dealError(useri, 411); //(411) ERR_NORECIPIENT
            return;
        }
        int channeli = findChannel(tmpName);
        if(channeli<0){
            dealError(useri, 401); //(401) ERR_NOSUCHNICK
            return;
        }
        int onChanneli;
        if((onChanneli = onChannel(useri, channeli))<0){
            dealError(useri, 442); //(442) ERR_NOTONCHANNEL
            return;
        }
        sendtoChannel(useri, channeli, cut, 0, 0);
    } else if(strcmp(command, "QUIT")==0){
        disconnect(useri);
    } else{
        dealError(useri, 421);
    }
}
void dealError(int useri, int error){
    // printf("error: %d\n", error);
    switch(error){
        case 401: //(401) ERR_NOSUCHNICK
            sprintf(buf, ":magic 401 %s %s :No such nick/channel\r\n",
                    userData[useri].nickname, tmpName);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 403: //(403) ERR_NOSUCHCHANNEL
            sprintf(buf, ":magic 403 %s %s :No such channel\r\n",
                    userData[useri].nickname, tmpName);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 409: // (409) ERR_NOORIGIN
            sprintf(buf, ":magic 409 %s :No origin specified\r\n",
                    userData[useri].nickname);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 411: // (411) ERR_NORECIPIENT
            sprintf(buf, ":magic 411 %s :No recipient given (PRIVMSG)\r\n",
                    userData[useri].nickname);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 412: // (412) ERR_NOTEXTTOSEND
            sprintf(buf, ":magic 412 %s :No text to send\r\n",
                    userData[useri].nickname);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 421: // (421) ERR_UNKNOWNCOMMAND
            sprintf(buf, ":mircd 421 %s %s :Unknown command\r\n",
                    userData[useri].nickname, command);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 431: // (431) ERR_NONICKNAMEGIVEN
            sprintf(buf, ":magic 431 :No nickname given\r\n");
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            //:mircd 431 :No nickname given
            break;
        case 436: // (436) ERR_NICKCOLLISION
            sprintf(buf, ":magic 436 %s :Nickname collision KILL\r\n", tmpName);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 442: // (442) ERR_NOTONCHANNEL
            sprintf(buf, ":magic 442 %s %s :You are not on that channel\r\n",
                    userData[useri].nickname, tmpName);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 451: // (451) ERR_NOTREGISTERED
            sprintf(buf, ":magic 451 :You have not registered\r\n");
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
        case 461: // (461) ERR_NEEDMOREPARAMS
            sprintf(buf, ":magic 461 %s %s :Not enought parameters\r\n",
                    userData[useri].nickname, command);
            send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
            break;
    }
}
void disconnect(int useri){
    for(int i=0; i<userData[useri].numInChannel; i++){
        leaveChannel(useri, userData[useri].inChannel[i], i, 1);
    }
    memset(userData[useri].nickname, '\0', NAME_LEN);
    memset(userData[useri].username, '\0', NAME_LEN);
    
    close(client[useri].fd);
    client[useri].fd=-1;
    if(useri==maxi){
        for(int i=maxi; i>=0; i--){
            if(client[i].fd>0){
                maxi = i;
                break;
            }
        }
    }
}
void namesList(int useri, int channeli){
    char nameBuf[MAXMES]="";
    int uNamei;
    for(int i=0; i<channels[channeli].numUser; i++){
        uNamei = channels[channeli].users[i];
        strcat(nameBuf, userData[uNamei].nickname);
        if(i!=channels[channeli].numUser-1) strcat(nameBuf, " ");
    }
    sprintf(buf, ":magic 353 %s %s :%s\r\n", userData[useri].nickname,
    channels[channeli].name, nameBuf);
    send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
    sprintf(buf, ":mircd 366 %s %s :End of Names List\r\n",
    userData[useri].nickname, channels[channeli].name);
    send(client[useri].fd, buf, strlen(buf), MSG_NOSIGNAL);
}