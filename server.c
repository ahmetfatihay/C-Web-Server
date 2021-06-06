#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 18001
#define MAXLINE 4096

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void* handle_connection(void* p_client_socket);
char* http_baslik_istegi(const char * istek);

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Cevap vereceğimiz adresler
    servaddr.sin_port = htons(PORT);

    if((bind(listenfd, (SA *) &servaddr, sizeof(servaddr))) < 0)
    {
        printf("Bind error");
        exit(0);
    }

    if((listen(listenfd, 100)) < 0)
    {
        printf("Listen error");
        exit(0);
    }

    while(1)
    {
        struct sockaddr_in addr;
        socklen_t addr_len;
        char client_address[MAXLINE+1];

        printf("Waiting for a connection on port %d\n", PORT);
        fflush(stdout);

        if((connfd = accept(listenfd, (SA *) &addr, &addr_len)) < 0)
        {
            printf("Bind error");
            exit(0);
        }

        inet_ntop(AF_INET, &addr, client_address, MAXLINE);
        printf("Client connection: %s\n", client_address);

        pthread_t t;
        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_create(&t, NULL, handle_connection, pclient);
    }
}

void* handle_connection(void* p_client_socket)
{
    int client_socket = *(int*)p_client_socket;
    free(p_client_socket);
    int n;
    uint8_t recvline[MAXLINE+1];
    uint8_t buff[MAXLINE+1];

    memset(recvline, 0, MAXLINE);

    while((n = recv(client_socket, recvline, MAXLINE-1, 0)) > 0)
    {
        if(n < 0)
        {
            printf("Read error");
            exit(0);
        }

        // Get request and copy
        char recvline_copy[n];
        strcpy(recvline_copy, recvline);
        printf("\nReceived:\n%s\n", recvline_copy);

        // Split request into headers and store it in [headers] array
        char* headers[30];
        int cur = 0;
        headers[cur++] = strtok(recvline_copy, "\r\n");
        while(headers[cur-1] != NULL)
        {
            headers[cur++] = strtok(NULL,"\r\n");
        }

        printf("[GET]:[%s]\n", http_baslik_istegi(headers[0]));

        // Loop the headers
        for(int i=0; headers[i] != NULL; i++)
        {
            printf("[%d] %s\n", i, headers[i]);
        }
        
        // Check if message ends and break
        if(n<4 || (recvline[n-1] == '\n' && recvline[n-2] == '\r' && recvline[n-3] == '\n' && recvline[n-4] == '\r'))
        {
            break;
        }

        memset(recvline, 0, MAXLINE);
    }    

    snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello");

    write(client_socket, (char*)buff, strlen((char*)buff));
    
    close(client_socket);

    return NULL;
}

char* http_baslik_istegi(const char* istek)
{
    char istek_copy[strlen(istek)];
    strcpy(istek_copy, istek);
    char* request_return;
    request_return = strtok(istek_copy," ");
    request_return = strtok(NULL," ");
    return request_return;
}