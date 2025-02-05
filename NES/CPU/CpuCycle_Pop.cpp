#include "..\NES.h"

DWORD HandleInstructionCycle_Pop()
{
	BYTE bValue = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_POP_BEGIN)
	{
		// no operation - ignore cycle
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_POP_NOP;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_POP_NOP)
	{
		// no operation - ignore cycle
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_POP_READ;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_POP_READ)
	{
		// pop value
		Stack_Pop(&bValue);

		if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_PLA)
		{
			// PLA instruction
			gSystem.Cpu.Reg.bA = bValue;

			// update flags
			StatusFlag_SetAuto_Zero(gSystem.Cpu.Reg.bA);
			StatusFlag_SetAuto_Negative(gSystem.Cpu.Reg.bA);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_PLP)
		{
			// PLP instruction
			StatusFlag_SetPendingInterruptDisableFlagUpdate();
			gSystem.Cpu.Reg.bStatus = bValue;
			StatusFlag_FixAfterRestore();
		}
		else
		{
			// invalid instruction
			return 1;
		}
		
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
