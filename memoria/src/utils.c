#include<utils.h>

char* mount_instructions_directory(char* text_path){
    char* path = strdup(cfg_mem->path_instrucciones);
    string_append(&path,text_path);
    return path;
}