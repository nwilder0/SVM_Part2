#include "cpu.h"

#include <iostream>

namespace vm
{
    Registers::Registers()
        : a(0), b(0), c(0), flags(0), ip(0), sp(0) {}

    CPU::CPU(MMU &mmu, PIC &pic): registers(), _mmu(mmu), _pic(pic) {}

    CPU::~CPU() {}

    void CPU::Step()
    {
        int ip = registers.ip;

        int instruction = _mmu.ram[ip];
        int data = _mmu.ram[ip + 1];
		MMU::page_index_offset_pair_type page_index_and_offset;
		MMU::page_entry_type frame;

        switch (instruction) {
        case CPU::MOVA_BASE_OPCODE:
            registers.a = data;
            registers.ip += 2;

            break;
        case CPU::MOVB_BASE_OPCODE:
            registers.b = data;
            registers.ip += 2;

            break;
        case CPU::MOVC_BASE_OPCODE:
            registers.c = data;
            registers.ip += 2;

            break;

		case CPU::LDA_BASE_OPCODE:

			page_index_and_offset =  _mmu.GetPageIndexAndOffsetForVirtualAddress(data);
			frame = _mmu.page_table->at(page_index_and_offset.first);
			if (frame== MMU::INVALID_PAGE) {

				int temp = registers.a;
				registers.a = page_index_and_offset.first;
				_pic.isr_4();
				registers.a = temp;

			} else {
				registers.a = _mmu.ram[frame + page_index_and_offset.second];
				registers.ip += 2;
			}
			break;

		case CPU::LDB_BASE_OPCODE:

			page_index_and_offset =  _mmu.GetPageIndexAndOffsetForVirtualAddress(data);
			frame = _mmu.page_table->at(page_index_and_offset.first);
			if (frame== MMU::INVALID_PAGE) {

				int temp = registers.a;
				registers.a = page_index_and_offset.first;
				_pic.isr_4();
				registers.a = temp;

			} else {
				registers.b = _mmu.ram[frame + page_index_and_offset.second];
				registers.ip += 2;
			}
			break;

		case CPU::LDC_BASE_OPCODE:

			page_index_and_offset =  _mmu.GetPageIndexAndOffsetForVirtualAddress(data);
			frame = _mmu.page_table->at(page_index_and_offset.first);
			if (frame== MMU::INVALID_PAGE) {

				int temp = registers.a;
				registers.a = page_index_and_offset.first;
				_pic.isr_4();
				registers.a = temp;

			} else {
				registers.c = _mmu.ram[frame + page_index_and_offset.second];
				registers.ip += 2;
			}
			break;

		case CPU::STA_BASE_OPCODE:

			page_index_and_offset =  _mmu.GetPageIndexAndOffsetForVirtualAddress(data);
			frame = _mmu.page_table->at(page_index_and_offset.first);
			if (frame== MMU::INVALID_PAGE) {

				int temp = registers.a;
				registers.a = page_index_and_offset.first;
				_pic.isr_4();
				registers.a = temp;

			} else {
				_mmu.ram[frame + page_index_and_offset.second] = registers.a;
				registers.ip += 2;
			}
			break;

		case CPU::STB_BASE_OPCODE:

			page_index_and_offset =  _mmu.GetPageIndexAndOffsetForVirtualAddress(data);
			frame = _mmu.page_table->at(page_index_and_offset.first);
			if (frame== MMU::INVALID_PAGE) {

				int temp = registers.a;
				registers.a = page_index_and_offset.first;
				_pic.isr_4();
				registers.a = temp;

			} else {
				_mmu.ram[frame + page_index_and_offset.second] = registers.b;
				registers.ip += 2;
			}
			break;

		case CPU::STC_BASE_OPCODE:

			page_index_and_offset =  _mmu.GetPageIndexAndOffsetForVirtualAddress(data);
			frame = _mmu.page_table->at(page_index_and_offset.first);
			if (frame== MMU::INVALID_PAGE) {

				int temp = registers.a;
				registers.a = page_index_and_offset.first;
				_pic.isr_4();
				registers.a = temp;

			} else {
				_mmu.ram[frame + page_index_and_offset.second] = registers.c;
				registers.ip += 2;
			}
			break;

        case CPU::JMP_BASE_OPCODE:
            registers.ip += data;

            break;
        case CPU::INT_BASE_OPCODE:
            _pic.isr_3();

            break;
        default:
            std::cerr << "CPU: invalid opcode data (" << instruction << "). Skipping..." << std::endl;
            registers.ip += 2;

            break;
        }
    }
}
