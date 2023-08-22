#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#include "functions.h"

#define STDIN 0
#define MAXBUFFER 200
#define MAXMESSAGES 500
#define ttl 3
#define version 8
#define resend 6


int main(int argc, char *argv[])
{
    int rc =-1;
    int numServers =-1;
    static int messagesCount = 0;
    struct servers servers; 
    struct configData myServer;
    fd_set socketFDS; // the socket descriptor set
    int maxSD; // tells the OS how many sockets are set
    int sd, toPort, seqNum;//track server information
    struct timeval timeout;

    char cmdBuffer[MAXBUFFER]; //buffer to hold command line
    char socketBuffer[MAXBUFFER]; //buffer to hold data from network
    char forwardBuffer[MAXBUFFER]; //buffer to hold data to forward
    char ackBuffer[MAXBUFFER];//buffer to hold the data to be acked back
    struct sockaddr_in server_address;
    char sendPath[80]; 
    struct message messages[MAXMESSAGES];//hold the messages from incoming servers
    struct kvPair tempPairs[MAXMESSAGES]; 
    struct grid myGrid; 

    //initialize myServer location
    myServer.location=-1;
    
    //check that a port was received via command line
    rc = checkServerArgs(argc, argv[1], &myServer); 
    
    //read the config file and save my server location
    numServers = readConfig(servers.serverArray, &myServer); 
    if (numServers<0){
        printf("Check your config file and try again!\n"); 
        exit(1); 
    }
    //save the number of servers read from the config file
    if (numServers>0){
        servers.numServers = numServers;
    }

    printf("Server Location: %d\n", myServer.location); 

    myGrid.col=atoi(argv[3]);
    myGrid.row=atoi(argv[2]);

    printf("Grid: %dx%d\n", myGrid.row, myGrid.col);

    rc = getGrid(&myGrid); 


    //create a socket descriptor for the server(s)
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    //ensure socket is valid 
    if (sd <0){
    perror("Error: ");
    exit (1); 
    }

    //Create a socket to speak to the network  
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(myServer.port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    rc = bind (sd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (rc < 0){
        perror("bind");
        exit (1);
    }

    
   for (;;){                
        memset (cmdBuffer, 0, MAXBUFFER);
        // do the select() stuff here
        FD_ZERO(&socketFDS);// NEW                                 
        FD_SET(sd, &socketFDS); //NEW - sets the bit for the initial sd socket
        FD_SET(STDIN, &socketFDS); // NEW tell it you will look at STDIN too
        if (STDIN > sd) // figure out what the max sd is. biggest number
        {   maxSD = STDIN;}
        else
        {   maxSD = sd;}
        timeout.tv_sec = 20;
        timeout.tv_usec = 0;    
        rc = select (maxSD+1, &socketFDS, NULL, NULL, &timeout); // NEW block until something arrives
        if(rc == 0){
            printf ("timeout!\n");
            int i =0; 
            struct kvPair forwardTemp[MAXMESSAGES]; 
            while (i<messagesCount){
                if (messages[i].resendTTL>0){
                    int numPairs = tokenizeNetworkData(messages[i].messageString, forwardTemp);
                    updateMessage(forwardTemp, forwardBuffer, numPairs, myServer.location, myServer.port); 
                    printf("Forwarding message %s\n", messages[i].messageString); 
                    forwardMessage(&servers, sd, messages[i].messageString, &myServer);
                    messages[i].resendTTL --;
                }
                i++;
            }
            continue;
        }
        if (FD_ISSET(STDIN, &socketFDS)){ // means i received something from the keyboard. 
            //get the command line from the user
            rc = getCmdLine(cmdBuffer); 
            if (rc <0){
                break;
            }
            //determine toPort 
            toPort = getToPort(cmdBuffer);
            if (toPort <0){
                printf("Invalid toPort\n"); 
                break; 
            } 
            
            addSendPath(cmdBuffer, myServer.port);
            //add TTL
            rc = addTTL(cmdBuffer, ttl); 
            //add my server location to the buffer
            rc = addMyServerData(cmdBuffer, &myServer); 
            if (rc <0){
                break;
            }
            addVersion(cmdBuffer, version); 
            //increment current seqence number for specific server and save the message (if not a move command)
            if (strstr(cmdBuffer, "move")==NULL){
                seqNum = incrementSeqNum(toPort, &servers);
                //addpend sequence number
                addSeqNum(cmdBuffer, seqNum); 
                memset(messages[messagesCount].messageString, 0, MAXBUFFER); 
                strcpy(messages[messagesCount%MAXMESSAGES].messageString, cmdBuffer);
                messages[messagesCount%MAXMESSAGES].resendTTL = resend; 
                messagesCount++; 
            } 
            
            //send the command line buffer with location
            sendCmdMessage(&servers, sd, cmdBuffer, &myServer); 

        }
        if (FD_ISSET(sd, &socketFDS)){
            memset (socketBuffer, 0, MAXBUFFER); 
            //get something from the network
            rc = getNetworkData(socketBuffer, sd); 
            if (rc<=0){
                exit(1);
            }

            //tokenize recieved data and store it in messages array
            rc = tokenizeNetworkData(socketBuffer,tempPairs);
            if (rc <=0){
                printf("Tokenization Error\n"); 
                exit(1); 
            }
            int numPairs = rc;
            
            //validate the tokens
            int val = validiateNetworkMesg(tempPairs, numPairs, &myServer, &myGrid);

            int myLoc = myServer.location;
            int myPort = myServer.port; 
            int msgSeq = getIntKeyValue(tempPairs, numPairs, "seqNum"); 
            int msgFromPort = getIntKeyValue(tempPairs, numPairs, "fromPort"); 
            int msgToPort = getIntKeyValue(tempPairs, numPairs, "toPort"); 
            getStringKeyValue(tempPairs, numPairs, "send-path", sendPath); 

            //message is a move command for me, update location
            if (val == 1){
                struct kvPair forwardTemp[MAXMESSAGES]; 
                int move = getIntKeyValue(tempPairs, numPairs, "move"); 
                myServer.location = move; 
                printf("--------------------MOVED TO LOCATION:%d --------------\n", myServer.location);
                int i = 0; 
                if (messagesCount> 0){
                    while (i < messagesCount){
                        
                        int numPairs = tokenizeNetworkData(messages[i].messageString, forwardTemp); 
                        updateMessage(forwardTemp, messages[i].messageString, numPairs, myServer.location, myPort); 
                        printf("Forwarding after move: %s\n", messages[i].messageString); 
                        forwardMessage(&servers, sd, messages[i].messageString, &myServer);
                        messages[i].resendTTL --;
                        i++; 
                        
                    } 
                }   
            }
            //message is for me and its not an ACK
            if (val == 2){
                //check if I've already recieved and acked this message back
                int rc = checkIfDupMsg(&servers, msgSeq, msgFromPort); 
                if (rc == 0){
                    //print the message format the ack and send it
                    printf("--------------------RECEIVED MESSAGE FOR SEQ:%d FROM:%d--------------\n", msgSeq, msgFromPort);
                    printMessage(tempPairs, numPairs, myLoc); 
                    updateAckMsg(tempPairs, ackBuffer, numPairs, msgFromPort, myPort, myLoc);
                    forwardMessage(&servers, sd, ackBuffer, &myServer);
                    printf("--------------------SENDING ACK FOR SEQ:%d TO:%d---------------------\n", msgSeq, msgFromPort);; 
                }
                else if (rc == 1){
                    forwardMessage(&servers, sd, ackBuffer, &myServer);
                    printf("------------------SENDING DUP ACK FOR SEQ:%d TO:%d------------------\n", msgSeq, msgFromPort);
                } 
            }
            //message is for somone else, not an ack
            else if (val == 3){
                //update the message to forward
                updateMessage(tempPairs, forwardBuffer, numPairs, myLoc, myPort); 
                memset(messages[messagesCount%MAXMESSAGES].messageString, 0, MAXBUFFER);
                strcpy(messages[messagesCount%MAXMESSAGES].messageString, socketBuffer); 
                messagesCount++; 
                //set resendTTL
                messages[messagesCount%MAXMESSAGES].resendTTL=resend; 
                //forward the message
                printf("Forwarding message\n"); 
                forwardMessage(&servers, sd, forwardBuffer, &myServer); 
            }
            //message is for somoneone else and is an ack
            else if (val == 4){
                printf("RECEIVED ACK FOR SOMEONE ELSE\n"); 
                //reset the resendTTL
                int msgIndex = checkMsgStorage(messages, msgSeq, msgToPort, msgFromPort, messagesCount);  
                //if the message being acked is in my storage, reset the OG message resendTTL
                if(msgIndex>-1){
                    printf("removing from message stack: %s\n" ,messages[msgIndex].messageString);
                    messages[msgIndex].resendTTL = 0; 
                     
                }
                //update message send path, TTL, and location
                updateMessage(tempPairs, forwardBuffer, numPairs, myLoc, myPort);
                //store the ack to send
                memset(messages[messagesCount%MAXMESSAGES].messageString, 0, MAXBUFFER);
                strcpy(messages[messagesCount%MAXMESSAGES].messageString, forwardBuffer); 
                messagesCount++; 
                //set resendTTL
                messages[messagesCount%MAXMESSAGES].resendTTL=resend; 
                //forward the message
                forwardMessage(&servers, sd, forwardBuffer, &myServer);
                  

            }
            //message is for me and is an ack
            else if (val == 5){
                
                rc = checkDupAck(msgSeq, msgFromPort, &servers); 
                //not a duplicate
                if (rc == 0){
                     int msgIndex = checkMsgStorage(messages, msgSeq, msgToPort, msgFromPort, messagesCount);  
                    //mark partner sequence as acked 
                    if (msgIndex>-1){
                        messages[msgIndex].resendTTL = 0;
                    } 
                    printf("----------------RCVD ACK FOR SEQ:%d FROM:%d-----------------\n", msgSeq, msgFromPort);
                    printMessage(tempPairs, numPairs, myLoc); 
                }
                //duplcate ack rcvd
                else if(rc ==1){
                    printf("----RCVD DUPLICATE ACK FOR SEQ:%d FROM:%d SENDPATH:%s------\n", msgSeq, msgFromPort, sendPath);
                }
            }
        }//if network message
    }//forever loop
    return (1); 
}