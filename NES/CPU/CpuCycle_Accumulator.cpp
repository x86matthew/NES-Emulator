#include "..\NES.h"

DWORD HandleInstructionCycle_Accumulator()
{
	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ACCUMULATOR_BEGIN)
	{
		// call common handler
		if(CallCommonInstructionHandler(gSystem.Cpu.Reg.bA, &gSystem.Cpu.Reg.bA) != 0)
		{
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
