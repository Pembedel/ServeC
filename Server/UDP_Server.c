/* UDP_Server.c
 * 
 * Implementation of Server Client UDP communication
 * - Server receives key and value from Client
 *   Stores it in temporary buffer
 *   --set <key> <value>
 * - Client retrieves the value when it sends the key
 *   --get <key>
 * - Client Deletes entry based on key supplied
 *   --del <key>
 *
 * Author: Kapil
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

#define MAXINPUT 1024 
#define ONEMILLION 1000000

/* splitting the string based on " " token*/
#define STRING_SPLIT(buffer,key, split_str) split_str = strtok(&buffer[6]," ");\
                                            strncpy(key,split_str,strlen(split_str));\
                                            key[strlen(split_str)]='\0';

/* return status codes*/
#define FAILURE -1
#define ENTRY_EXIST 1
#define SUCCESS 0

#define IPADDR 1
#define PORTNUM 2

#define MIN_PORTNO 1
#define MAX_PORTNO 65535

/* linked list for storing key-value pair*/
struct node{
    char *key;
    char *value;
    struct node *next;
};

/* Function prototypes */
int find_entry(struct node **head, char *, int length, char *value);
int add_entry(struct node** head, char* key, int key_len, char* value, int value_len); 
int del_entry(struct node **head, char *key, int length);
void del_all_entry(struct node **head);
void error(char *msg);
int validate_ip_addr(char *ip_addr);

/** Functions **/

/* main function*/
int main(int argc, char **argv) 
{ 
    int sockfd; 
    int portno;
    int num_bytes;
    int key_len;
    int value_len;
    int status = FAILURE;
    int entry_count = 0;
    int count = 1;
    char buffer[MAXINPUT]; 
    char *ip_addr;
    char *port_num;
    char key[257];
    char value[257];
    char *split_str;
    char msg[]="SUCCESS";
    struct node* head = NULL;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr; 
    unsigned int len;

    /*validate input parameters*/
    if (argc != 2) 
    {
        printf("usage: %s <ipaddress>:<port>\n", argv[0]);
        error("Incorrect Input");
    }

    /*Separating ipaddr and portno from <ipaddr>:<portno> format*/    
    split_str = strtok(argv[1],":");

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

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        error("Opening socket");
    }

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    memset(&cli_addr, 0, sizeof(cli_addr)); 

    /* server IP config */
    serv_addr.sin_family = AF_INET; /* IPv4 */ 
    serv_addr.sin_addr.s_addr = inet_addr(ip_addr); 
    serv_addr.sin_port = htons(portno); 
    
    /* Bind the socket with the server address*/ 
    if ( bind(sockfd, (const struct sockaddr *)&serv_addr, 
                       sizeof(serv_addr)) < 0 ) 
    { 
        error("Bind failed"); 
    } 
    
    len = sizeof(cli_addr);

    /* Server receives commands set,get,del from Client */
    do
    {
        num_bytes = recvfrom(sockfd, (char *)buffer, MAXINPUT, 
                    MSG_WAITALL, ( struct sockaddr *) &cli_addr, 
                    &len); 
        buffer[num_bytes] = '\0';

        printf("\nClient UDP message received:%s\n", buffer); 
        
        /* Processing --set command from Client*/
        if (strncmp(buffer,"--set",5)==0)
        {
            if (entry_count < ONEMILLION)
            {
                STRING_SPLIT(buffer,key, split_str);
                
                while (split_str != NULL)
                {
                    strncpy(value,split_str,strlen(split_str));
                    value[strlen(split_str)]='\0';
                    split_str = strtok (NULL, " ");
                }
                key_len = strlen(key);
                value_len = strlen(value);
                
                /* Check if key exists in db before adding it*/
                if (SUCCESS == find_entry(&head, key, key_len, value))
                    status = ENTRY_EXIST;
                else
                    status = add_entry(&head, key, key_len, value, value_len);
                
                /* Adding appropriate status message for Client*/
                if (status == FAILURE)
                {
                    printf("\nCommand --set FAILED");
                    strcpy(msg,"FAIL");
                }
                else if (status == ENTRY_EXIST)
                {
                    printf("\nEntry in Server exists, --set Ignored");    
                    strcpy(msg,"EXISTS");
                }
                else
                {
                    strcpy(msg,"SUCCESS");
                    entry_count++;
                }
            }
            else
            {
                strcpy(msg,"MAXLMT");
                printf("\nMax limit reached for --set");
            }

            sendto(sockfd, (const char *)msg, strlen(msg), 
                    MSG_CONFIRM, (const struct sockaddr *) &cli_addr, len);                
        }
        /* Processing --get command from Client*/
        else if (strncmp(buffer,"--get",5)==0)
        {
            STRING_SPLIT(buffer,key, split_str);
            
            key_len = strlen(key);
            status = find_entry(&head, key, key_len, value);    
            if (status == FAILURE)
            {
                strcpy(value,"Key not found : ");
                strcat(value, key);
            }
            sendto(sockfd, (const char *)value, strlen(value), 
                    MSG_CONFIRM, (const struct sockaddr *) &cli_addr, len);            
        }
        /* Processing --del command from Client*/
        else if (strncmp(buffer,"--del",5)==0)
        {
            STRING_SPLIT(buffer,key, split_str);
            
            key_len = strlen(key);
            status = del_entry(&head, key, key_len);        
            if (status == FAILURE)
            {
                strcpy(msg,"NOEXIST");
                printf("\nEntry does not exists");
            }
            else
            {
                strcpy(msg,"SUCCESS");
                entry_count--;
            }
            sendto(sockfd, (const char *)msg, strlen(msg), 
                    MSG_CONFIRM, (const struct sockaddr *) &cli_addr, len);                
        }
        else if (strncmp(buffer,"--fin",5)==0)
        {
            strcpy(msg,"FIN");
            sendto(sockfd, (const char *)msg, strlen(msg), 
                    MSG_CONFIRM, (const struct sockaddr *) &cli_addr, len);                

            /* Code execution should not reach here because validation 
                is in place for cmd types*/
            printf("FIN received");
        }

    }while(strncmp(buffer,"--fin",5)!=0);
    
    del_all_entry(&head);
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

