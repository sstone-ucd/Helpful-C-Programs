#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include "functions.h"

#define MAXBUFFER 200
#define MAXMESSAGES 500
#define MAXKVSIZE 80
#define ARGS 4
#define versionNum "8"
#define VALFLAGS 7
#define ttl 3
#define MAXPARTNERS 100
#define resend 5


/****************************PROGRAM START UP*******************************************************/
//check the IP passed in to the config file
int checkIP(char *string){
    struct sockaddr_in server_address; 
    if(inet_pton(AF_INET, string, &server_address)!=1){
        printf("Error with IP, not in correct IPV4 form: %s\n", string); 
        return (-1); 
    }
    return (1); 
}

//check the port number is a valid number
int checkPortNumber(char *string){
    //does the string contain only digits
    for (int i=0; i<strlen(string); i++){
        if (!isdigit(string[i])){
            printf("The Portnumber isn't a number:%c -> %s\n", string[i], string); 
            return (-1); 
        }
    }
    //is it a valid number
    int tempPort=atoi(string); 
    //exit if portnumber is too big or too small
    if ((tempPort > 65535) || (tempPort < 0)){
        //let user know the issue, return a negative code
        printf ("you entered an invalid port number: %d\n", tempPort);
        return(-1); 
    }
    //All tests passed, return positive code
    return (tempPort);
}

//check location is a valid number
int checkLocation(char *string){
    //does the string contain only digits
    for (int i=0; i<strlen(string); i++){
        if (!isdigit(string[i])){
            printf("The location isn't a number:%c -> %s\n", string[i], string); 
            return (-1); 
        }
    }
    //is it a valid number
   
    int tempLoc=atoi(string); 
    
    //exit if portnumber is too big or too small
    if (tempLoc < 0){
        //let user know the issue, return a negative code
        printf ("you entered an invalid location number: %d\n", tempLoc);
        return(-1); 
    }
    //All tests passed, return positive code
    return (tempLoc);
}

//replace whitespace with ^ charater to allow for tokenization
int swapWSpace(char buffer[]){
    if (strlen(buffer)<0){
        return (-1);
    }
    for (int i=0; i<strlen(buffer); i++){
    //if the char is a double quote, skip ahead until the next double quote
    if (buffer[i]=='\"'){
        //move one space to get into the quoted message
        i++; 
        //while inside the message keep moving forward 
        while (buffer[i]!='\"'){
        i++; 
        }//while
    }//if1
    //otherwise, replace the whitespace with a carrot
    if (buffer[i]==' '){
        buffer[i]='^';
    }//if2
    }//for
    return 1; 
}

//get grid data from user
int getGrid(struct grid *myGrid){

    /*printf("Enter Rows:\n"); 
    scanf("%d", &myGrid->row); 
    printf("Enter Columns:\n"); 
    scanf("%d", &myGrid->col); 
    //scanf("%[^\n]s", foo);
    getchar();*/

    if (myGrid->col > 0 && myGrid->row >0){
        myGrid->maxLocation = myGrid->col * myGrid->row; 
        return(myGrid->maxLocation); 
    }
    else{
        return (-1); 
    }
}

//run program with the correct arguements
int checkArgNum(int argc){
    if (argc < ARGS){
                printf("Run format is ./drone3 <portnumber> <row> <col>\n");
        return (-1); 
    }

return (1); 
}

//check valid arguements are passed in
int checkServerArgs(int argc, char argv[], struct configData *myServer){
    int rc = -1;
    rc = checkArgNum(argc);
    if (rc<0){
        exit(1); 
    }
    //check port number
    rc = checkPortNumber(argv);
    if (rc<0){
        exit(1); 
    }
    myServer->port = rc; 

    return(rc); 

}

