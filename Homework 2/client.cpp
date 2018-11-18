#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int information_array[10000];
    int file_size = 0; // keeps track of how many times we request
    int num_rings;
    int token;
    int buffer;
    int pid;

    token = 0;
    buffer = 0;

    // port number generally 20000
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0){
        cout << "Error opening socket \n";
        exit(1);
    }

    // what to put as hostname??
    server = gethostbyname(argv[1]);

    if (server == NULL)
    {
        cout << "Error, no such host\n";
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        cout << "Error connecting\n";
        exit(0);
    }

    num_rings = 0;
    n = write(sockfd,&num_rings,sizeof(int));
    n = read(sockfd,&num_rings,sizeof(int));
    cout << "Number of Children in Ring = " << num_rings << endl << endl;
    close(sockfd);

    // initialize pipes
    int pipefd[num_rings + 1][2];

    // spawn pipes
    for(int i = 0; i <= num_rings; i++)
    {
        if (pipe(pipefd[i]) < 0)
        {
            printf("Pipe Creation failed\n");
            exit(1);
        }
        else
        {
            cout << "Pipe created successfully" << endl;
        }
    }

    cout << endl;
    
    for(int i = 0; i < num_rings -1; i++)
    {
        if ((pid=fork())==0)
        {
            while(1)
            {

                read(pipefd[i][0], &token, sizeof(int));
                if(token == -1)
                {
                    cout << "Child process ending" << endl;
                    token = -1;
                    write(pipefd[i+1][1], &token, sizeof(int));
                    _exit(0);
                }
                portno = atoi(argv[2]);
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
                serv_addr.sin_port = htons(portno);

                if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
                    cout << "Error connecting\n";
                    cout << "Process number with error " << i << endl;
                    exit(0);
                }

                n = write(sockfd,&buffer,sizeof(int));
                n = read(sockfd,&buffer,sizeof(int));
                close(sockfd);

                if (token >= 0 && buffer == 1)
                {
                    cout << "Process " << i+1 << " is using the network" << endl;
                }

                if(buffer == -1)
                {
                    cout << "Child process ending" << endl;
                    token = -1;
                    write(pipefd[i+1][1], &token, sizeof(int));
                    _exit(0);
                }

                token = token + 1;

                write(pipefd[i+1][1], &token, sizeof(int));
            }
        }
        close(pipefd[i][0]);
        close(pipefd[i+1][1]);
    }

    token = 0;
    while(1)
    {
        portno = atoi(argv[2]);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        {
            cout << "Error connecting\n";
            cout << 0 << endl;
            exit(0);
        }

        n = write(sockfd,&buffer,sizeof(int));
        n = read(sockfd,&buffer,sizeof(int));
        close(sockfd);

        if (buffer == 1)
            cout << "Process 0 is using the network" << endl;

        else if (buffer == -1)
        {
            token = -1;
            cout << "Ending" << endl;
            write(pipefd[0][1], &token, sizeof(int));
            break;
        }

        token = token + 1;
        write(pipefd[0][1], &token, sizeof(int));
        read(pipefd[num_rings-1][0], &token, sizeof(int));
        if(token == -1)
        {
            token = -1;
            cout << "Parent ending" << endl;
            write(pipefd[0][1], &token, sizeof(int));
            break;
        }
    }

    for(int j = 0; j < num_rings; j++)
    {
        wait(NULL);
    }

    cout << "All children have finished terminating" << endl;

    return 0;
}
