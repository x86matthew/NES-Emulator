#include "..\NES.h"

DWORD HandleInstructionCycle_Relative()
{
	BYTE bRelativeOffset = 0;
	BYTE bConditionMet = 0;
	WORD wTargetAddress = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RELATIVE_BEGIN)
	{
		// read relative value
		Memory_Read8(gSystem.Cpu.Reg.wPC, &bRelativeOffset);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		// check branch type
		if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BCC)
		{
			// BCC instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_CARRY) == 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BCS)
		{
			// BCS instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_CARRY) != 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BEQ)
		{
			// BEQ instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_ZERO) != 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BMI)
		{
			// BMI instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_NEGATIVE) != 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BNE)
		{
			// BNE instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_ZERO) == 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BPL)
		{
			// BPL instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_NEGATIVE) == 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BVC)
		{
			// BVC instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_OVERFLOW) == 0)
			{
				bConditionMet = 1;
			}
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_BVS)
		{
			// BVS instruction
			if(StatusFlag_Get(CPU_STATUS_FLAG_OVERFLOW) != 0)
			{
				bConditionMet = 1;
			}
		}
		else
		{
			// invalid instruction
			return 1;
		}

		if(bConditionMet == 0)
		{
			// condition not met - finished
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
		}
		else
		{
			// condition met
			gSystem.Cpu.bTempStateValue8 = bRelativeOffset;
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RELATIVE_CONDITION_MET;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RELATIVE_CONDITION_MET)
	{
		// calculate branch target address
		wTargetAddress = gSystem.Cpu.Reg.wPC + (short)((char)gSystem.Cpu.bTempStateValue8);

		// check if the destination address crosses into a different page
		if(CheckPageCross(gSystem.Cpu.Reg.wPC, wTargetAddress) != 0)
		{
			// page-cross
			gSystem.Cpu.wTempStateValue16 = wTargetAddress;
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_RELATIVE_PAGE_CROSSED;
		}
		else
		{
			// update PC
			gSystem.Cpu.Reg.wPC = wTargetAddress;

			// finished
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_RELATIVE_PAGE_CROSSED)
	{
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
