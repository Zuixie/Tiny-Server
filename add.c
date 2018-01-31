#include "csapp.h"

int add(int a, int b) 
{
    sleep(5);
    return a + b;
}

int main(int argc, char **argv)
{
    char buf[MAXLINE];
    char *query_str, *ptr;
    int numa, numb;

    query_str = getenv("QUERY_STRING");
    
    sprintf(buf, "argv:%s\n", query_str);
    if (query_str && (ptr = index(query_str, '&'))) 
    {
        *ptr = 0;
        numa = atoi(query_str);
        numb = atoi(ptr+1);
    }
    
    sprintf(buf, "%sa:%d, b:%d, sum:%d \n", buf, numa, numb, add(numa, numb));

    printf("Content-lenght: %d\r\n", (int)strlen(buf));
    printf("Content-type: text/plain\r\n\r\n");
    
    printf("%s", buf);

    return 0;
}
