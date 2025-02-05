#include "..\NES.h"

DWORD HandleInstructionCycle_ZeroPageXY()
{
	BYTE bValue = 0;
	BYTE bZeroPageOffset = 0;
	WORD wValue = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ZERO_PAGE_XY_BEGIN)
	{
		// read byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, &bZeroPageOffset);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		// store address
		gSystem.Cpu.wTempStateValue16 = bZeroPageOffset;

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ZERO_PAGE_XY_ADD_INDEX;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ZERO_PAGE_XY_ADD_INDEX)
	{
		// calculate new address
		wValue = gSystem.Cpu.wTempStateValue16;
		if(gSystem.Cpu.pCurrInstruction->AddressMode == ADDRESS_MODE_ZERO_PAGE_X)
		{
			// X mode
			wValue += gSystem.Cpu.Reg.bX;
		}
		else if(gSystem.Cpu.pCurrInstruction->AddressMode == ADDRESS_MODE_ZERO_PAGE_Y)
		{
			// Y mode
			wValue += gSystem.Cpu.Reg.bY;
		}
		else
		{
			return 1;
		}

		// update address (ignore high byte)
		gSystem.Cpu.wTempStateValue16 = (BYTE)wValue;

		// check if this is a read/write/read-modify-write instruction
		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ)
		{
			// read instruction
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ZERO_PAGE_XY_READ_ADDRESS;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_WRITE)
		{
			// write instruction
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ZERO_PAGE_XY_WRITE_ADDRESS;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// read-modify-write instruction
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ZERO_PAGE_XY_READ_ADDRESS;
		}
		else
		{
			return 1;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ZERO_PAGE_XY_READ_ADDRESS)
	{
		// read value
		Memory_Read8(gSystem.Cpu.wTempStateValue16, &bValue);

		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ)
		{
			// call common handler
			if(CallCommonInstructionHandler(bValue, 0) != 0)
			{
				return 1;
			}

			// finished
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// store original value to modify
			gSystem.Cpu.bTempStateValue8 = bValue;
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ZERO_PAGE_XY_MODIFY;
		}
		else
		{
			return 1;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ZERO_PAGE_XY_MODIFY)
	{
		// write original value back to address (this emulates real 6502 behaviour)
		Memory_Write8(gSystem.Cpu.wTempStateValue16, gSystem.Cpu.bTempStateValue8);

		// call common handler
		if(CallCommonInstructionHandler(gSystem.Cpu.bTempStateValue8, &gSystem.Cpu.bTempStateValue8) != 0)
		{
			return 1;
		}

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ZERO_PAGE_XY_WRITE_ADDRESS;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ZERO_PAGE_XY_WRITE_ADDRESS)
	{
		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// retrieve modified value
			bValue = gSystem.Cpu.bTempStateValue8;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_WRITE)
		{
			// call common handler
			if(CallCommonInstructionHandler(0, &bValue) != 0)
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}

		// write value
		Memory_Write8(gSystem.Cpu.wTempStateValue16, bValue);

		// finished
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
	}
	else
	{
		// invalid state
		return 1;
	}

	return 0;
}
