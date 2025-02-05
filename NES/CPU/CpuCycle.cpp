#include "..\NES.h"

DWORD UpdateInstructionHandler(DWORD (*pInstructionCycleHandler)(), InstructionStateEnum InitialState)
{
	// store initial instruction state
	gSystem.Cpu.pInstructionCycleHandler = pInstructionCycleHandler;
	gSystem.Cpu.InstructionState = InitialState;

	return 0;
}

DWORD PrepareInstructionHandler(AddressModeEnum AddressMode, InstructionTypeEnum InstructionType)
{
	if(AddressMode == ADDRESS_MODE_IMPLIED)
	{
		// implied instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Implied, INSTRUCTION_STATE_IMPLIED_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_IMMEDIATE)
	{
		// immediate instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Immediate, INSTRUCTION_STATE_IMMEDIATE_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ABSOLUTE)
	{
		// absolute instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Absolute, INSTRUCTION_STATE_ABSOLUTE_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ABSOLUTE_X)
	{
		// absolute_x instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_AbsoluteXY, INSTRUCTION_STATE_ABSOLUTE_XY_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ABSOLUTE_Y)
	{
		// absolute_y instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_AbsoluteXY, INSTRUCTION_STATE_ABSOLUTE_XY_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_RELATIVE)
	{
		// relative instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Relative, INSTRUCTION_STATE_RELATIVE_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ZERO_PAGE)
	{
		// zero_page instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_ZeroPage, INSTRUCTION_STATE_ZERO_PAGE_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ZERO_PAGE_X)
	{
		// zero_page_x instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_ZeroPageXY, INSTRUCTION_STATE_ZERO_PAGE_XY_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ZERO_PAGE_Y)
	{
		// zero_page_y instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_ZeroPageXY, INSTRUCTION_STATE_ZERO_PAGE_XY_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_INDIRECT)
	{
		// indirect instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Indirect, INSTRUCTION_STATE_INDIRECT_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_INDIRECT_X)
	{
		// indirect_x instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_IndirectX, INSTRUCTION_STATE_INDIRECT_X_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_INDIRECT_Y)
	{
		// indirect_y instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_IndirectY, INSTRUCTION_STATE_INDIRECT_Y_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_ACCUMULATOR)
	{
		// accumulator instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Accumulator, INSTRUCTION_STATE_ACCUMULATOR_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_PUSH)
	{
		// push instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Push, INSTRUCTION_STATE_PUSH_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_POP)
	{
		// pop instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Pop, INSTRUCTION_STATE_POP_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_INTERRUPT)
	{
		// interrupt instruction handler
		UpdateInstructionHandler(HandleInstructionCycle_Interrupt, INSTRUCTION_STATE_INTERRUPT_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_DMA)
	{
		// DMA handler (used by PPU)
		UpdateInstructionHandler(HandleInstructionCycle_DMA, INSTRUCTION_STATE_DMA_BEGIN);
	}
	else if(AddressMode == ADDRESS_MODE_SPECIAL)
	{
		// special address mode - check specific instruction type
		if(InstructionType == INSTRUCTION_TYPE_JSR)
		{
			// JSR instruction handler
			UpdateInstructionHandler(HandleInstructionCycle_JSR, INSTRUCTION_STATE_JSR_BEGIN);
		}
		else if(InstructionType == INSTRUCTION_TYPE_RTI)
		{
			// RTI instruction handler
			UpdateInstructionHandler(HandleInstructionCycle_RTI, INSTRUCTION_STATE_RTI_BEGIN);
		}
		else if(InstructionType == INSTRUCTION_TYPE_RTS)
		{
			// RTS instruction handler
			UpdateInstructionHandler(HandleInstructionCycle_RTS, INSTRUCTION_STATE_RTS_BEGIN);
		}
		else if(InstructionType == INSTRUCTION_TYPE_STP)
		{
			// STP instruction handler
			UpdateInstructionHandler(HandleInstructionCycle_STP, INSTRUCTION_STATE_STP_BEGIN);
		}
		else
		{
			// not found
			return 1;
		}
	}
	else
	{
		// not found
		return 1;
	}

	return 0;
}

DWORD CycleCpu_CheckIrq(BYTE *pbIrqReady)
{
	BYTE bIrqReady = 0;

	if(gSystem.Cpu.bIrqPending_ApuDmc != 0 || gSystem.Cpu.bIrqPending_ApuFrame != 0)
	{
		// IRQ pending, check if interrupts are disabled
		if(gSystem.Cpu.bPendingInterruptDisableFlagUpdate != 0)
		{
			// current status flag value is pending, check stored value from previous instruction
			if(gSystem.Cpu.bPendingInterruptDisableFlagUpdate_OrigValue == 0)
			{
				// ready
				bIrqReady = 1;
			}
		}
		else
		{
			// check status flag
			if(StatusFlag_Get(CPU_STATUS_FLAG_INTERRUPT_DISABLE) == 0)
			{
				// ready
				bIrqReady = 1;
			}
		}
	}

	// reset flag for next instruction
	gSystem.Cpu.bPendingInterruptDisableFlagUpdate = 0;

	// store ready flag
	*pbIrqReady = bIrqReady;

	return 0;
}

DWORD CycleCpu()
{
	BYTE bOpcode = 0;
	BYTE bIrqReady = 0;

	if(gSystem.Cpu.pCurrInstruction == NULL)
	{
		// check IRQ
		CycleCpu_CheckIrq(&bIrqReady);

		if(gSystem.Cpu.bNmiPending != 0)
		{
			// NMI pending - get NMI placeholder instruction
			if(LookupInstruction_Placeholder(INSTRUCTION_TYPE_NMI, &gSystem.Cpu.pCurrInstruction, &gSystem.Cpu.pInstructionAttributes) != 0)
			{
				return 1;
			}

			// clear flag
			gSystem.Cpu.bNmiPending = 0;
		}
		else if(bIrqReady != 0)
		{
			// IRQ pending - get IRQ placeholder instruction
			if(LookupInstruction_Placeholder(INSTRUCTION_TYPE_IRQ, &gSystem.Cpu.pCurrInstruction, &gSystem.Cpu.pInstructionAttributes) != 0)
			{
				return 1;
			}
		}
		else if(gSystem.Cpu.bDmaPending != 0)
		{
			// DMA pending - get DMA placeholder instruction
			if(LookupInstruction_Placeholder(INSTRUCTION_TYPE_DMA, &gSystem.Cpu.pCurrInstruction, &gSystem.Cpu.pInstructionAttributes) != 0)
			{
				return 1;
			}

			// clear flag
			gSystem.Cpu.bDmaPending = 0;
		}
		else
		{
			// get next instruction
			Memory_Read8(gSystem.Cpu.Reg.wPC, &bOpcode);

			// increase PC
			gSystem.Cpu.Reg.wPC++;

			// lookup instruction from opcode
			if(LookupInstruction(bOpcode, &gSystem.Cpu.pCurrInstruction, &gSystem.Cpu.pInstructionAttributes) != 0)
			{
				// invalid instruction
				printf("Error: Invalid instruction: 0x%02X\n", bOpcode);
				return 1;
			}
		}

		// prepare initial instruction handler state
		if(PrepareInstructionHandler(gSystem.Cpu.pCurrInstruction->AddressMode, gSystem.Cpu.pInstructionAttributes->InstructionType) != 0)
		{
			return 1;
		}
	}
	else
	{
		// execute next cycle for current instruction
		if(gSystem.Cpu.pInstructionCycleHandler() != 0)
		{
			return 1;
		}

		// check if the instruction is complete
		if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_COMPLETE)
		{
			// clear current instruction
			gSystem.Cpu.pCurrInstruction = NULL;
		}
	}

	// increase 8-bit cycle counter - this is currently only used by DMA to determine if the current cycle is odd/even
	gSystem.Cpu.bCpuCounter++;

	return 0;
}