//read in config file
int readConfig(struct configData serverArray[], struct configData *myServer){

    char configFileName[]="config.file"; 
    char configBuffer[200]; 
    FILE *configFile; 
    char *t1, *t2, *t3;
    int count = 0;//keep track of servers
    int rc;
    
    //get the file information
    //printf("Enter config file name\n"); 
    //scanf("%[^\n]s", configFileName);
    //getchar();

    //open the file
    configFile = fopen(configFileName, "r");
    if (configFile==NULL){
        printf("Error opening config file.n");
        return (-1);
    }
    
    //clear the buffer
    memset(configBuffer, 0, 80); 

    //while there are lines to read in the config file
    while(fgets(configBuffer, 200, configFile)){ 
        //for the length of the string just pulled
        char *cBufferDup = strdup(configBuffer); 
        for (int  i=0; i<strlen(configBuffer); i++){
            //tokenize the string 
            while ((t1= strsep(&cBufferDup," "))!=NULL){  
                while ((t2 = strsep(&cBufferDup," "))!=NULL){
                     if((t3 = strsep(&cBufferDup,"\n,\0"))!=NULL){ 
                        //check the value of the IP is valid
                        rc = checkIP(t1); 
                        if (rc<0){
                            return (-1); 
                        }
                        //if the IP is valid copy the string into the array
                        memcpy(serverArray[count].ip, t1, 80); 

                        //check the port number is valid                        
                        rc =checkPortNumber(t2); 
                        if (rc < 0){
                            return (-1); 
                        }
                        //if valid, checkPortNumber returns the port number
                        serverArray[count].port = rc; 
                        
                        //initialize the seg number to 0
                        serverArray[count].curSeqNum = 0; 

                        for(int i =0; i<MAXMESSAGES; i++){
                            serverArray[count].myAck[i]=0; 
                            serverArray[count].partnerAck[i]=0;
                        }


                        //get the location, check that its valid
                        rc = checkLocation(t3); 
                        if (rc<0){
                            return (-1); 
                        }
                        serverArray[count].location=rc; 

                        if (serverArray[count].port == myServer->port){
                            myServer->location = rc;
                        }
                       
                    }//while location token is valid
                }//while portnumber token is valid
                //increment the count for the servers, then read the next server
                count++;
            }//while ip is valid
        }//while we are still working on a string
    }//while we have data to read from the file
    if (myServer->location < 0){
        printf("Config file didnt contain location for your server\n"); 
        return(-1); 
    }
    return (count);    
    fclose(configFile);
}//read config file function


/****************************UPDATE MSG BEFORE SEND*******************************************************/
//add time to live to the buffer before sending it
int addTTL(char buffer[], int TTL){
    if (strlen(buffer)>190){
        printf("Command message too long to add TTL. Shorten command line message\n"); 
        return (-1); 
    }
    //get the TTL and push it into a string
    char ttlFromInt[4]; 
    memset(ttlFromInt, 0, 4);
    sprintf(ttlFromInt,"%d", TTL); 

    //get rid of the null character on the buffer
    buffer[strlen(buffer)-1]=0;

    //add the TTL key
    strcat(buffer, " TTL:"); 
    strcat(buffer, ttlFromInt); 
    return (strlen(buffer)); 
}

//add time to live to the buffer before sending it
int addVersion(char buffer[], int version){
    if (strlen(buffer)>190){
        printf("Command message too long to add TTL. Shorten command line message\n"); 
        return (-1); 
    }
    char addVersion[20]; 
    memset(addVersion, 0, 20); 
    sprintf(addVersion, " version:%d", version); 
    strcat(buffer, addVersion); 


    return (strlen(buffer)); 
}

//add the sequence number to the buffer
int addSeqNum(char buffer[], int seqNum){
    if (strlen(buffer)>190){
        printf("Command message too long to add TTL. Shorten command line message\n"); 
        return (-1); 
    }

    //get the seqNum and push it into a string
    char addSeqNum[12]; 
    memset(addSeqNum, 0, 12);
    sprintf(addSeqNum," seqNum:%d", seqNum); 

    
    //get rid of the null character on the buffer
    buffer[strlen(buffer)+1]=0;

    //add the sequence number to the buffer
    strcat(buffer, addSeqNum);
    
    //return the length of the buffer
    return (strlen(buffer));  
}

