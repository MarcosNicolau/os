#include <bitarray_monitor.h>

static t_bitarray *mem_bitarray;
static pthread_mutex_t MUTEX_MEM_BITARRAY;

void init_mem_bitarray(int mem_size, int frame_size)
{
    pthread_mutex_init(&MUTEX_MEM_BITARRAY, 0);
    int total_frames = mem_size / frame_size;
    int total_frames_in_bytes = ceil((double)total_frames / 8.0);
    char *raw_bitmap = calloc(total_frames_in_bytes, sizeof(char));
    mem_bitarray = bitarray_create_with_mode(raw_bitmap, total_frames_in_bytes, LSB_FIRST);
}

bool test_frame(uint32_t frame_number)
{
    pthread_mutex_lock(&MUTEX_MEM_BITARRAY);
    bool res = bitarray_test_bit(mem_bitarray, frame_number);
    pthread_mutex_unlock(&MUTEX_MEM_BITARRAY);
    return res;
}

void set_frame_ocuppied(uint32_t frame_number)
{
    pthread_mutex_lock(&MUTEX_MEM_BITARRAY);
    bitarray_set_bit(mem_bitarray, frame_number);
    pthread_mutex_unlock(&MUTEX_MEM_BITARRAY);
}

void clear_frame(uint32_t frame_number)
{
    pthread_mutex_lock(&MUTEX_MEM_BITARRAY);
    bitarray_clean_bit(mem_bitarray, frame_number);
    pthread_mutex_unlock(&MUTEX_MEM_BITARRAY);
}
// quizas destroy

