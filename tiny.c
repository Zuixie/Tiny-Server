#include "csapp.h"

#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

#define DEFAULT_INDEX "index.html"
#define DYNAMIC_KEY_WORD "cgi-bin"

int doit(int connfd);
void rperror(int connfd, int errcode, char *errmsg);
int parse_url(char *url, char *filename, char *argv);
void read_request_headrs(rio_t *riop);
void get_filetype(char *filename, char *filetype);
int server_static(int connfd, char *filename, size_t filesize);
int server_dynamic(int connfd, char *filename, char *argv);
void sigchild_handler(int sig);
void sigpipe_handler(int sig);
void init_server();

int main(int argc, char **argv)
{
    char *port, clhost[MAXLINE], clport[MAXLINE];
    int listenfd, connfd;
    socklen_t addrlen;
    struct sockaddr_storage addr;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }
    port = argv[1];
    
    printf("Server start init ... ...\n");
    init_server();

    listenfd = Open_listenfd(port);
    printf("Init success!\n");
    while (1) {
        printf("Waiting for connecting\n");
        addrlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (struct sockaddr*)&addr, &addrlen);
        Getnameinfo((SA *)&addr, addrlen, clhost, MAXLINE, 
                    clport, MAXLINE, 0);
        printf("Connected to (%s, %s) %d\n", clhost, clport, (int)addrlen);
        doit(connfd);
        Close(connfd);
        printf("disconnect %s:%s\n", clhost, clport);
    }
    
    return 0;
}