//add send-Path to the buffer
int addSendPath(char buffer[], int myPort){
    //check the buffer is valid
    if (strlen(buffer)>190){
        printf("Command message too long to add TTL. Shorten command line message\n"); 
        return (-1); 
    }

    //get the seqNum and push it into a string
    char addSendPath[20]; 
    memset(addSendPath, 0, 20);
    sprintf(addSendPath," send-path:%d", myPort); 

    //get rid of the null character on the buffer
    buffer[strlen(buffer)-1]=0;

    //add the sequence number to the buffer
    strcat(buffer, addSendPath);
    //add the null terminator back on
    buffer[strlen(buffer)]='\n'; 
    //return the length of the buffer
    return (strlen(buffer));
}

//get prompt from command line to forward to the network
int getCmdLine(char buffer[]){
    char *ptr = NULL; 
    //clear the buffer
    memset(buffer, 0, 200);
    //get information from the command line
    ptr = fgets(buffer, 200, stdin);
    if (ptr == NULL){
        printf("issue getting command line\n"); 
        return (-1); 
    }

    return(1); 

}

//add my location and port to end of the the commandline prompt for other drones to know where the message is from
int addMyServerData(char buffer[], struct configData *myServer){
    //check string length wont be exceeded by adding the location
    if (strlen(buffer)>190){
        printf("Command message too long to add location. Shorten command line message\n"); 
        return (-1); 
    }
    
    //get rid of the endline character in the buffer
    char loc[5]; 
    memset(loc, 0, 5);
    //get the location of my drone and turn the int to a string
    sprintf(loc,"%d", myServer->location); 
    //add location key
    strcat(buffer, " location:"); 
    //add location value
    strcat(buffer, loc);

    //add my server port data
    char port[6]; 
    memset (port, 0, 6); 
    sprintf(port, "%d", myServer->port); 
    strcat(buffer, " fromPort:"); 
    strcat(buffer, port); 

    //check the total string length buffer and new location key/value 
    int newLength = strlen(buffer);
    return(newLength);
}

/**************************** SEND CMD MESSAGE *******************************************************/
//send message to other drones one config file
void sendCmdMessage(struct servers *servers, int sd, char buffer[], struct configData *myServer){
    struct sockaddr_in server_address;//structures for addresses
    int rc=-1; 
    //for the number of servers saved
    for (int i=0; i<servers->numServers;i++){
        //For all servers in the config file that arent me
        if (servers->serverArray[i].port != myServer->port){
            //assign server address data
            server_address.sin_family = AF_INET; // use AF_INET addresses
            server_address.sin_port = htons(servers->serverArray[i].port); //convert port number
            server_address.sin_addr.s_addr = inet_addr(servers->serverArray[i].ip); //convert IP addr
            rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_address, sizeof(server_address));
            //check that send was successful, if not notify the user and end the loop
            if (rc < 0){
                //let the user know there was an issue
                perror ("sendto"); 
                exit(1); 
            }   
        }      
    }//for

}

/**************************** CHECK RCVD MESSAGE *******************************************************/
//listen for information on the network
int getNetworkData(char buffer[], int sd){
    socklen_t fromLength = sizeof (struct sockaddr_in); 
    struct sockaddr_in from_address;
    int rc =-1;
    rc = recvfrom(sd, buffer, MAXBUFFER, 0, 
                (struct sockaddr *)&from_address, &fromLength);
    if (rc <0){
        printf("Issue receiving data from the network\n"); 
        return (-1);
    }  
    return rc;          
}

//8a. compare coordinates to ensure message is within reach
double compareCoordinates(int location1, int location2, struct grid *myGrid){

    //x = location / column
    //y = location % column
    int x1 = (location1 -1) / myGrid->col; 
    int y1 = (location1 -1)% myGrid->col; 

    int x2 = (location2 -1) / myGrid->col; 
    int y2 = (location2 -1) % myGrid->col;

    double distance; 
    //Eucladian formula: sqrt((x2-x1)^2 + (y2-y1)^2)
    int xPow = pow((x2-x1), 2); 
    int yPow = pow((y2-y1), 2); 
    int sVal = xPow + yPow;
    distance = floor(sqrt(sVal)); 
    
    return (distance); 
}

