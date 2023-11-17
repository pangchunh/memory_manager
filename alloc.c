//
// Created by Chun Hang Pang on 7/11/2023.
//
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 4096

typedef struct memChunk{
    void *ptr;
    int size;
    struct memChunk* next;
}mem_t;

char* page_start = NULL;
static mem_t* free_list = NULL;
static mem_t* allocated_list = NULL;

void printState(){
    printf("==========page start===========\n");
    printf("page start: %p page end: %p \n", page_start, page_start + 4095);
    printf("==========free_list===========\n");
    printf("Free list %p\n", free_list->ptr);
    mem_t* current = free_list;
    while(current){
        printf("data address: (%p), size: (%d), next: (%p)\n", current->ptr, current->size, current->next);
        current = current->next;
    }
    printf("==========allocated_list===========\n");
    current = allocated_list;
    printf("allocated list %p\n", allocated_list);

    while(current){
        printf("data address: (%p), size: (%d), next: (%p)\n", current->ptr, current->size, current->next);
        current = current->next;
    }
    printf("================\n\n\n");
}

mem_t* createNode(void* ptr, int size, mem_t* next){
    mem_t* node = (mem_t*)malloc(sizeof (mem_t));
    node->ptr = ptr;
    node->size = size;
    node->next = next;
    return node;
}


int init_alloc(){
    page_start = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (page_start == MAP_FAILED){
        perror("Fail to create map");
        return -1;
    }
    free_list = createNode(page_start, PAGE_SIZE, NULL);
    printf("after init_alloc\n");
    printState();
    return 0;
}

int cleanup(){
    if (munmap(page_start, PAGE_SIZE) == -1) {
        perror("Error unmapping memory page");
        return -1; // Indicate failure
    }
    page_start = NULL;
    while(free_list){
        mem_t* next = free_list->next;
        free(free_list);
        free_list = next;
    }

    while(allocated_list){
        mem_t* next = allocated_list->next;
        free(allocated_list);
        allocated_list = next;
    }
    free_list = NULL;
    allocated_list = NULL;
    return 0;
}

char * alloc(int size){
    printState();
    if (size % 8 != 0){
        return NULL;
    }
    mem_t* prev = NULL;
    mem_t* current = free_list;

    while(current){
        if(current->size >= size){
                    mem_t* new_memory = createNode(current->ptr + size, current->size - size, current->next);
                if(prev){
                    prev->next = new_memory;
                } else{
                    free_list = new_memory;
                }
            current->size = size;
            current->next = allocated_list;
            allocated_list = current;
            printf("allocate successful, current state: \n");
            printState();
            printf("Returning this from alloc addr: %p size: %d, next: %p\n", current->ptr, current->size, current->next);
            return (char*)current->ptr;
        } else{
            prev = current;
            current = current->next;
        }

    }
    return NULL;
}

void sortList(mem_t* head) {
    int swapped;
    mem_t* current;
    mem_t* last = NULL;

    if(head == NULL || head->next == NULL){
        return;
    }
    do {
        swapped = 0;
        current = head;
        while (current->next != last){
            if(current->ptr > current->next->ptr){
                void* tmpPtr = current->ptr;
                int tmpSize = current->size;

                current->ptr = current->next->ptr;
                current->size = current->next->size;
                current->next->ptr = tmpPtr;
                current->next->size = tmpSize;
                swapped = 1;
            }
            current = current->next;
        }
        last = current;
    } while (swapped);

}

void insert_in_free_list(mem_t* target_chunk){
    free_list = createNode(target_chunk->ptr, target_chunk->size, free_list);
    free(target_chunk);
}

//assumme we place the current_chunk in the right position, we can try to merge any adjacent chunk
void merge_free_chunks(mem_t* target_chunk){
    if(free_list->size == 0){
        free_list = target_chunk;
        return;
    }
    printf("before sorting\n");
    sortList(free_list);
    printf("after sorting...\n");
    printState();

    mem_t* current = free_list;

    while(current != NULL && current->next !=NULL){
        void* currentEnd = (char*)current->ptr + current->size;
        void* nextStart = (char*)current->next->ptr;
        if(currentEnd == nextStart){
            current->size += current->next->size;
            mem_t *tmp = current->next;
            current->next = current->next->next;
            free(tmp);
        }else{
            current = current->next;
        }
    }
}

void dealloc(char * ptr){
    printf("entering dealloc..\n");
    printf("ptr to be dealloc %p\n", ptr);
    printState();
    if(!ptr){
        printf("ptr is null\n");
        return;
    }
    mem_t* prev = NULL;
    mem_t* current = allocated_list;
    if(!current->next){
        allocated_list = NULL;
    }
    while(current){
        if((char*)current->ptr == ptr){
            printf("current->ptr == ptr, before insert \n");
            insert_in_free_list(current);
            printf("after insert, printing...\n");
            printState();
            merge_free_chunks(current);

            printf("after merge\n");
            printState();
            if(prev){
                prev->next = current->next;
            } else{
                allocated_list = current->next;
            }
            printf("Successfully dealloc memory\n");
            printState();
            return;

        } else{
            prev = current;
            current = current->next;
        }
    }
    perror("cannot find ptr in the allocated list, fail.");
    printState();
    return;
}







