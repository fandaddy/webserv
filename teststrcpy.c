/*************************************************************************
	> File Name: teststrcpy.c
	> Author: 
	> Mail: 
	> Created Time: Tue 19 Jul 2016 09:17:16 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>

int main()
{
    char *str = NULL;
    char *dest = NULL;
    char *src = NULL;

    str = (char *)malloc(sizeof(char) * 10);
    strcpy(str, "hello");
    printf("First: %s\n", str);
    src = dest = str;

    while(*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';

    strcpy(str, str+1);
    printf("Second: %s\n", str);
    
}
