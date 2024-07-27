#include <mmu.h>

uint32_t get_frame(uint32_t page_number, t_log *logger)
{
    uint32_t frame_number = tlb_search(context.pid, page_number, logger);
    if (frame_number == -1)
    {
        frame_number = access_page_table(page_number);
        if (frame_number == -1)
        {
            log_error(logger, "error al obtener el frame de memoria");
            exit(1);
        }
        tlb_insert(context.pid, page_number, frame_number, logger);
    }
    log_info(logger, "PID: %u - OBTENER MARCO - Página: %u - Marco: %u", context.pid, page_number, frame_number);
    return frame_number;
}

t_list *access_to_memory_create(uint32_t logical_address, uint32_t total_bytes, uint32_t page_size, t_log *logger)
{
    t_list *result = list_create();

    uint32_t first_page = floor(logical_address / page_size);
    // uint32_t extra_page = logical_address % page_size == 0 ? 0 : 1;
    // uint32_t pages_amount = total_bytes / page_size + extra_page;

    int i = 0;

    while (total_bytes > 0)
    {
        t_access_to_memory *access = malloc(sizeof(t_access_to_memory));

        uint32_t page_number = first_page + i;
        uint32_t offset = logical_address - page_number * page_size;

        uint32_t frame_number = get_frame(page_number, logger);

        uint32_t physical_address = frame_number * page_size + offset;

        access->address = physical_address;

        if (total_bytes + offset >= page_size)
        {
            access->bytes_to_access = page_size - offset;
        }
        else
        {
            access->bytes_to_access = total_bytes;
        }
        total_bytes -= access->bytes_to_access;
        logical_address += access->bytes_to_access;
        list_add(result, access);
        i++;
    }

    return result;
}