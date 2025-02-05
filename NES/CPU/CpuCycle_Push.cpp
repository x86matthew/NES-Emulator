#include "..\NES.h"

DWORD HandleInstructionCycle_Push()
{
	BYTE bValue = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_PUSH_BEGIN)
	{
		// cycle ignored
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_PUSH_WRITE;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_PUSH_WRITE)
	{
		if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_PHA)
		{
			// PHA instruction
			bValue = gSystem.Cpu.Reg.bA;
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_PHP)
		{
			// PHP instruction
			bValue = gSystem.Cpu.Reg.bStatus | CPU_STATUS_FLAG_BREAK;
		}
		else
		{
			// invalid instruction
			return 1;
		}
		
		// push value
		Stack_Push(bValue);

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