//save data from network into key value pairs
int tokenizeNetworkData(char buffer[],struct kvPair tempPairs[]){ 
    char *token, *key, *value;
    int count =0; 

    
    int rc =-1; 

    //copy the buffered string so we can use strsep on it and not edit the original buffer
    char *string = strdup(buffer);

    //swap the whitespace in the message for a carrot
    rc= swapWSpace(string); 
    if (rc < 0){
        printf("Issue tokenizing the string from the network\n");
        return (-1);
    }
    //Tokenize the string into two a key and a value
    for (int i=0; i<strlen(buffer); i++){
      //parse the string at the carrot and colon symbol
      while ((token = strsep(&string, "^"))!= NULL){
        //strsep returns empty strings when it hits a delimiter, skip those
        if (strlen(token)>0){
          while ((key = strsep(&token, ":" ))!= NULL){
            //store the token into value
            value = strsep(&token, "\0\n"); 
            //clear the buffer of the temp
            memset(tempPairs[count].key, 0, MAXKVSIZE);
            memset(tempPairs[count].val, 0, MAXKVSIZE);
            //store the key and value pair into a struct  
            memcpy(tempPairs[count%MAXMESSAGES].key, key, strlen(key));
            memcpy(tempPairs[count%MAXMESSAGES].val, value, strlen(value));
            }//while the key is made, get the value
        }//while there is data in the token, get the key
        count++; 
      }//while we have the tokens between the carrots  
    }//for the length of the buffer
    
  return count; 
}

//validate network message
int validiateNetworkMesg(struct kvPair tempPairs[], int count, struct configData *myServer, struct grid *myGrid){
    //traverse the pairs to ensure it includes a location, port, and version 5
    //search terms
    char checkVersion[] = "version"; 
    char checkLocation[] = "location"; 
    char checkTTL[] = "TTL"; 
    char checkToPort[] = "toPort";
    char checkFromPort[] = "fromPort";
    char checkMsgType[] = "type"; 
    char checkMove[] = "move";
    char checkSendPath[] = "send-path";

    int myPort = myServer->port;
    int validate[] = {0,0,0,0,0,0,0,0,0,0};
    //validate int array: {0:checkVersion, 1:checkTTL, 2:checkOnGrid, 3:checkDistance, 
                        //4:checkToPortValid, 5:checkToPortMine, 6:checkFromPortValid, 7: checkIfAck
                        //8:checkIfMove 9:optimizeSendPath

    for (int i =0; i<count;i++){
        //check that the KV set includes version 5
        if ((strcmp(tempPairs[i].key, checkVersion))==0){
            if ((strcmp(tempPairs[i].val,versionNum))==0){
                validate[0]= 1;
            }
        }
        //check for TTL key
        if ((strcmp(tempPairs[i].key, checkTTL))==0){
            //if found key, check value >0
            int timeToLive = atoi(tempPairs[i].val);
            if (timeToLive > 0){
                validate[1] = 1; 
            }
        }
        //check for location key
        if ((strcmp(tempPairs[i].key,checkLocation))==0){
            //captupre the value as an int
            int fromLocation = atoi(tempPairs[i].val);
            //check the location is in the grid
            if (fromLocation < myGrid->maxLocation +1 && fromLocation>0){
                validate[2] = 1;
                //check that the location is within 2 grid spaces
                double distance = compareCoordinates(myServer->location, fromLocation, myGrid);
                if (distance < 3 && distance >= 0){
                    validate[3] = 1;
                }
            }     
        }
        //check toPort
        if ((strcmp(tempPairs[i].key,checkToPort))==0){
            int port; 
            
            port = atoi(tempPairs[i].val);
            //printf("%d\n", port);
            if ((port>0) && (port<65535)){
                validate[4] = 1;
                //check if the port in the message indicates the message is for me 
                if (port == myPort){
                    validate[5] = 1;
                }
            }
        }
        //check fromPort isnt mine (dont reforward my own message)
        if ((strcmp(tempPairs[i].key,checkFromPort))==0){
            int port; 
            port = atoi(tempPairs[i].val);
            //printf("%d\n", port);
            if ((port>0) && (port<65535)){
                if (port!=myServer->port){
                    validate[6] = 1;
                }
                
            }
        }
        //check if message is an ACK
        if ((strcmp(tempPairs[i].key,checkMsgType))==0){
            //verify if the type of message is an Ack
            if ((strcmp(tempPairs[i].val,"ACK"))==0){
                validate[7] = 1; 
            }
        }
        //check move
        if ((strcmp(tempPairs[i].key, checkMove))==0){
            validate[8] = 1; 
        }
        //check send path 
        if ((strcmp(tempPairs[i].key, checkSendPath))==0){
            char myCharPort[10];
            memset(myCharPort, 0, 10); 
            sprintf(myCharPort,"%d", myPort);
            if ((strstr(tempPairs[i].val, myCharPort))==NULL){
                validate[9]=1; 
            }
        }
    }
    
    /*printf("version:%d TTL:%d Grid:%d Distance:%d toPort:%d myMsg:%d from:%d ACK:%d Move:%d SendPath:%d\n", 
    validate[0],validate[1],validate[2],validate[3],validate[4],validate[5],validate[6],validate[7],validate[8],validate[9]);
    */
    int val = checkValFlags(validate); 
    return val;
}

