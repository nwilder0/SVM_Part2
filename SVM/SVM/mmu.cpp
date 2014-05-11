#include "mmu.h"

namespace vm
{
    MMU::MMU()
        : ram(RAM_SIZE)
    {
        // TODO: initialize data structures for the frame allocator
    }

    MMU::~MMU() {}

    MMU::page_table_type* MMU::CreateEmptyPageTable()
    {
		/*
			TODO:

			Return a new page table (for kernel or processes)
			Each entry should be invalid
		*/

		return nullptr;
    }

    MMU::page_index_offset_pair_type MMU::GetPageIndexAndOffsetForVirtualAddress(vmem_size_type address)
    {
        page_index_offset_pair_type result = std::make_pair((page_table_size_type) -1, (ram_size_type) -1);

		/*
			TODO:
			
			Calculate the page index from the virtual address
			Calculate the offset in the physical memory from the virtual address
		*/

        return result;
    }

    MMU::page_entry_type MMU::AcquireFrame()
    {
		// TODO: find a new free frame (you can use a bitmap or stack)

        return INVALID_PAGE;
    }
    
    void MMU::ReleaseFrame(page_entry_type page)
    {
        // TODO: free the physical frame (you can use a bitmap or stack)
    }
}
