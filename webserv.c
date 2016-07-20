/*************************************************************************
	> File Name: webserv.c
	> Author: Jiang Yuhang
	> Mail: jiangxiaobai1989@gmail.com
	> Created Time: Tue 19 Jul 2016 10:19:08 AM CST
 ************************************************************************/

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>i
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define PORTNUM 8080

time_t server_starter;
int server_bytes_sent;
int server_requests;

pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int ac, char *av)
{
    int sd, fd;
    int *fdptr;
    pthread_t worker;
    pthread_attr_t attr_detached;
    void *handle_call(void *);

    sd = make_server_socket(PORTNUM);
    if(sd == -1)
    {
        perror("make socket");
        exit(2);
    }

    setup(&attr_detached);

    while(1)
    {
        fd = accept(sd, NULL, NULL);
        stats_add(0, 1);

        fdptr = malloc(sizeof(int));
        *fdptr = fd;
        pthread_create(&worker, &attr_detached, handle_call, fdptr);
    }

}

stats_get(time_t *started, int *bytesp, int *hitsp)
{
    pthread_mutex_lock(&stats_lock);
    *started = server_starter;
    *bytesp = server_bytes_sent;
    *hitsp = server_requests;
    pthread_mutex_unlock(&stats_lock);
}

stats_add(int bytesamt, int hitsamt)
{
    pthread_mutex_lock(&stats_lock);
    server_bytes_sent += bytesamt;
    server_requests += hitsamt;
    pthread_mutex_unlock(&stats_lock);
}

setup(pthread_attr_t *attr_detached)
{
    pthread_attr_init(attr_detached);
    pthread_attr_setdetachstate(attr_detached, PTHREAD_CREATE_DETACHED);

    time(&server_starter);
    server_requests = 0;
    server_bytes_sent = 0;
}

void *handle_call(void *fdptr)
{
    FILE *fpin;
    char request[BUFSIZ];
    int fd;

    fd = *(int *)fdptr;
    free(fdptr);
    fpin = fdopen(fd, "r");
    fgets(request, BUFSIZ, fpin);
    printf("got a call on %d: request = %s", fd, request);

    skip_rest_of_header(fpin);

    process_rq(request, fd);

    fclose(fpin);
}

skip_rest_of_header(FILE *fp)
{
    char buf[BUFSIZ];

    while( (fgets(buf, BUFSIZ, fp) != NULL) && strcmp(buf, "\r\n") != 0 );
}

process_rq(char *rq, int fd)
{
    char cmd[BUFSIZ], arg[BUFSIZ], version[BUFSIZ];

    if(sscanf(rq, "%s%s%s", cmd, arg, version) != 3)
    {
        return;
    }
    sanitize(arg);
    printf("sanitized version is %s\n", version);
    printf("arg : %s\n", arg);


    if( strcmp(cmd, "GET") != 0 )
    {
        not_implenmented(fd);
    }
    else if(build_in(arg, fd))
        ;
    else if(not_exist(arg))
    {
        do_404(arg, fd);
    }
    else if(isadir(arg))
    {
        do_ls(arg, fd);
    }
    else
    {
        do_cat(arg, fd);
    }
}

sanitize(char *str)
{
    char *src, *dest;
    char *str_s, *str_t;

    src = dest = str;
    str_s = str_t = str;
    
    while(*src)
    {
        if(strncmp(src, "/../", 4) == 0)
        {
            src += 3;
        }
        else if(strncmp(src, "//", 2) == 0)
        {
            src++;
        }
        else
        {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
    if(*str == '/')
    {
        //strcpy(str, str+1);
        str_t++;
        while(*str_s)
        {
            *str_s++ = *(str_t++);
        }
    }
    if(str[0] == '\0' || strcmp(str, "./") == 0 || strcmp(str, "./..") == 0)
    {
        strcpy(str, ".");
    }
}

http_reply(int fd, FILE **fpp, int code, char *msg, char *type, char *content)
{
    FILE *fp = fdopen(fd, "w");
    int bytes = 0;

    if(fp != NULL)
    {
        bytes = fprintf(fp, "HTTP/1.0 %d %s\r\n", code, msg);
        bytes += fprintf(fp, "Content-type: %s\r\n\r\n", type);
        if( content )
        {
            bytes += fprintf(fp, "%s\r\n", content);
        }
    }
    fflush(fp);
    if(fpp)
    {
        *fpp = fp;
    }
    else
    {
        fclose(fp);
    }
    return bytes;
}

not_implenmented(int fd)
{
    http_reply(fd, NULL, 501, "Not Implenmented", "text/plain", "That command is not implenmented");
}

build_in(char *arg, int fd)
{
    FILE *fp;
    time_t start_time;
    int requests;
    int volume;

    if(strcmp(arg, "status") != 0)
    {
        return 0;
    }
    http_reply(fd, &fp, 200, "OK", "text/plain", NULL);

    stats_get(&start_time, &volume, &requests);
    fprintf(fp, "Server started: %s", ctime(&start_time));
    fprintf(fp, "Total requests: %d\n", requests);
    fprintf(fp, "Bytes sent out: %d\n", volume);
    fclose(fp);
    return 1;
}

not_exist(char *f)
{
    struct stat info;
    return ( stat(f, &info) == -1 );
}

do_404(char *item, int fd)
{
    http_reply(fd, NULL, 404, "Not Found", "text/plain",
              "The item you seek is not here");
}

isadir(char *f)
{
    struct stat info;
    return ( stat(f, &info) != -1 && S_ISDIR(info.st_mode));
}

do_ls(char *dir, int fd)
{
    DIR *dirptr;
    struct dirent *direntp;
    FILE *fp;
    int bytes = 0;

    bytes = http_reply(fd, &fp, 200, "OK", "text/plain", NULL);
    bytes += fprintf(fp, "Listing of Directory %s\n", dir);

    if((dirptr = opendir(dir)) != NULL)
    {
        while(direntp = readdir(dirptr))
        {
            bytes += fprintf(fp, "%s\n", direntp->d_name);
        }
        close(dirptr);
    }
    fclose(fp);
    server_bytes_sent += bytes;
}

char *file_type(char *f)
{
    char *cp;
    if((cp = strrchr(f, '.')) != NULL)
    {
        return cp+1;
    }
    return "";
}

do_cat(char *f, int fd)
{
    char *extension = file_type(f);
    char *type = "text/plain";
    FILE *fpsock, *fpfile;
    int c;
    int bytes = 0;

    if(strcmp(extension, "html") == 0)
    {
        type = "text/html";
    }
    else if (strcmp(extension, "gif") == 0)
    {
        type = "image/gif";
    }
    else if (strcmp(extension, "jpg") == 0)
    {
        type = "image/jpg";
    }
    else if (strcmp(extension, "jpeg") == 0)
    {
        type = "image/jpeg";
    }
    fpsock = fdopen(fd, "w");
    fpfile = fopen(f, "r");
    if(fpsock != NULL && fpfile != NULL)
    {
        bytes = http_reply(fd, &fpsock, 200, "OK", type, NULL);
        while((c = getc(fpfile)) != EOF)
        {
            putc(c, fpsock);
            bytes++;
        }
        fclose(fpfile);
        fclose(fpsock);
    }
    server_bytes_sent += bytes;
}
