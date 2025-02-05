#include "..\NES.h"

DWORD HandleInstructionCycle_Interrupt()
{
	BYTE bStatusValue = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INTERRUPT_BEGIN)
	{
		if(gSystem.Cpu.pInstructionAttributes->InstructionType == INSTRUCTION_TYPE_NMI)
		{
			// NMI - set handler address
			gSystem.Cpu.wTempStateValue16 = INTERRUPT_VECTOR_NMI;
		}
		else if(gSystem.Cpu.pInstructionAttributes->InstructionType == INSTRUCTION_TYPE_BRK)
		{
			// BRK instruction - set handler address
			gSystem.Cpu.wTempStateValue16 = INTERRUPT_VECTOR_IRQ;

			// increase PC
			gSystem.Cpu.Reg.wPC++;
		}
		else
		{
			return 1;
		}

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INTERRUPT_PUSH_PC_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INTERRUPT_PUSH_PC_HIGH)
	{
		// push PC high byte
		Stack_Push((BYTE)(gSystem.Cpu.Reg.wPC >> 8));

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INTERRUPT_PUSH_PC_LOW;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INTERRUPT_PUSH_PC_LOW)
	{
		// push PC low byte
		Stack_Push((BYTE)gSystem.Cpu.Reg.wPC);

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INTERRUPT_PUSH_STATUS;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INTERRUPT_PUSH_STATUS)
	{
		// push status value
		bStatusValue = gSystem.Cpu.Reg.bStatus;
		if(gSystem.Cpu.pInstructionAttributes->InstructionType == INSTRUCTION_TYPE_BRK)
		{
			// BRK instruction - push status value with break flag set
			bStatusValue |= CPU_STATUS_FLAG_BREAK;
		}
		Stack_Push(bStatusValue);

		// disable interrupts
		StatusFlag_Set(CPU_STATUS_FLAG_INTERRUPT_DISABLE);

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INTERRUPT_READ_PC_LOW;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INTERRUPT_READ_PC_LOW)
	{
		// read low PC byte
		Memory_Read8(gSystem.Cpu.wTempStateValue16, (BYTE*)&gSystem.Cpu.Reg.wPC);

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_INTERRUPT_READ_PC_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_INTERRUPT_READ_PC_HIGH)
	{
		// read high PC byte
		Memory_Read8(gSystem.Cpu.wTempStateValue16 + 1, (BYTE*)&gSystem.Cpu.Reg.wPC + 1);

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
