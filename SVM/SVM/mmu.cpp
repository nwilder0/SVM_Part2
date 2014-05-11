#include "mmu.h"

namespace vm
{
    MMU::MMU()
        : ram(RAM_SIZE)
    {
		
		real_list = new header();
		real_list->block = 0;
		real_list->size = ram.size()/PAGE_SIZE;
		real_list->next = NULL;
		real_list->free = true;
		
		//for (page_entry_type frame = PAGE_SIZE; frame< RAM_SIZE; frame += PAGE_SIZE) 
		//{
		//	free_frames.push(frame);
		//}
    }

    MMU::~MMU() {}

    MMU::page_table_type* MMU::CreateEmptyPageTable()
    {
		return new page_table_type(RAM_SIZE / PAGE_SIZE);
    }

	MMU::header* MMU::CreateNewVMBlockList() 
	{
		MMU::header *blocklist = new header();
		blocklist->block = 0;
		blocklist->size = RAM_SIZE/PAGE_SIZE;
		blocklist->next = NULL;
		blocklist->free = true;
		return blocklist;
	}

    MMU::page_index_offset_pair_type MMU::GetPageIndexAndOffsetForVirtualAddress(vmem_size_type address)
    {
        page_index_offset_pair_type result = std::make_pair((page_table_size_type) -1, (ram_size_type) -1);

		result.first = address / PAGE_SIZE;
		result.second = address % PAGE_SIZE;

        return result;
    }

    MMU::page_entry_type MMU::AcquireFrame()
    {
		MMU::header *current = real_list;
		MMU::page_entry_type result = INVALID_PAGE;

		while(current) {
			if(current->free) {
				
				result = current->block;
				current->free = false;

				if(current->size > 1) {
					MMU::header *tail = new MMU::header();
					tail->block = current->block + 1;
					tail->next = current->next;
					tail->free = true;
					tail->size = current->size - 1;
					
					current->next = tail;
					current->size = 1;					
				}
			} else {
				current = current->next;
			}
		}
		return result;
    }
    
    void MMU::ReleaseFrame(page_entry_type page)
    {
        //free_frames.push(page);
			MMU::header *current = real_list;
			MMU::header *prev = NULL;

			while(current) {
				if(current->block == page) {
					current->free = true;
					if(prev) {
						if(prev->free) {
							// merge
							prev->next = current->next;
							prev->size += current->size;
							current = prev;
						}
					}
					if(current->next) {
						if(current->next->free) {
							//merge
							current->size += current->next->size;
							current->next = current->next->next;
						}
					}
					current = NULL;
				} else {
					prev = current;
					current = current->next;
				}

			}
    }
}
