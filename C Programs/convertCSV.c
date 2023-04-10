/* Usage: ./convert <file to be converted> <new .csv file name> 
    to run at terminal: gcc -o convert convertCSV.c
    Open file to be converted
    Open/create csv file to be converted to
    Read line from file to be converted
    Swap old delimiter (line 38) with new delimiter (line 39)
    Write line with new delimiter to new file (likely CSV, but up to user)
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUFFER 1000

int main(int argc, char *argv[]){
    FILE *fp, *destination; 
    char fileBuffer[MAXBUFFER]; 
    memset(fileBuffer, 0, MAXBUFFER); 
    char *token; 

    if (argc <3){
        printf("Usage is ./convert <filename> < destinationFileName>\n"); 
    }

    
    fp = fopen(argv[1], "r"); 
    if (fp == NULL){
        printf("Error opening file to convert: %s\n", argv[1]); 
        exit(1);
    }
    destination = fopen(argv[2], "w+"); 
    if (destination == NULL){
        printf("Error opening destination file: %s\n", argv[2]); 
        exit(1);
    }
    char delim[]=" "; 
    char newDelim[] = ",";
    while (fgets(fileBuffer, MAXBUFFER, fp)){
        for (int i =0; i<strlen(fileBuffer); i++){
            if (fileBuffer[i] == delim[0]){
                fileBuffer[i] = newDelim[0]; 
            }
        }

        fputs(fileBuffer, destination); 
        memset(fileBuffer, 0, MAXBUFFER); 
    }


}