//check the flags from validating the messages
int checkValFlags(int validate[]){
    //validate int array: {0:checkVersion, 1:checkTTL, 2:checkOnGrid, 3:checkDistance, 
                        //4:checkToPortValid, 5:checkToPortMine, 6:checkFromPortValid, 7: checkIfAck
                        //8:checkIfMove 9:optimizeSendPath}
    //version, my port move
    //already sent by me, ignore message
    if (validate[9] == 0){
        return 0; 
    }

    //version, my port move
    else if (validate[0]==1 && validate[5] == 1 && validate[8]==1){
        
        return 1; 
    }

    //message is valid and for me and not an ack
    else if (validate[0]==1 && validate[1]==1 && validate[2]==1 && validate[3]==1 && validate[4]==1 &&validate[5]==1 && validate[6]==1 && validate[7]== 0){
        return 2; 
    }
    //message is valid and needs to be forwarded and is not an ack
    else if (validate[0]==1 && validate[1]==1 && validate[2]==1 && validate[3]==1 && validate[4]==1 && validate[5]==0 && validate[6]==1 && validate[7]==0){
        return 3; 
    }
    //message is valid and needs to be forwarded and is an ack
    else if (validate[0]==1 && validate[1]==1 && validate[2]==1 && validate[3]==1 && validate[4]==1 && validate[5]==0 && validate[6]==1 && validate[7]==1){
        return 4; 
    }
    //message is valid, for me, and is an ack
    else if(validate[0]==1 && validate[1]==1 && validate[2]==1 && validate[3]==1 && validate[4]==1 &&validate[5]==1 && validate[6]==1 && validate[7]== 1){
        return 5; 
    }

    //message is not valid
    else{
        return 0; 
    }
}

//determine the toPort for the cmdline message
int getToPort(char buffer[]){
    char *token, *key, *value, *string;
    int count =0, toPort=-1; 

    //copy the buffered string so we can use strsep on it and not edit the original buffer
    string = strdup(buffer); 
    
    //swap the whitespace in the message for a carrot
    int rc =-1; 
    rc= swapWSpace(string); 
    if (rc < 0){
        printf("Error reading command line\n"); 
        return(rc); 
    }


    //Tokenize the string into two a key and a value
    for (int i=0; i<strlen(buffer); i++){
      //parse the string at the carrot and colon symbol
      while ((token = strsep(&string, "^"))!= NULL){
        //strsep returns empty strings when it hits a delimiter, skip those
        if (strlen(token)>0){
          while ((key = strsep(&token, ":" ))!= NULL){
            value = strsep(&token, "\0\n");
            if (strcmp(key, "toPort")==0){
            //store the token into value
                toPort = atoi(value); 
            }//if
            }//while
        }//while there is data in the token, get the key
        count++; 
      }//while we have the tokens between the carrots  
    }//for the length of the buffer


    if ((toPort<0) && (toPort>65535)){
        toPort = -1;
    } 

    return toPort;
}

