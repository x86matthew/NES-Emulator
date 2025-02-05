#include "..\NES.h"

DWORD HandleInstructionCycle_Implied()
{
	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_IMPLIED_BEGIN)
	{
		if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_CLC)
		{
			// CLC instruction
			StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_CLD)
		{
			// CLD instruction
			StatusFlag_Clear(CPU_STATUS_FLAG_DECIMAL);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_CLI)
		{
			// CLI instruction
			StatusFlag_SetPendingInterruptDisableFlagUpdate();
			StatusFlag_Clear(CPU_STATUS_FLAG_INTERRUPT_DISABLE);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_CLV)
		{
			// CLV instruction
			StatusFlag_Clear(CPU_STATUS_FLAG_OVERFLOW);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_DEX)
		{
			// DEX instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bX - 1, &gSystem.Cpu.Reg.bX);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_DEY)
		{
			// DEY instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bY - 1, &gSystem.Cpu.Reg.bY);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_INX)
		{
			// INX instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bX + 1, &gSystem.Cpu.Reg.bX);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_INY)
		{
			// INY instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bY + 1, &gSystem.Cpu.Reg.bY);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_NOP)
		{
			// NOP instruction
			CommonInstructionHandler_NOP(0, NULL);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_SEC)
		{
			// SEC instruction
			StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_SED)
		{
			// SED instruction
			StatusFlag_Set(CPU_STATUS_FLAG_DECIMAL);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_SEI)
		{
			// SEI instruction
			StatusFlag_SetPendingInterruptDisableFlagUpdate();
			StatusFlag_Set(CPU_STATUS_FLAG_INTERRUPT_DISABLE);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_TAX)
		{
			// TAX instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bA, &gSystem.Cpu.Reg.bX);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_TAY)
		{
			// TAY instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bA, &gSystem.Cpu.Reg.bY);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_TSX)
		{
			// TSX instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bSP, &gSystem.Cpu.Reg.bX);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_TXA)
		{
			// TXA instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bX, &gSystem.Cpu.Reg.bA);
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_TXS)
		{
			// TXS instruction (no flags to update)
			gSystem.Cpu.Reg.bSP = gSystem.Cpu.Reg.bX;
		}
		else if(gSystem.Cpu.pCurrInstruction->InstructionType == INSTRUCTION_TYPE_TYA)
		{
			// TYA instruction
			InstructionUtils_Load(gSystem.Cpu.Reg.bY, &gSystem.Cpu.Reg.bA);
		}
		else
		{
			// invalid instruction
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