/* Function: find_entry() - To find the if a key-value pair exists 
 * in parameters: 
 *   head - head of the linked list
 *   key - key value to be found in db
 *   length - length of the key
 *   value - value of the key
 *
 * return:
 *   status - status of the operation
 */
int find_entry(struct node **head, char *key, int length, char *value)
{
  struct node *temp = *head;
  int status = FAILURE;
  
  while(temp != NULL)
  {
    /* Check string length also to avoid sub-string match*/
    if(strncmp(temp->key, key, length) == 0 && strlen(temp->key) == length)
    {
       printf("\nEntry in db exists for the queried key, Found value:%s",temp->value);
       strcpy(value, temp->value);
       status = SUCCESS;
       break;
    }
    temp = temp->next;
  }

  return status;
}

/* 
 * Function: add_entry() - To add a key-value pair exists 
 * in parameters: 
 *   head - head of the linked list
 *   key - key value to be found in db
 *   key_len - length of the key
 *   value - value of the key to be added
 *   value_len - length of value
 *
 * return:
 *   status - status of the operation
 */

int add_entry(struct node** head, char* key, int key_len, char* value, int value_len) 
{ 
    struct node* new_node = (struct node*) malloc(sizeof(struct node)); 
  
    if (NULL == new_node)
        return FAILURE;

    new_node = (struct node*) malloc(sizeof(struct node));
    new_node->key = malloc((key_len+1)*sizeof(char));
    new_node->value = malloc((value_len+1)*sizeof(char));
    
    strncpy(new_node->key, key, key_len);
    key[key_len] = '\0';
    strncpy(new_node->value, value, value_len);
    value[value_len] ='\0';

  
    printf("\nset value success: added new key:%s and new value:%s\n", new_node->key,new_node->value);

    if (*head == NULL) 
    { 
       *head = new_node; 
        new_node->next = NULL; 

       return SUCCESS; 
    }   
    /* add new node at head*/   
    new_node->next = *head; 
    *head = new_node; 

    return SUCCESS;
} 

/* 
 * Function: del_entry() - To find the if a key-value pair exists 
 * in parameters: 
 *   head - head of the linked list
 *   key - key value to be found in db
 *   length - length of the key
 *
 * return:
 *   status - status of the operation
 */

int del_entry(struct node **head, char *key, int length)
{
  struct node *temp = *head;
  struct node *prev = NULL;

  while(temp != NULL)
  {
    /* Check string length also to avoid sub-string match*/
    if(strncmp(temp->key, key, length) == 0 && strlen(temp->key) == length)
    {
        if (prev == NULL && temp->next == NULL)
        {
            /* Only one node case*/
            head = NULL;
            printf("\nDelete operation success, key removed:%s",temp->key);
            free(temp->key);
            free(temp->value);
            free(temp);
            return SUCCESS;
        }
        else if(prev == NULL && temp->next != NULL)
        { 
            /*If the node is the head*/
            *head = temp->next;
            printf("\nDelete operation success, key removed:%s",temp->key);
            free(temp->key);
            free(temp->value);
            free(temp);
            return SUCCESS;
        }
        else
        {
            prev->next = temp->next;
            printf("\nDelete operation success, key removed:%s",temp->key);
            free(temp->key);
            free(temp->value);
            free(temp);
            return SUCCESS;
        }
    }
    prev = temp;
    temp = temp->next;
  }
  return FAILURE;
}

/* 
 * Function: del_all_entry() - Free all allocated mem to avoid memleak 
 * in parameters: 
 *   head - head of the linked list
 *
 * return:
 *   void
 */
void del_all_entry(struct node **head)
{
  struct node *temp = *head;

  while(temp != NULL)
  {
    if (temp->next == NULL)
    {
        /* Only one node case*/
        printf("\nDelete operation success, key freed:%s",temp->key);
        free(temp->key);
        free(temp->value);
        free(temp);
        break;
    }
    else
    {
        *head = temp->next;
        printf("\nDelete operation success, key freed:%s",temp->key);
        free(temp->key);
        free(temp->value);
        free(temp);
    }
    temp = temp->next;
  }
}