//get int value given a key from the message
int getIntKeyValue(struct kvPair tempPairs[], int pairCount, char searchTerm[]){
    int intVal = -1; 
    for(int i =0; i<pairCount; i++ ){
        if(strcmp(tempPairs[i].key, searchTerm)==0){
            intVal = atoi(tempPairs[i].val); 
        }
         
    }
    return intVal; 
}

//get string value given a key from the message
void getStringKeyValue(struct kvPair tempPairs[], int numPairs, char searchTerm[], char value[]){
    memset(value, 0, 80); 
    for(int i =0; i<numPairs; i++ ){
        if(strcmp(tempPairs[i].key, searchTerm)==0){
            strcpy(value, tempPairs[i].val); 
        }
         
    }
 
}

//check if I have already acked the message I just recieved, if not, increment the ack
int checkIfDupMsg(struct servers *servers, int msgSeq, int msgFromPort){
    int rc =-1; 
    for (int i=0; i< servers->numServers; i++){
        //found the server that send the message
        if (servers->serverArray[i].port == msgFromPort){
            //check if I already acked the seqence in the message
            if (servers->serverArray[i].myAck[msgSeq]==0){
                //set ack to 1 to indicate ACK and return 0 to indicate not a dup message
                servers->serverArray[i].myAck[msgSeq]=1; 
                rc = 0; 
            }
            else if(servers->serverArray[i].myAck[msgSeq]>0){
                rc = 1; 
            }
        }
    }
    return rc; 
}

// USED BY FORWARDMESSAGE: get the send path from the message and compare it to the passed in port
int compareSendPath(char buffer[], int port){
    char charPort[10];
    memset(charPort, 0, 10); 
    sprintf(charPort,"%d", port); 
    //tokenize buffer to get sendpath

    char *token, *key, *value, *string, sendPath[80];
    memset(sendPath, 0, 80); 
    //copy the buffered string so we can use strsep on it and not edit the original buffer
    string = strdup(buffer); 
    //swap the whitespace in the message for a carrot
    swapWSpace(string); 
    //Tokenize the string into two a key and a value
    for (int i=0; i<strlen(buffer); i++){
      //parse the string at the carrot and colon symbol
      while ((token = strsep(&string, "^"))!= NULL){
        //strsep returns empty strings when it hits a delimiter, skip those
        if (strlen(token)>0){
          while ((key = strsep(&token, ":" ))!= NULL){
            value = strsep(&token, "\0\n");
                if ((strcmp(key, "send-path"))==0){
                    strcpy(sendPath, value); 
                }
            }//while
        }//while there is data in the token, get the key
      }//while we have the tokens between the carrots  
    }//for the length of the buffer
    
    if((strstr(sendPath, charPort))!=NULL){
        return (-1); 
    }
    return(1); 
}

//check it the message in the buffer recvd is stored in the messages array, return its index
int checkMsgStorage(struct message messages[], int seq, int toPort, int fromPort, int msgCount){
    int messagesIndex =-1;
    int i = 0; 
    char seqBuf[80], toPortBuf[80], fromPortBuf[80]; 
    sprintf(seqBuf, "seqNum:%d", seq); 
    sprintf(toPortBuf, "toPort:%d", fromPort); 
    sprintf(fromPortBuf, "fromPort:%d", toPort); 
     
    while(i<msgCount){
        if (strstr(messages[i].messageString, seqBuf)!=NULL){
             
            if (strstr(messages[i].messageString, toPortBuf)!=NULL){
                
                if(strstr(messages[i].messageString, fromPortBuf)!=NULL){
                    messagesIndex = i; 
        }   }   }
        i++;
    }


    return messagesIndex; 
}

