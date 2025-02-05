#include "..\NES.h"

DWORD HandleInstructionCycle_RTS()
{
	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTS_BEGIN)
	{
		// cycle ignored
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTS_NOP;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTS_NOP)
	{
		// cycle ignored
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTS_POP_PC_LOW;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTS_POP_PC_LOW)
	{
		// pop PC low byte
		Stack_Pop((BYTE*)&gSystem.Cpu.Reg.wPC);

		// update state
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTS_POP_PC_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTS_POP_PC_HIGH)
	{
		// pop PC high byte
		Stack_Pop((BYTE*)&gSystem.Cpu.Reg.wPC + 1);

		// update state
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RTS_INCREASE_PC;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RTS_INCREASE_PC)
	{
		// increase PC
		gSystem.Cpu.Reg.wPC++;

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
