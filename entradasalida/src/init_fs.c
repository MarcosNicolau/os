#include <init_fs.h>

void load_fs(t_log *logger) {
    struct stat st = {0};

    char *path = mount_pathbase("");
    char *path_files = mount_pathbase_files("");

    if (stat(path, &st) == -1) {
        mkdir(path, 0777);
    }
    
    if (stat(path_files, &st) == -1) {
        mkdir(path_files, 0777);
    }


    load_blocks(logger);
    load_bitmap(logger);
    init_fcbs();

    free(path);
    free(path_files);
}


void close_fs(void){
    close_blocks();
    close_bitmap();
}