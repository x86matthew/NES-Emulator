#include "..\NES.h"

DWORD HandleInstructionCycle_Indirect()
{
	BYTE bHighByte = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INDIRECT_BEGIN)
	{
		// read low byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)&gSystem.Cpu.wTempStateValue16);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INDIRECT_READ_ADDRESS_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INDIRECT_READ_ADDRESS_HIGH)
	{
		// read high byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)((BYTE*)&gSystem.Cpu.wTempStateValue16 + 1));

		// update PC
		gSystem.Cpu.Reg.wPC = gSystem.Cpu.wTempStateValue16;

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INDIRECT_READ_POINTER_LOW;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INDIRECT_READ_POINTER_LOW)
	{
		// read low byte
		Memory_Read8(gSystem.Cpu.wTempStateValue16, &gSystem.Cpu.bTempStateValue8);

		// increase address (low byte only - page-cross not supported)
		(*(BYTE*)&gSystem.Cpu.wTempStateValue16)++;

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INDIRECT_READ_POINTER_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INDIRECT_READ_POINTER_HIGH)
	{
		// read high byte
		Memory_Read8(gSystem.Cpu.wTempStateValue16, &bHighByte);

		// update PC
		gSystem.Cpu.Reg.wPC = ((WORD)bHighByte << 8) + gSystem.Cpu.bTempStateValue8;

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