/*recieved a message of type ACK, need to check it this ack has been recieved before
find fromPort in partner server list and increment the sequence number*/
int checkDupAck(int msgSeq, int msgFromPort, struct servers *servers){
    int numServers = servers->numServers;
    int rc = -1; 
     
    //traverse through server array to find toPort in list
    for (int i=0; i<numServers;i++){
        
        //if the server in the list contains the toPort for my message
        if (servers->serverArray[i].port == msgFromPort){
            //check the sequence number that exisits for that server
           //check if partner already acked the seqence in the message
            if (servers->serverArray[i].partnerAck[msgSeq]==0){
                //set ack to 1 to indicate ACK and return 0 to indicate not a dup message
                servers->serverArray[i].partnerAck[msgSeq]=1; 
                rc = 0; 
            }
            else if(servers->serverArray[i].partnerAck[msgSeq]>0){
                rc = 1; 
            }
        }
    }//for
    return rc;  
}

/**************************** UPDATE/FORWARD RCVD MESSAGE *******************************************************/
//update location and TTL tokens, then save data to buffer to Ack the message
int updateMessage(struct kvPair tempPairs[], char forwardBuffer[], int numPairs, int location, int myPort){ 
    int ttlVal;
    memset(forwardBuffer, 0, MAXBUFFER);
    for (int i=0; i<numPairs; i++){                 
        //decrement the TTL
        if (strcmp(tempPairs[i].key, "TTL")==0){
            //decrement the TTL
            ttlVal =  atoi(tempPairs[i].val);
            if (ttlVal > 0){
                ttlVal --;
            }
            else{
                ttlVal = 0; 
            }
            //convert it to a string
            char sTTL[5]; 
            memset(sTTL, 0, 5);
            sprintf(sTTL,"%d", ttlVal); 
            //update the messages array
            strcpy(tempPairs[i].val, sTTL);
        }
        //update the send path
        else if (strcmp(tempPairs[i].key, "send-path")==0){
            char temp[10]; 
            sprintf(temp, "%d", myPort);
            if (strstr(tempPairs[i].val, temp)==NULL){
                char addPort[20]; 
                memset(addPort, 0, 20); 
                sprintf(addPort, ",%d", myPort); 
                strcat(tempPairs[i].val, addPort);
            }
 
        }
        //update the location
        else if (strcmp(tempPairs[i].key, "location")==0){
            char loc[5]; 
            memset(loc, 0, 5);
            sprintf(loc,"%d", location);
            
            //update the messages array with my location
            strcpy(tempPairs[i].val, loc);
        }
        //copy the data into the forward buffer if the TTL is still valid
        //copy the data in to the forwarding buffer
        strcat(forwardBuffer, tempPairs[i].key);
        strcat(forwardBuffer, ":");
        strcat(forwardBuffer, tempPairs[i].val);
        //add the extra space if there is more data coming
        if(i!=numPairs-1){
            strcat(forwardBuffer, " "); 
        }
    }   
    forwardBuffer[strlen(forwardBuffer)+1]='\n'; 
    return (strlen(forwardBuffer));
}

//find toPort in partner server list and increment the sequence number
int incrementSeqNum(int toPort, struct servers *servers){
    int seqNum=-1; 
    int numServers = servers->numServers;

     
    //traverse through server array to find toPort in list
    for (int i=0; i<numServers;i++){
        //if the server in the list contains the toPort for my message
        if (servers->serverArray[i].port == toPort){
            //increment the sequence number
            servers->serverArray[i].curSeqNum++; 
            seqNum = servers->serverArray[i].curSeqNum;
        }      
    }//for

    return seqNum;  
}

//add Ack message type
int addMsgType(char buffer[]){
    if (strlen(buffer)>190){
        printf("Command message too long to add TTL. Shorten command line message\n"); 
        return (-1); 
    }

    //get the seqNum and push it into a string
    char addMsgType[]=" type:ACK"; 

    
    //get rid of the null character on the buffer
    buffer[strlen(buffer)-1]=0;

    //add the sequence number to the buffer
    strcat(buffer, addMsgType);
    //add the null terminator back on
    buffer[strlen(buffer)]='\n'; 
    //return the length of the buffer
    return (strlen(buffer));      
}

