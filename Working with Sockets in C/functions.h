

//server struct to collect neighbor server data
struct configData{
    char ip[20]; 
    int port; 
    int location;
    int curSeqNum; 
    //list of acks from my partner
    int partnerAck[100]; 
    //list of acks to my partner
    int myAck[100];
};

//array to save server data
struct servers{
    struct configData serverArray[50]; 
    int numServers; 
};

//store key value pairs
struct kvPair{
    char key[80]; 
    char val[80]; 
};

//store key value pairs into messages
struct message{
    char messageString[200]; 
    int numPairs; 
    int myMessage; 
    int resendTTL;
    struct kvPair pairs[100]; 
};


//store grid
struct grid{
    int row; 
    int col; 
    int maxLocation; 

};
/****************************PROGRAM START UP*******************************************************/
//check the IP passed in to the config file
int checkIP(char *string); 

//check the port number is a valid number
int checkPortNumber(char *string); 

//check location is a valid number
int checkLocation(char *string); 

//replace whitespace with ^ charater to allow for tokenization
int swapWSpace(char buffer[]);

//get grid data from user
int getGrid(struct grid *myGrid); 

//run program with the correct arguements
int checkArgNum(int argc);

//check valid arguements are passed in
int checkServerArgs(int argc, char argv[], struct configData *myServer);

//read the config file information to store information about networked drones
int readConfig(struct configData serverArray[], struct configData *myServer); 

/**************************** UPDATE MSG BEFORE SEND*******************************************************/
//add time to live to the buffer before sending it
int addTTL(char buffer[], int TTL);

//add version to the buffer before sending it
int addVersion(char buffer[], int version); 

//append sequence number to message
int addSeqNum(char buffer[], int seqNum);

//add send-Path to the buffer
int addSendPath(char buffer[], int myPort);

//get prompt from command line to forward to the network
int getCmdLine(char buffer[]);

//add my location to end of the the commandline prompt for other drones to know where the message is from
int addMyServerData(char buffer[], struct configData *myServer);

/**************************** SEND CMD MESSAGE *******************************************************/
//send message to other drones one config file
void sendCmdMessage(struct servers *servers, int sd, char buffer[], struct configData *myServer);

/**************************** CHECK RCVD MESSAGE *******************************************************/
//listen for information on the network
int getNetworkData(char buffer[], int sd);

//get coordinates
double compareCoordinates(int location1, int location2, struct grid *myGrid); 

//tokenize the network data to more easily validate the message
int tokenizeNetworkData(char buffer[],struct kvPair tempPairs[]);

//validate network message
int validiateNetworkMesg(struct kvPair tempPairs[], int count, struct configData *myServer, struct grid *myGrid);

//check the flags from validating the messages
int checkValFlags(int validate[]);

//determine the toPort in the cmd line message and return the toPort
int getToPort(char buffer[]);

//get int value given a key from the message
int getIntKeyValue(struct kvPair tempPairs[], int messagesCount, char searchTerm[]); 

//get string value given a key from the message
void getStringKeyValue(struct kvPair tempPairs[], int numPairs, char searchTerm[], char value[]); 

//check if I have already acked the message I just recieved, if not, increment the ack
int checkIfDupMsg(struct servers *servers, int msgSeq, int msgFromPort);

//get the send path from the message to ensure the message is only forwarded to those who have not recieved the message before
int compareSendPath(char buffer[], int port);

//check it the message in the buffer recvd is stored in the messages array, return its index
int checkMsgStorage(struct message messages[], int seq, int toPort, int fromPort, int msgCount);

/*recieved a message of type ACK, need to check it this ack has been recieved before
find fromPort in partner server list and increment the sequence number*/
int checkDupAck(int msgSeq, int msgFromPort, struct servers *servers);

/**************************** UPDATE/FORWARD RCVD MESSAGE *******************************************************/
//update location and TTL tokens, then save data to buffer
int updateMessage(struct kvPair tempPairs[], char forwardBuffer[], int numPairs, int location, int myPort);

//find toPort in partner server list and increment the sequence number
int incrementSeqNum(int toPort, struct servers *servers);

//add Ack message type
int addMsgType(char buffer[]); 

//update toPort and fromPort and RETURN the sequence number of the message
int updateAckMsg(struct kvPair tempPairs[], char ackBuffer[], int numPairs, int msgFromPort, int myPort, int loc);

//send message to other drones on config file
void forwardMessage(struct servers *servers, int sd, char buffer[], struct configData *myServer);

//print key and value pairs and add location
void printMessage(struct kvPair tempPairs[], int pairCount, int location);












