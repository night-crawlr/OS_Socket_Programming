#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 8000
#define BUFF_SIZE 10000
#define MIN_SIZE 1024
char Download_buff[BUFF_SIZE];

void percentage(long long int sb_size, long long int sb2_size)
{
    char s1[50] = "";
    char s2[50] = "";
    s2[0] = '\r';
    float per;
    per = sb2_size * 100;
    per = per / (float)sb_size;
    sprintf(s1, "%0.2f", per);
    s2[1] = s1[0];
    s2[2] = s1[1];
    s2[3] = s1[2];
    s2[4] = s1[3];
    s2[5] = s1[4];
    s2[6] = s1[5];
    write(1, s2, 6);
}

void parse(char **argv, long long int *args, char *input)
{
    *args = 0;
    char *del = "' ','\n'";
    argv[1] = strtok(input, del);

    long long int i = 2;
    while (1)
    {
        argv[i] = strtok(NULL, del);
        if (argv[i] == NULL)
        {
            i--;
            break;
        }
        i++;
    }
    *args = i;
    return;
}

int main()
{
    int c_socketd;               // client socket descriptor
    struct sockaddr_in serv_adr; // structutre for scoket adress

    if ((c_socketd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Failed in Socket creation ... \n");
        return 0;
    }

    // setting serv_adr
    memset(&serv_adr, '0', sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1"); // converts readable 127.0.0.1 into 32 bit uint  its same as inet_pton used in tutorial codes
    serv_adr.sin_port = htons(PORT);

    //Creating command line interface

    printf("Instructions : \n 1) Get <file_1> <file_2> <file_3> ........<file_n>\n 2) Quit \n");

    //printing prompt

    if (connect(c_socketd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0)
    {
        printf("\nUnable to connect to 127.0.0.1\n");
        return 0;
    }

    while (1)
    {
        printf("\n< client > ");
        char *input;
        size_t size = MIN_SIZE;
        getline(&input, &size, stdin);
        //printf("%s", input);
        if (strcmp(input, "\n") == 0)
        {
            continue;
        }
        write(c_socketd, input, MIN_SIZE);
        //bzero(input, 1024);
        char *argv[1024];
        long long int args;
        parse(argv, &args, input);
        printf("\nRequest sent to Server\n");
        printf("\nMessage recived from server :\n");
        char from_server[MIN_SIZE] = "";
        bzero(from_server, MIN_SIZE);
        read(c_socketd, from_server, sizeof("q") - 1); // reading a validity string from server
        if (strcmp(from_server, "@") == 0)
        {
            //Invalid request
            printf("1) Invalid request \n");
            printf("2) Instructions : \n  1) Get <file_1> <file_2> <file_3> ........<file_n>\n  2) Quit \n");
            continue;
        }
        if (strcmp(from_server, "q") == 0)
        {
            //quit the client
            printf("\nExiting   .....      \n");
            break;
        }
        // NEED TO PICk FILES FROM SERVER
        for (long long int i = 2; i <= args; i++)
        {
            char from_server1[MIN_SIZE] = "";
            bzero(from_server1, MIN_SIZE);
            read(c_socketd, from_server1, sizeof("no") - 1);
            // reading whether file exist or not in server
            if (strcmp("no", from_server1) == 0) // it means no such file exists
            {
                printf("\n%lld) %s : No such file in server directory to Download\n", i, argv[i]);
                write(c_socketd, "go", sizeof("go") - 1); // sending that msg is taken
                continue;
            }
            write(c_socketd, "co", sizeof("co") - 1); // acknowlaedge similar to go
            char size_file[MIN_SIZE] = "";
            long long int file_size = 0;
            bzero(size_file, MIN_SIZE);
            read(c_socketd, size_file, MIN_SIZE - 1);
            //file_size = atoi(size_file);
            char *eptr;
            file_size = strtol(size_file, &eptr, 10);
            if (file_size == 0)
            {
                printf("\nconversion error ( Maybe file in server is empty or file size is out of range )\n");
                write(c_socketd, "n", sizeof("n") - 1);
                continue;
            }
            int fd; // file descriptor of argv[i]
            fd = open(argv[i], O_RDWR, 0644);
            if (fd > 0)
            {
                printf("\n%s already exist press 'y' to overwrite it,press 'n' to stop downloading : ", argv[i]);
                char *per;
                size_t size1 = MIN_SIZE;
                getline(&per, &size1, stdin);
                if (per[0] == 'n')
                {
                    write(c_socketd, "n", sizeof("n") - 1); // sending the premissions
                    continue;
                }
                unlink(argv[i]); //deleting files
            }
            write(c_socketd, "y", sizeof("y") - 1); // sending the premissions
            fd = open(argv[i], O_CREAT | O_RDWR, 0644);
            printf("\n%s started Downloading    ...........  \n", argv[i]);

            // Downlaoding starts fd is filedescriptor of doenlaoding file with fileise: file_size
            bzero(Download_buff, BUFF_SIZE);
            long long int mul, rem;
            mul = file_size / BUFF_SIZE;
            rem = file_size % BUFF_SIZE;
            for (long long int b = 1; b <= mul; b++)
            {
                read(c_socketd, Download_buff, BUFF_SIZE);
                write(c_socketd, "ack", sizeof("ack") - 1);
                write(fd, Download_buff, BUFF_SIZE);
                struct stat sb2;
                stat(argv[i], &sb2);
                percentage(file_size, sb2.st_size);
                bzero(Download_buff, BUFF_SIZE);
            }
            bzero(Download_buff, BUFF_SIZE);
            read(c_socketd, Download_buff, rem);
            write(c_socketd, "ack", sizeof("ack") - 1);
            write(fd, Download_buff, rem);
            struct stat sb3;
            stat(argv[i], &sb3);
            percentage(file_size, sb3.st_size);
            printf("\n%s Downlaod Completed\n", argv[i]);
        }
        sleep(1); // slleping as acknowledge ment to be recieves and server should wait for
    }
}
