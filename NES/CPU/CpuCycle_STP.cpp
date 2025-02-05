#include "..\NES.h"

DWORD HandleInstructionCycle_STP()
{
	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_STP_BEGIN)
	{
		// cpu halted - infinite loop
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_STP_BEGIN;
	}
	else
	{
		// invalid state
		return 1;
	}

	return 0;
}