int doit(int connfd)
{
    char buf[MAXLINE];
    char method[MAXLINE];
    char url[MAXLINE];
    char version[MAXLINE];
    char filename[MAXLINE];
    char argv[MAXLINE];
    int iret;
    int is_static;
    struct stat st;

    rio_t rio;

    dbg_printf("connfd :%d\n", connfd);

    // read first line in request content
    Rio_readinitb(&rio, connfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    iret = sscanf(buf, "%s %s %s", method, url, version);
    printf("%s", buf);
    if (iret < 3) 
    {
        rperror(connfd, 400, "error in http headr message");
        return -1;
    }


    dbg_printf("method:%s, uri:%s, version:%s\n", method, url, version);
    // parse uri
    is_static = parse_url(url, filename, argv);
    
    if (!strcmp(method, "POST"))
    {
        // do post
        do_post(connfd, &rio, filename);
        return 1;
    }

    read_request_headrs(&rio);

    if (strcmp(method, "GET")) 
    {
        rperror(connfd, 405, "Method Not Allowed");
        return -1;
    }

    dbg_printf("parese result: filename:%s, argv:%s\n", filename, argv);

    // check file & get file size 
    if (stat(filename, &st) < 0) 
    {  
        rperror(connfd, 404, "Not found file.");
        return -1;
    }

    if (is_static) 
    {
        // static content
        if (!S_ISREG(st.st_mode) || !(st.st_mode & S_IRUSR)) 
        {
            // permissions error
            rperror(connfd, 403, "Forbidden can't read this file.");
            return -1;
        }
        server_static(connfd, filename, st.st_size);
    } 
    else
    {
        // dynamic content
        if (!S_ISREG(st.st_mode) || !(st.st_mode & S_IXUSR)) 
        {
            // permissions error
            rperror(connfd, 403, "Forbidden can't execute this file.");
            return -1;
        }
        server_dynamic(connfd, filename, argv);
    }

    return 0;
}

void rperror(int connfd, int errcode, char *errmsg)
{
    dbg_printf("connfd:%d, code:%d, msg:%s\n", connfd, errcode, errmsg);

    char contentbuf[MAXLINE];
    char buf[MAXLINE];
    sprintf(contentbuf, "%d, %s\n", errcode, errmsg);

    sprintf(buf, "HTTP/1.1 %d %s\r\n", errcode, errmsg);
    sprintf(buf, "%sServer: Tiny Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, (int)strlen(contentbuf));
    sprintf(buf, "%sContent-type: text/html\r\n\r\n", buf);

    Rio_writen(connfd, buf, strlen(buf));
    Rio_writen(connfd, contentbuf, strlen(contentbuf));

}

void read_request_headrs(rio_t *riop) 
{
    char buf[MAXLINE];
    buf[0] = 0;
    while (strcmp(buf, "\r\n")) 
    {
        Rio_readlineb(riop, buf, MAXLINE);
        printf("%s", buf);
    }
    
}

int parse_url(char *url, char *filename, char *argv) 
{
    char *ptr;
    ptr = index(url, '?');
    
    if (!strstr(url, DYNAMIC_KEY_WORD)) 
    {
        // static
        if (ptr)
        {
            *ptr = 0;
        }
        strcpy(filename, ".");
        strcpy(argv, "");
        strcat(filename, url);

        if (strlen(url) == 1 && url[0] == '/') 
        {
            strcat(filename, DEFAULT_INDEX);
        }
        return 1;
    }
    else 
    {
        // dynamic
        if (ptr) 
        {
            *ptr = 0;
            strcpy(argv, ptr+1);
        } 
        else 
        {
            strcpy(argv, "");
        }
        
        strcpy(filename, ".");
        strcat(filename, url);
        return 0;
    }

}

void get_filetype(char *filename, char *filetype) 
{  
    if (strstr(filename, ".html")) 
    {
        strcpy(filetype, "text/html");
    }
    else if (strstr(filename, ".jpg"))
    {
        strcpy(filetype, "image/jpeg");
    }
    else if (strstr(filename, ".mp4"))
    {
        strcpy(filetype, "video/mpeg4");
    }
    else
    {
        strcpy(filetype, "text/plain");
    }
}

int server_static(int connfd, char *filename, size_t filesize)
{
    dbg_printf("static: filename: %s, filesize: %d\n", filename, (int)filesize);

    char buf[MAXLINE];
    char filetype[MAXLINE];
    char *ptr;
    int srcfd;
    int ret;
    // get file type
    get_filetype(filename, filetype);
    
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, (int)filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    
    srcfd = open(filename, O_RDONLY);

    if ((ptr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0)) 
        == NULL) 
    {
        rperror(connfd, 503, "Service Unavailable");
        return -1;
    }

    Close(srcfd);

    printf("start transferring data.\n");
    Rio_writen(connfd, buf, strlen(buf));

    ret = rio_writen(connfd, ptr, filesize);
    munmap(ptr, filesize);

    printf("static content done! %d\n", ret);

    return 1;
}

int server_dynamic(int connfd, char *filename, char* argv)
{
    dbg_printf("dynamic: filename: %s\n", filename);
    
    int ret;
    char buf[MAXLINE];
    char *emptylist[] = {0}; 
    pid_t pid;

    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Server\r\n", buf);

    rio_writen(connfd, buf, strlen(buf));
    
    // fork && execve
    
    if((pid = Fork()) == 0) 
    {
        // child
        ret = setenv("QUERY_STRING", argv, 1);
        if (ret) 
        {
            fprintf(stderr, "setenv fail ... ...\n");
        }
        Dup2(connfd, STDOUT_FILENO);
        execve(filename, emptylist, environ);
    }
    dbg_printf("pid: %d\n", (int)pid);
    // wait
    // Wait(NULL); // replace by sigchild_handler

    return 1;
}

void init_server()
{
    if(signal(SIGCHLD, sigchild_handler) == SIG_ERR)  
    {  
        fprintf(stderr, "signal error : %s\n", strerror(errno));  
        exit(-1);  
    }

    if(signal(SIGPIPE, sigpipe_handler) == SIG_ERR)  
    {  
        fprintf(stderr, "signal error : %s\n", strerror(errno));  
        exit(-1);  
    }
}

void sigchild_handler(int sig) 
{
    pid_t pid;
    dbg_printf("sigchild : %d\n", sig);
    pid = Wait(NULL);
    dbg_printf("sigchild pid: %d\n", (int)pid);
}

void sigpipe_handler(int sig)
{
    dbg_printf("sigpipe doing ... ...\n");
    return;
}


// handler post method
int do_post(int connfd, rio_t *rio, char *filename) 
{
    char buf[MAXLINE];
    char postbuf[MAXLINE];
    char length[MAXLINE];
    int len;

    buf[0] = 0;
    while (strcmp(buf, "\r\n")) 
    {
        Rio_readlineb(rio, buf, MAXLINE);
        if (strstr(buf, "Content-Length: ")) 
        {
            sscanf(buf, "Content-Length: %s", length);
        }
        printf("%s", buf);
    }

    setenv("CONTENT_LENGTH", length, 1);
    dbg_printf("get length: %s\n", length);
    len = atoi(length);
    // TODO check permission

    rio_readnb(rio, postbuf, len);
    printf("father : read post %s\n", postbuf);

    int ret;
    char *emptylist[] = {0}; 
    int pipefd[2];

    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Server\r\n", buf);

    rio_writen(connfd, buf, strlen(buf));


    if (pipe(pipefd) < 0) 
    {
        fprintf(stderr, "pipe error!!!\n");
    }
    dbg_printf("pipe: %d, %d\n", pipefd[0], pipefd[1]);

    if (Fork() == 0)
    {
        //child
        Close(pipefd[1]);

        Dup2(pipefd[0], STDIN_FILENO);
        Dup2(connfd, STDOUT_FILENO);
        fprintf(stderr, "child start execve %s \n", filename);
        ret = execve(filename, emptylist, environ);
        fprintf(stderr, "execve ret:%d \n", ret);
        exit(0);
    }
    
    Close(pipefd[0]);
    printf("write ... ... \n");
    write(pipefd[1], postbuf, MAXLINE);
    Close(pipefd[1]);
    printf("write ... ... done\n");

    return 1;
}
