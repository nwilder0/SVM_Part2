#include "kernel.h"

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <limits>

namespace vm
{
    Kernel::Kernel(Scheduler scheduler, std::vector<std::string> executables_paths)
        : machine(), processes(), priorities(), scheduler(scheduler),
          _last_issued_process_id(0),
		  _current_process_index(0), 
		  _cycles_passed_after_preemption(0)
    {
        // Memory

		machine.mmu.ram[0] = _free_physical_memory_index = 0;
		machine.mmu.ram[1] = machine.mmu.ram.size() - 2;

		//this->page_table = MMU::CreateEmptyPageTable();
		//this->blocklist = MMU::CreateNewVMBlockList();

        // Process page faults (find an empty frame)
        machine.pic.isr_4 = [&]() {
            std::cout << "Kernel: page fault." << std::endl;

			MMU::page_entry_type page = machine.cpu.registers.a;

			MMU::page_entry_type frame = machine.mmu.AcquireFrame();

			if(frame != MMU::INVALID_PAGE) {

				(*(machine.mmu.page_table))[page] = frame;
			} else {
				std::cout << "Kernel: Error on Page Fault - Process: " << processes[_current_process_index].id << " skipping instruction: " << machine.cpu.registers.ip << std::endl;
				machine.cpu.registers.ip += 2;
				// or machine.Stop();
			}
        };

        // Process Management

        std::for_each(executables_paths.begin(), executables_paths.end(), [&](const std::string &path) {
            CreateProcess(path);
        });

        if (!processes.empty()) {
            std::cout << "Kernel: setting the first process: " << processes[_current_process_index].id << " for execution." << std::endl;

            machine.cpu.registers = processes[_current_process_index].registers;
			machine.mmu.page_table = processes[_current_process_index].page_table;
			machine.mmu.blocklist = processes[_current_process_index].blocklist;
			
            processes[_current_process_index].state = Process::Running;
        }

        if (scheduler == FirstComeFirstServed || scheduler == ShortestJob) {
            machine.pic.isr_0 = [&]() {};
            machine.pic.isr_3 = [&]() {};
        } else if (scheduler == RoundRobin) {
            machine.pic.isr_0 = [&]() {
                std::cout << "Kernel: processing the timer interrupt." << std::endl;

                if (!processes.empty()) {
                    if (_cycles_passed_after_preemption <= Kernel::_MAX_CYCLES_BEFORE_PREEMPTION)
                    {
                        std::cout << "Kernel: allowing the current process " << processes[_current_process_index].id << " to run." << std::endl;

                        ++_cycles_passed_after_preemption;

                        std::cout << "Kernel: the current cycle is " << _cycles_passed_after_preemption << std::endl;
                    } else {
                        if (processes.size() > 1) {
                            std::cout << "Kernel: switching the context from process " << processes[_current_process_index].id;
                
                            processes[_current_process_index].registers = machine.cpu.registers;
                            processes[_current_process_index].state = Process::Ready;

                            _current_process_index = (_current_process_index + 1) % processes.size();

                            std::cout << " to process " << processes[_current_process_index].id << std::endl;

                            machine.cpu.registers = processes[_current_process_index].registers;
							machine.mmu.page_table = processes[_current_process_index].page_table;
							machine.mmu.blocklist = processes[_current_process_index].blocklist;
							
                            processes[_current_process_index].state = Process::Running;
                        }

                        _cycles_passed_after_preemption = 0;
                    }
                }

                std::cout << std::endl;
            };

            machine.pic.isr_3 = [&]() {
                std::cout << "Kernel: processing the first software interrupt." << std::endl;

                if (!processes.empty()) {
                    std::cout << "Kernel: unloading the process " << processes[_current_process_index].id << std::endl;
					
					//clear out the process' VM
					for(int i = 0; i<processes[_current_process_index].page_table->size(); i++) {
						machine.mmu.ReleaseFrame((*(processes[_current_process_index].page_table))[i]);
					}
					//send the start position of the memory to be freed
					FreeMemory(processes[_current_process_index].memory_start_position,NULL);
					
                    processes.erase(processes.begin() + _current_process_index);

                    if (processes.empty()) {
                        _current_process_index = 0;

                        std::cout << "Kernel: no more processes. Stopping the machine." << std::endl;

                        machine.Stop();
                    } else {
                        if (_current_process_index >= processes.size()) {
                            _current_process_index %= processes.size();
                        }

                        std::cout << "Kernel: switching the context to process " << processes[_current_process_index].id << std::endl;

                        machine.cpu.registers = processes[_current_process_index].registers;
						machine.mmu.page_table = processes[_current_process_index].page_table;
						machine.mmu.blocklist = processes[_current_process_index].blocklist;
						
                        processes[_current_process_index].state = Process::Running;

                        _cycles_passed_after_preemption = 0;
                    }
                }

                std::cout << std::endl;
            };
        } else if (scheduler == Priority) {
            machine.pic.isr_0 = [&]() {};
            machine.pic.isr_3 = [&]() {};
        }

        machine.Start();
    }

