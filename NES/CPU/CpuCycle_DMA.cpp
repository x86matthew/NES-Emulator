#include "..\NES.h"

DWORD HandleInstructionCycle_DMA()
{
	BYTE bImmediate = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_DMA_BEGIN)
	{
		// check if this is an even cycle
		if((gSystem.Cpu.bCpuCounter % 2) == 0)
		{
			// try again next cycle
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_DMA_BEGIN;
		}
		else
		{
			// reset counter
			gSystem.Cpu.wTempStateValue16 = 0;

			// read first byte
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_DMA_READ;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_DMA_READ)
	{
		// read byte
		Memory_Read8(gSystem.Cpu.wDmaBaseAddress + gSystem.Cpu.wTempStateValue16, &gSystem.Cpu.bTempStateValue8);

		// write byte
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_DMA_WRITE;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_DMA_WRITE)
	{
		// write byte to OAM
		PpuObjectAttributeMemory_Write(gSystem.Cpu.bTempStateValue8);
		gSystem.Ppu.bObjectAttributeMemoryAddress++;

		// increase counter
		gSystem.Cpu.wTempStateValue16++;

		// check if all 256 bytes have been copied
		if(gSystem.Cpu.wTempStateValue16 < 256)
		{
			// read next byte
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_DMA_READ;
		}
		else
		{
			// finished
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
		}
	}
	else
	{
		// invalid state
		return 1;
	}

	return 0;
}
