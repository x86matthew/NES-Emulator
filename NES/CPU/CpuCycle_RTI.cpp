#include "..\NES.h"

DWORD HandleInstructionCycle_RTI()
{
	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTI_BEGIN)
	{
		// cycle ignored
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTI_NOP;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTI_NOP)
	{
		// cycle ignored
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTI_POP_STATUS;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTI_POP_STATUS)
	{
		// pop status value
		Stack_Pop((BYTE*)&gSystem.Cpu.Reg.bStatus);
		StatusFlag_FixAfterRestore();

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTI_POP_PC_LOW;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTI_POP_PC_LOW)
	{
		// pop PC low byte
		Stack_Pop((BYTE*)&gSystem.Cpu.Reg.wPC);

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTI_POP_PC_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTI_POP_PC_HIGH)
	{
		// pop PC high byte
		Stack_Pop((BYTE*)&gSystem.Cpu.Reg.wPC + 1);

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
