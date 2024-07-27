#include <blocks.h>

static void *blocks_mmap;

static void create_blocks(t_log *logger)
{
    char *blocks_name = mount_pathbase("bloques.dat");
    FILE *f = fopen(blocks_name, "wb+");
    int fd = fileno(f);
    if (-1 == ftruncate(fd, cfg_io->block_count * cfg_io->block_size))
    {
        log_error(logger, "Error al crear el archivo de bloques.dat: %s", strerror(errno));
        exit(1);
    }
    fclose(f);
    free(blocks_name);
    log_info(logger, "se ha creado un nuevo archivo de bloques.dat");
}

void load_blocks(t_log *logger)
{
    char *blocks_name = mount_pathbase("bloques.dat");
    FILE *f = fopen(blocks_name, "rb+");
    if (!f)
    {
        create_blocks(logger);
        f = fopen(blocks_name, "rb+");
    }

    int blocks_fd = fileno(f);

    blocks_mmap = mmap(0, cfg_io->block_count * cfg_io->block_size, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);

    free(blocks_name);

    fclose(f);
}

void write_blocks(uint32_t first_block, uint32_t file_ptr, void *value, int size)
{
    uint32_t offset = first_block * cfg_io->block_size + file_ptr;
    memcpy(blocks_mmap + offset, value, size);
    msync(blocks_mmap, cfg_io->block_count * cfg_io->block_size, MS_SYNC);
}

void *read_blocks(uint32_t first_block, uint32_t file_ptr, int size)
{
    void *res = malloc(size);
    uint32_t offset = first_block * cfg_io->block_size + file_ptr;
    memcpy(res, blocks_mmap + offset, size);
    return res;
}

void close_blocks(void)
{
    munmap(blocks_mmap, cfg_io->block_count * cfg_io->block_size);
}

void update_blocks(void *new_blocks)
{
    memcpy(blocks_mmap, new_blocks, cfg_io->block_count * cfg_io->block_size);
    msync(blocks_mmap, cfg_io->block_count * cfg_io->block_size, MS_SYNC);
}
