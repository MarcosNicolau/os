#include<mem_monitor.h>


static void* physical_mem;
static pthread_mutex_t MEM_MUTEX;

int init_ram(int size){
    pthread_mutex_init(&MEM_MUTEX,0);
    physical_mem = calloc(size,sizeof(char));
    if(physical_mem == NULL) return -1;
    return 0;
}

void write_mem(uint32_t physical_address, void * value, uint32_t size){
    pthread_mutex_lock(&MEM_MUTEX);
    memcpy(physical_mem + physical_address,value,size);
    pthread_mutex_unlock(&MEM_MUTEX);
    
}

void read_mem(uint32_t physical_address, void * value, uint32_t size){
    pthread_mutex_lock(&MEM_MUTEX);
    memcpy(value,physical_mem + physical_address,size);
    pthread_mutex_unlock(&MEM_MUTEX);
}