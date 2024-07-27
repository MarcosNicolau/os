#include <tlb.h>

static t_tlb TLB;

void tlb_init(int entries_amount, char *tlb_algorithm)
{
    TLB.entries = list_create();
    TLB.entries_amount = entries_amount;
    if (!strcmp("FIFO", tlb_algorithm))
    {
        TLB.algorithm = FIFO_TLB;
    }
    else
    {
        TLB.algorithm = LRU_TLB;
    }
}

int tlb_search(uint32_t pid, uint32_t page, t_log *logger)
{

    if (0 == TLB.entries_amount)
    {
        // creo que no hay que loggear nada, ya que esto es equivalente a no tener tlb
        //log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pid, page);
        return -1;
    }

    bool find_entry(void *elem)
    {
        t_tlb_row *temp = (t_tlb_row *)elem;
        return pid == temp->pid && page == temp->page;
    }

    t_tlb_row *entry = (t_tlb_row *)list_find(TLB.entries, find_entry);

    if (!entry)
    {
        log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pid, page);
        return -1;
    }

    log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pid, page);

    if (TLB.algorithm == LRU_TLB)
    {
        entry->last_use =  current_timestamp();
    }

    return entry->frame;
}

void tlb_insert(uint32_t pid, uint32_t page, uint32_t frame, t_log *logger)
{
    if (0 == TLB.entries_amount)
        return;
    int current_size = list_size(TLB.entries);

    if (current_size < TLB.entries_amount)
    {
        t_tlb_row *entry = malloc(sizeof(t_tlb_row));
        entry->pid = pid;
        entry->page = page;
        entry->frame = frame;
        entry->last_use = current_timestamp();
        list_add(TLB.entries, entry);
        return;
    }

    void *min_tlb(void *elem1, void *elem2)
    {
        t_tlb_row *entry1 = (t_tlb_row *)elem1;
        t_tlb_row *entry2 = (t_tlb_row *)elem2;
        if (entry1->last_use <  entry2->last_use)
            return entry1;
        else
            return entry2;
    }

    t_tlb_row *lowest_timestamp = (t_tlb_row *)list_get_minimum(TLB.entries, min_tlb);

    log_debug(logger, "TLB - Reemplaza PAGINA: %d - PID: %d por PAGINA: %d - PID: %d", lowest_timestamp->page, lowest_timestamp->pid, page, pid);

    lowest_timestamp->pid = pid;
    lowest_timestamp->page = page;
    lowest_timestamp->frame = frame;
    lowest_timestamp->last_use = current_timestamp();
}

void cleanup_tlb(uint32_t pid, uint32_t size){
    uint32_t page_amount =  ceil( (double) size / PAGE_SIZE);

    bool closure(void *elem){
        t_tlb_row* entry = (t_tlb_row*) elem;
        return entry->pid == pid && entry->page >= page_amount;
    }

    list_remove_and_destroy_all_by_condition(TLB.entries,closure,free);
}

void tlb_dump(t_log* logger){
    void print_tlb_entry(void* elem){
        t_tlb_row* entry = (t_tlb_row*) elem;
        log_debug(logger,"entrada: %d | %d | %d | TUR: %lld",entry->pid,entry->page,entry->frame,entry->last_use);
    }

    list_iterate(TLB.entries,print_tlb_entry);
}