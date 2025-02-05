#include "..\NES.h"

DWORD HandleInstructionCycle_Immediate()
{
	BYTE bImmediate = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_IMMEDIATE_BEGIN)
	{
		// read immediate value
		Memory_Read8(gSystem.Cpu.Reg.wPC, &bImmediate);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		// call common handler
		if(CallCommonInstructionHandler(bImmediate, NULL) != 0)
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