    Kernel::~Kernel() {}

    void Kernel::CreateProcess(const std::string &name)
    {
        if (_last_issued_process_id == std::numeric_limits<Process::process_id_type>::max()) {
            std::cerr << "Kernel: failed to create a new process. The maximum number of processes has been reached." << std::endl;
        } else {
            std::ifstream input_stream(name, std::ios::in | std::ios::binary);
            if (!input_stream) {
                std::cerr << "Kernel: failed to open the program file." << std::endl;
            } else {
                MMU::ram_type ops;

                input_stream.seekg(0, std::ios::end);
                auto file_size = input_stream.tellg();
                input_stream.seekg(0, std::ios::beg);
                ops.resize(static_cast<MMU::ram_size_type>(file_size) / 4);

                input_stream.read(reinterpret_cast<char *>(&ops[0]), file_size);

                if (input_stream.bad()) {
                    std::cerr << "Kernel: failed to read the program file." << std::endl;
                } else {

					// get the position of the first-fit memory location with sufficient size
                    MMU::ram_size_type new_memory_position = AllocateMemory(ops.size(),NULL);
                    if (new_memory_position == -1) {
                        std::cerr << "Kernel: failed to allocate memory." << std::endl;
                    } else {
                        std::copy(ops.begin(), ops.end(), (machine.mmu.ram.begin() + new_memory_position));

                        Process process(_last_issued_process_id++, new_memory_position,
                                                                   new_memory_position + ops.size());

                        // Old sequential allocation
                        //
                        // std::copy(ops.begin(), ops.end(), (machine.memory.ram.begin() + _last_ram_position));
                        //
                        // Process process(_last_issued_process_id++, _last_ram_position,
                        //                                            _last_ram_position + ops.size());
                        //
                        // _last_ram_position += ops.size();
                    }
                }
            }
        }
    }

    MMU::ram_size_type Kernel::AllocateMemory(MMU::ram_size_type units, Process *process)
    {
		MMU::ram_size_type new_allocation = -1;
		MMU::header *current;

		if(process) 
		{	
			current = process->blocklist;
		} else {
			current = machine.mmu.real_list;
		}

		MMU::ram_size_type frame_units = units / MMU::PAGE_SIZE + 1;

		while(current) {
			if(current->free && current->size >= frame_units) {

				// alloc from this block
				new_allocation = current->block;
				current->free = false;

				if(current->size > frame_units) {
					MMU::header *tail = new MMU::header();
					tail->block = current->block + frame_units;
					tail->next = current->next;
					tail->free = true;
					tail->size = current->size - frame_units;
				
					current->next = tail;
					current->size = frame_units;
				}
			} else {
				current = current->next;
			}
		}
		return new_allocation;
    }

    void Kernel::FreeMemory(MMU::ram_size_type physical_memory_index, Process *process)
    {
		MMU::header *current;
		if(process) {
			current = process->blocklist;
		} else {
			current = machine.mmu.real_list;
		}

		MMU::header *prev = NULL;

		while(current) {
			if(current->block == physical_memory_index) {
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