//update toPort and fromPort and RETURN the sequence number of the message
int updateAckMsg(struct kvPair tempPairs[], char ackBuffer[], int numPairs, int msgFromPort, int myPort, int loc){
    char portBuff[10]; 
    memset(ackBuffer, 0, MAXBUFFER);
    int sequence=-1; 
    
    //for the number of key value pairs in the message    
    for (int i = 0; i<numPairs; i++){           
        if (strcmp(tempPairs[i].key, "toPort")==0){
            //turn toPort to a string
            memset(portBuff, 0, 10); 
            sprintf(portBuff, "%d", msgFromPort);
            //update the toPort to the previous sender
            strcpy(tempPairs[i].val, portBuff); 
        }
        //update the from port to my port number
        else if (strcmp(tempPairs[i].key, "fromPort")==0){
            memset(portBuff, 0, 10); 
            sprintf(portBuff, "%d", myPort);
            strcpy(tempPairs[i].val, portBuff);
        }
        //get the sequence number of the message
        else if (strcmp(tempPairs[i].key, "seqNum")==0){
            sequence = atoi(tempPairs[i].val);

        }
        //reset the TTL
        else if (strcmp(tempPairs[i].key, "TTL")==0){
            char ttlBuff[5]; 
            //turn TTL to a string
            memset(ttlBuff, 0, 5); 
            sprintf(ttlBuff, "%d", ttl);
            //update the toPort to the previous sender
            strcpy(tempPairs[i].val, ttlBuff); 

        }
        else if (strcmp(tempPairs[i].key, "send-path")==0){
            char tmpBuf[MAXKVSIZE]; 
            //turn TTL to a string
            memset(tmpBuf, 0, MAXKVSIZE); 
            sprintf(tmpBuf, "%d", myPort);
            //update the toPort to the previous sender
            strcpy(tempPairs[i].val, tmpBuf); 

        }
        else if (strcmp(tempPairs[i].key, "location")==0){
            char tmpBuf[MAXKVSIZE]; 
            //turn TTL to a string
            memset(tmpBuf, 0, MAXKVSIZE); 
            sprintf(tmpBuf, "%d", loc);
            //update the toPort to the previous sender
            strcpy(tempPairs[i].val, tmpBuf); 

        }

        //copy the data into the forward buffer if the TTL is still valid
        //copy the data in to the forwarding buffer
        strcat(ackBuffer, tempPairs[i].key);
        strcat(ackBuffer, ":");
        strcat(ackBuffer, tempPairs[i].val);
        //add the extra space if there is more data coming
        if (i!=(numPairs -1))
            strcat(ackBuffer, " "); 
            
    }
     
    strcat(ackBuffer, " type:ACK"); 
    ackBuffer[strlen(ackBuffer)+1]='\n'; 
    return (sequence);
}

//forward message to other servers
void forwardMessage(struct servers *servers, int sd, char buffer[], struct configData *myServer){
    
    struct sockaddr_in server_address;//structures for addresses

    int rc=-1; 
    
    //for the number of servers saved
    for (int i=0; i<servers->numServers;i++){
        //For all servers in the config file that arent me
        if (servers->serverArray[i].port != myServer->port){
            if ((rc = compareSendPath(buffer, servers->serverArray[i].port))>0){
                //assign server address data
                server_address.sin_family = AF_INET; // use AF_INET addresses
                server_address.sin_port = htons(servers->serverArray[i].port); //convert port number
                server_address.sin_addr.s_addr = inet_addr(servers->serverArray[i].ip); //convert IP addr
                rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_address, sizeof(server_address));
                //check that send was successful, if not notify the user and end the loop
                if (rc < 0){
                    //let the user know there was an issue
                    perror ("sendto"); 
                    exit(1); 
                }
            }    
        }      
    }//for
}

//print key and value pairs and add location
void printMessage(struct kvPair tempPairs[], int pairCount, int location){
    char h1[]="Key"; 
    char h2[]="Value";
    printf("\t\t%-15s\t -> \t %-15s\n", h1, h2);
    
    for(int i =0; i<pairCount; i++ ){
        printf("\t\t%-15s\t -> \t %-15s\n",tempPairs[i].key, tempPairs[i].val);
    }
    char s[] = "myLocation";
    printf("\t\t%-15s\t -> \t %-15d",s, location); 
    printf("\n\n"); 
}





