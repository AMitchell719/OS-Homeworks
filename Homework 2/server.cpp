#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <netdb.h>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, clilen, portno;
    int buffer;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    int request;
    int request_array[10000];
    int counter = 0;
    int request_count = 0;
    bool flag = true;

    portno = atoi(argv[1]);

    while(cin >> request)
    {
        request_array[counter] = request;
        counter++;
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        cout << "Error creating socket. Terminating" << endl;
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Error on binding socket. Terminating" << endl;
        exit(1);
    }

    while(flag)
    {
        listen(sockfd,5);
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);

        if (newsockfd < 0)
        {
            cout << "Error accepting request. Terminating" << endl;
            exit(1);
        }

        n = read(newsockfd,&buffer,sizeof(int));
        cout << "Client is sending a request" << endl;

        if (n < 0)
        {
            cout << "Error reading from socket. Terminating" << endl;
            exit(1);
        }

        buffer = request_array[request_count];
        request_count++; //increment to give next value back to client
        
        n = write(newsockfd,&buffer,sizeof(int));

        if (n < 0)
        {
            cout << "Error writing to socket. Terminating" << endl;
            exit(1);
        }

        if (n >= counter)
        {
            close(newsockfd);
        }

        if (request_count == counter)
        {
            flag = false;
        }
    } // end while loop

    cout << "Closing server connection..." << endl;
    return 0;
}
