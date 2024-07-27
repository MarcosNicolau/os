#include <fcb.h>

static t_dictionary *metadata_dictionary;

void init_fcbs(void)
{
    metadata_dictionary = dictionary_create();
    // cargar todos los ya existentes
    DIR *d;
    struct dirent *dir;
    char *files_path = mount_pathbase("/files");
    d = opendir(files_path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            //revisar?
            if (dir->d_type == DT_REG)
            {
                char *final_name = mount_pathbase_files(dir->d_name);
                dictionary_put(metadata_dictionary, dir->d_name, config_create(final_name));
                free(final_name);
            }
        }
        closedir(d);
    }
    free(files_path);
}

bool file_already_exists(char *file_name)
{
    return dictionary_get(metadata_dictionary, file_name) != NULL;
}

void create_file(char *file_name)
{
    char *final_name = mount_pathbase_files(file_name);
    FILE *f = fopen(final_name, "wb+");
    fclose(f);
    t_config *config = config_create(final_name);
    // econtrar el bloque libre
    uint32_t first_block = find_first_free_block();

    char *first_block_str = string_itoa(first_block);

    config_set_value(config, METADATA_BLOQUE_INICIAL, first_block_str);
    config_set_value(config, METADATA_TAMANIO_ARCHIVO, "0");
    config_save(config);

    dictionary_put(metadata_dictionary, file_name, config);

    occupy_free_blocks(first_block, 1);

    free(final_name);
    free(first_block_str);
}

t_fcb *get_metadata(char *file_name)
{
    t_fcb *fcb = malloc(sizeof(t_fcb));
    t_config *config = dictionary_get(metadata_dictionary, file_name);
    fcb->first_block = config_get_int_value(config, METADATA_BLOQUE_INICIAL);
    fcb->size = config_get_int_value(config, METADATA_TAMANIO_ARCHIVO);
    return fcb;
}

void delete_file(char *file_name)
{
    t_fcb *fcb = get_metadata(file_name);
    int blocks_occupied = ceil((double)fcb->size / cfg_io->block_size);
    int blocks_to_free = blocks_occupied ? blocks_occupied : 1;
    free_blocks(fcb->first_block, blocks_to_free);
    dictionary_remove_and_destroy(metadata_dictionary, file_name, (void *)config_destroy);
    char *final_name = mount_pathbase_files(file_name);
    remove(final_name);
    free(final_name);
    free(fcb);
}

void fcb_set_size(char *file_name, uint32_t size)
{
    t_config *config = dictionary_get(metadata_dictionary, file_name);
    char *result = string_itoa(size);
    config_set_value(config, METADATA_TAMANIO_ARCHIVO, result);
    config_save(config);
    free(result);
}

void fcb_set_first_block(char *file_name, int block)
{
    t_config *config = dictionary_get(metadata_dictionary, file_name);
    char *result = string_itoa(block);
    config_set_value(config, METADATA_BLOQUE_INICIAL, result);
    config_save(config);
    free(result);
}

void compact(char *file_name, int target_blocks)
{
    uint8_t *blocks_cpy = calloc(cfg_io->block_count * cfg_io->block_size, 1);
    //uint32_t bytes_offset = 0;
    uint32_t blocks_offset = 0;
    void iterator(char *name, void *elem)
    {
        t_config *config = (t_config *)elem;
        int file_size_in_bytes = config_get_int_value(config, METADATA_TAMANIO_ARCHIVO);
        void *file_blocks = read_blocks(config_get_int_value(config, METADATA_BLOQUE_INICIAL), 0, file_size_in_bytes);

        memcpy(blocks_cpy + blocks_offset * cfg_io->block_size, file_blocks, file_size_in_bytes); //bytes
        fcb_set_first_block(name, blocks_offset); // bloques
        
        uint32_t file_size_in_blocks = ceil((double) file_size_in_bytes / cfg_io->block_size);
        uint32_t real_size = file_size_in_blocks ? file_size_in_blocks : 1;
        if(strcmp(name,file_name)){
            occupy_free_blocks(blocks_offset, real_size); //bloques
            blocks_offset += real_size; //bloques
           // bytes_offset += file_size_in_bytes; //bytes

        } else {
            occupy_free_blocks(blocks_offset, target_blocks); //bloques
            blocks_offset += target_blocks; //bloques
           // bytes_offset += target_blocks * cfg_io->block_size; //bytes
        }
        free(file_blocks);
    }
    dictionary_iterator(metadata_dictionary, iterator);

    free_blocks(blocks_offset, cfg_io->block_count - blocks_offset);

    update_blocks(blocks_cpy);

    free(blocks_cpy);
}