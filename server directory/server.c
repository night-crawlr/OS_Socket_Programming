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

#define BUFF_SIZE 10000
#define MIN_SIZE 1024
#define PORT 8000
#define MAX_QUEUE 8
char Download_buff[BUFF_SIZE];
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
void send_file(char *filename, int dup_serverfd)
{
    int fd = open(filename, O_RDONLY);
    char ackno[MIN_SIZE] = "";
    bzero(ackno, MIN_SIZE);
    if (fd == -1)
    {
        write(dup_serverfd, "no", sizeof("no") - 1); //sending error message to client on openng the file
        read(dup_serverfd, ackno, sizeof("go") - 1);
        return;
    }
    write(dup_serverfd, "ok", sizeof("ok") - 1); // sending the message the file that has benn asked is existing
    read(dup_serverfd, ackno, sizeof("co") - 1);
    //Finding size of input file
    struct stat sb;
    stat(filename, &sb);
    long long int sb_size = sb.st_size; // fd has file descripter of open file sb_szie has size of it
    //in sb
    char size_file[MIN_SIZE] = "";
    bzero(size_file, MIN_SIZE);
    sprintf(size_file, "%ld", sb.st_size);
    printf("\nSize sending : %s\n", size_file);
    write(dup_serverfd, size_file, MIN_SIZE - 1); // sent file_size
    char per[MIN_SIZE] = "";
    bzero(per, MIN_SIZE);
    read(dup_serverfd, per, sizeof("n") - 1); //reading permission if y continue doouoading if n then stop  downloading
    if (per[0] == 'n')
    {
        printf("\nDownloading stopped\n");
        return;
    }

    //Downlaoding starts

    bzero(Download_buff, BUFF_SIZE);
    long long int mul, rem;
    mul = sb_size / BUFF_SIZE;
    rem = sb_size % BUFF_SIZE;
    for (long long int b = 1; b <= mul; b++)
    {
        read(fd, Download_buff, BUFF_SIZE);            //reading BUUF_SIZE bits and keepinf in buff later transfering buff to socket
        write(dup_serverfd, Download_buff, BUFF_SIZE); // writing Download_buff to socket buff
        char ack[MIN_SIZE] = "";
        bzero(ack, MIN_SIZE);
        read(dup_serverfd, ack, sizeof("ack") - 1); // acknowledgement that client has read the package sent;
        bzero(Download_buff, BUFF_SIZE);
    }
    // transfering BUFF_size*mul size completed now transfering reamainig data to client
    bzero(Download_buff, BUFF_SIZE);
    read(fd, Download_buff, rem);
    write(dup_serverfd, Download_buff, rem); // completed sending file
    char ack[MIN_SIZE] = "";
    bzero(ack, MIN_SIZE);
    read(dup_serverfd, ack, sizeof("ack") - 1); // acknowledgement that client has read the package sent;

    printf("\n%s Download Completed\n", filename);
}
void Packages_need_to_send(char **argv, long long int args, int dup_serverfd)
{
    for (long long int i = 2; i <= args; i++)
    {
        char *file_name;
        file_name = argv[i];
        send_file(file_name, dup_serverfd);
    }
}
int main()
{
    int serverfd, dup_serverfd;
    socklen_t clilen;
    int opt = 1;
    struct sockaddr_in cliadrr;

    //creatiing server socket
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        return 0;
    }
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // SOL_SOCKET is the socket layer itself
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    //server adress settings
    cliadrr.sin_family = AF_INET;
    cliadrr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cliadrr.sin_port = htons(PORT);

    //binding server
    if ((bind(serverfd, (struct sockaddr *)&cliadrr, sizeof(cliadrr))) < 0)
    {
        perror("bind");
        return 0;
    }
    //listen

    if ((listen(serverfd, MAX_QUEUE)) < 0)
    {
        perror("listen");
        return 0;
    }

    clilen = sizeof(cliadrr);
    if ((dup_serverfd = accept(serverfd, (struct sockaddr *)&cliadrr, (socklen_t *)&clilen)) < 0)
    {
        perror("accept");
        return 0;
    }
    printf("\nClient connected\n");
    while (1)
    {
        char buf[1024] = "";
        char *argv[1024];
        //memset(argv, "\0", 1024);
        long long int args;
        bzero(buf, MIN_SIZE);
        read(dup_serverfd, buf, MIN_SIZE);
        parse(argv, &args, buf); // parse acording to ' ' and '\n'
        if (strcmp("Quit", argv[args]) == 0)
        {
            write(dup_serverfd, "q", sizeof("q") - 1); //clarity char
            break;                                     // breaking the connection
        }
        if ((strcmp("Get", argv[1]) != 0) || ((strcmp("Get", argv[1]) == 0) && (args == 1)))
        {
            write(dup_serverfd, "@", sizeof("@") - 1); // clarity char
            continue;
        }
        write(dup_serverfd, "!", sizeof("!") - 1); //clarity char
        //NEED TO SEND FILES
        Packages_need_to_send(argv, args, dup_serverfd);
    }
}