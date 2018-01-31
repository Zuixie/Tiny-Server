#include "csapp.h"

int main(int argc, char **argv)
{
    char buf[MAXLINE];
    char postdata[MAXLINE];
    char *len_str, *ptr;
    int content_length;
    int n;

    len_str = getenv("CONTENT_LENGTH");
    
    sprintf(buf, "content-length: %s\n", len_str);
    if (len_str) 
    {
        content_length = atoi(len_str);
    }
    fprintf(stderr, "postrea start \n");    
    // TODO fix to malloc
    n = read(STDIN_FILENO, postdata, content_length + 1);
    // postdata[n] = 0;
    sprintf(buf, "%spost data:%s\n", buf, postdata);

    printf("Content-lenght: %d\r\n", (int)strlen(buf));
    printf("Content-type: text/plain\r\n\r\n");
    
    printf("%s", buf);

    return 0;
}
