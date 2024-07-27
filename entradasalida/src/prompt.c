#include <prompt.h>

char *prompt(int size)
{
    // int c;
    // while((c=getchar()!='\n') && c !=EOF);
    char *line;
    do
    {
        line = readline("press Enter to send a message: ");
        if (line && *line && strlen(line)>=size)
            break;
        free(line);
        printf("debes ingresar %d caracteres por favor!\n",size);
    } while (1);
    char* result = calloc(size+1,sizeof(char));
    strncpy(result,line,size);
    free(line);
    return result;
}