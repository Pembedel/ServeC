/* UDP_Client.c
 *
 * Client implementation of the Server-Client 
 * UDP interaction
 *
 * Client sends three messages to Server
 *  - To set the value for a specific key in Server
 *     --set <key> <value>
 *  - To get the value for a key from Server
 *     --get <key>
 *  - To delete a key-value pair from Server db
 *     --del <key>
 *  - To stop server and delete all entries
 *      --fin fin
 *
 *  Author: Kapil
 *
 */
 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

/* Maximum number of characters in command*/
#define MAXLINE 1024 
#define MAXCHAR 256

#define IPADDR 1
#define PORTNUM 2

#define MIN_PORTNO 1
#define MAX_PORTNO 65535

#define FAILURE -1
#define SUCCESS 0

/* Function prototypes*/
void error(char *msg);
int validate_ip_addr(char *ip_addr);

/* main function*/
int main(int argc, char **argv) 
{ 
    int sockfd; 
    int portno;
    int num_bytes;
    int count = 1;
    char *ip_addr;
    char *port_num;
    char buffer[MAXLINE]; 
    char *split_str;
    struct sockaddr_in servaddr; 
    unsigned int len;
    
    /*validate input parameters*/
    if (argc < 5) 
    {
      printf("usage: %s --server <ipaddress>:<port> --get <key>\n", argv[0]);
      printf("usage: %s --server <ipaddress>:<port> --set <key> <value>\n", argv[0]);
      printf("usage: %s --server <ipaddress>:<port> --del <key>\n", argv[0]);
      printf("usage: %s --server <ipaddress>:<port> --fin fin\n", argv[0]);

      error("Incorrect Input");
    }
    /* Validate the commands allowed*/
    if (strncmp(argv[1],"--server",8)!=0)
    {
        error("Incorrect Input : --server expected");
    }

    if (!(strncmp(argv[3],"--set",5)==0 || strncmp(argv[3],"--get",5)==0 || strncmp(argv[3],"--del",5)==0 || strncmp(argv[3],"--fin",5)==0))
    {
        error("Incorrect Input : --set or --get or --del expected");
    }

    /* Validate Command formats*/
    if (strncmp(argv[3],"--set",5)==0 && argc != 6)
    {
        error("Incorrect Input : --set <key> <value> expected");
    }
    if (strncmp(argv[3],"--get",5)==0 && argc != 5)
    {
        error("Incorrect Input : --get <key> expected");
    }
    if (strncmp(argv[3],"--del",5)==0 && argc != 5)
    {
        error("Incorrect Input : --del <key> expected");
    }

    /* Max limit of 256 Characters*/
    if (strlen(argv[4]) > MAXCHAR)
    {
        error("Incorrect Input : Key length >256");
    }
    if (argc == 6 && strlen(argv[5]) > MAXCHAR)
    {
        error("Incorrect Input : value length >256");
    }
    /* End of validation*/

    /*Separating ipaddr and portno from <ipaddr>:<portno> format*/    
    split_str = strtok(argv[2],":");

    while (split_str != NULL)
    {
        switch (count)
        {
            case IPADDR:
                ip_addr = (char*)malloc(strlen(split_str)+1);
                strncpy(ip_addr,split_str,strlen(split_str)+1);
                ip_addr[strlen(split_str)]='\0';
            break;

            case PORTNUM:
                port_num = (char*)malloc(strlen(split_str)+1);
                strncpy(port_num,split_str,strlen(split_str)+1);
                port_num[strlen(split_str)]='\0';
            break;

            default:
            break;
        }
        split_str = strtok (NULL, ":");
        count++; 
    }

    /* Validation of ip address and port number*/
    if (port_num == NULL || ip_addr == NULL)
    {
        error("Incorrect IP addr and port input");
    }
    else
    {
        portno = atoi(port_num);
        
        if (portno < MIN_PORTNO || portno > MAX_PORTNO)
        {
            error("portnumber invalid");
        }
        if(validate_ip_addr(ip_addr) != 0)
        {
            error("ip_address invalid");
        }
    }
    printf("\nPort number:%d, ipaddr:%s",portno, ip_addr);


    /* Creating socket file descriptor*/ 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("Socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    memset(&servaddr, 0, sizeof(servaddr)); 
    
    /* Filling server information*/ 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(portno); 
    servaddr.sin_addr.s_addr = inet_addr(ip_addr); 
    
    /* message to be sent*/
    memset(&buffer, 0, sizeof(buffer)); 
    strcat(buffer,argv[3]);
    strcat(buffer," ");
    strcat(buffer,argv[4]);

    /* Only in case of --set*/
    if (argc > 5)
    {
        strcat(buffer," ");
        strcat(buffer,argv[5]);
    }    
    
    /* Send UDP message to Server*/
    sendto(sockfd, (const char *)buffer, strlen(buffer), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
            sizeof(servaddr)); 
    printf("\nMessage sent to Server:%s\n",buffer); 
    
    /* Receive a response from Server */
    memset(&buffer, 0, sizeof(buffer));    
    num_bytes = recvfrom(sockfd, (char *)buffer, MAXLINE, 
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len); 
    buffer[num_bytes] = '\0'; 
    
    printf("Server response: %s\n", buffer); 
    free(ip_addr);
    free(port_num);
    close(sockfd);
    return 0;
} 

/* Function: validate_ip_addr() - To validate IP addr 
 * in parameters: 
 *   ip_addr_in - IP addr to be validated
 *
 * return:
 *   status of validation
 */

int validate_ip_addr(char *ip_addr_in)
{
    char *split_str;
    int count = 0;
    int temp_ip=0;
    int status = 0;
    char *ip_addr;

    ip_addr = (char*)malloc(strlen(ip_addr_in)+1);
    strncpy(ip_addr,ip_addr_in,strlen(ip_addr_in)+1);

   /*Separating ipaddr based on <num>.<num>.<num>.<num> format*/      
    split_str = strtok(ip_addr,".");

    while (split_str != NULL)
    {
        status = FAILURE;

        /* Only 3 dots in IP addr*/
        if(count > 3)
            break;

        temp_ip = atoi(split_str);

        if(temp_ip>=0 && temp_ip < 256)
        {
            status = SUCCESS;
        }

        split_str = strtok (NULL, ".");
        count++;
 
    }

    if (count < 4)
        status = FAILURE;
    
    free(ip_addr);
    return status;
}


/* Function: error() - To print error message 
 * in parameters: 
 *   msg - message to be printed
 *
 * return:
 *   void
 */
void error(char *msg) 
{
  printf("\nERROR:%s\n",msg);
  exit(EXIT_FAILURE);
}
