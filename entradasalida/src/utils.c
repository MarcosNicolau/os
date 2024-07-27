#include <utils.h>

char* mount_pathbase(char* name){
  char * result = string_duplicate(cfg_io->path_base_dialfs);
  string_append_with_format(&result,"/%s",name);
  return result;
}

char* mount_pathbase_files(char* name){
  char * result = string_duplicate(cfg_io->path_base_dialfs);
  string_append_with_format(&result,"/files/%s",name);
  return result;
}