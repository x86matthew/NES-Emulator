#include "..\NES.h"

DWORD HandleInstructionCycle_JSR()
{
	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_JSR_BEGIN)
	{
		// read low byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)&gSystem.Cpu.wTempStateValue16);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_JSR_NOP;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_JSR_NOP)
	{
		// no operation - ignore cycle
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_JSR_PUSH_PC_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_JSR_PUSH_PC_HIGH)
	{
		// push PC high byte
		Stack_Push((BYTE)(gSystem.Cpu.Reg.wPC >> 8));

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_JSR_PUSH_PC_LOW;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_JSR_PUSH_PC_LOW)
	{
		// push PC low byte
		Stack_Push((BYTE)gSystem.Cpu.Reg.wPC);

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_JSR_UPDATE_PC;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_JSR_UPDATE_PC)
	{
		// read high byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)((BYTE*)&gSystem.Cpu.wTempStateValue16 + 1));

		// update PC
		gSystem.Cpu.Reg.wPC = gSystem.Cpu.wTempStateValue16;

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
