#include "..\NES.h"

DWORD HandleInstructionCycle_Absolute()
{
	BYTE bValue = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_BEGIN)
	{
		// read low byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)&gSystem.Cpu.wTempStateValue16);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		// read next byte
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_READ_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_READ_HIGH)
	{
		// read high byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)((BYTE*)&gSystem.Cpu.wTempStateValue16 + 1));

		if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_JMP)
		{
			// jmp - update PC
			gSystem.Cpu.Reg.wPC = gSystem.Cpu.wTempStateValue16;

			// finished
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
		}
		else
		{
			// increase PC
			gSystem.Cpu.Reg.wPC++;

			// check if this is a read/write/read-modify-write instruction
			if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ)
			{
				// read instruction
				gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_READ_ADDRESS;
			}
			else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_WRITE)
			{
				// write instruction
				gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_WRITE_ADDRESS;
			}
			else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
			{
				// read-modify-write instruction
				gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_READ_ADDRESS;
			}
			else
			{
				return 1;
			}
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_READ_ADDRESS)
	{
		// read value from address
		Memory_Read8(gSystem.Cpu.wTempStateValue16, &bValue);

		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ)
		{
			// call common handler
			if(CallCommonInstructionHandler(bValue, NULL) != 0)
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
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_MODIFY;
		}
		else
		{
			return 1;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_MODIFY)
	{
		// write original value back to address (this emulates real 6502 behaviour)
		Memory_Write8(gSystem.Cpu.wTempStateValue16, gSystem.Cpu.bTempStateValue8);

		// call common handler
		if(CallCommonInstructionHandler(gSystem.Cpu.bTempStateValue8, &gSystem.Cpu.bTempStateValue8) != 0)
		{
			return 1;
		}

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_WRITE_ADDRESS;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_WRITE_ADDRESS)
	{
		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// retrieve modified value
			bValue = gSystem.Cpu.bTempStateValue8;
		}
		else
		{
			// call common handler
			if(CallCommonInstructionHandler(0, &bValue) != 0)
			{
				return 1;
			}
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
