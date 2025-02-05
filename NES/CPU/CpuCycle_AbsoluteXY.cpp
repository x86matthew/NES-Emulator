#include "..\NES.h"

DWORD HandleInstructionCycle_AbsoluteXY()
{
	WORD wValue = 0;
	BYTE bValue = 0;

	if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_XY_BEGIN)
	{
		// read low byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)&gSystem.Cpu.wTempStateValue16);

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		// read next byte
		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_READ_HIGH;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_XY_READ_HIGH)
	{
		// read high byte
		Memory_Read8(gSystem.Cpu.Reg.wPC, (BYTE*)((BYTE*)&gSystem.Cpu.wTempStateValue16 + 1));

		// increase PC
		gSystem.Cpu.Reg.wPC++;

		// update address
		wValue = gSystem.Cpu.wTempStateValue16;
		if(gSystem.Cpu.pCurrInstruction->AddressMode == ADDRESS_MODE_ABSOLUTE_X)
		{
			// X mode
			wValue += gSystem.Cpu.Reg.bX;
		}
		else if(gSystem.Cpu.pCurrInstruction->AddressMode == ADDRESS_MODE_ABSOLUTE_Y)
		{
			// Y mode
			wValue += gSystem.Cpu.Reg.bY;
		}
		else
		{
			return 1;
		}

		// check for page-cross
		gSystem.Cpu.bTempStateValue8 = 0;
		if(CheckPageCross(gSystem.Cpu.wTempStateValue16, wValue) != 0)
		{
			// set page-cross flag
			gSystem.Cpu.bTempStateValue8 = 1;
		}

		// update address
		gSystem.Cpu.wTempStateValue16 = wValue;

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS)
	{
		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ)
		{
			// check if a page-cross occurred
			if(gSystem.Cpu.bTempStateValue8 == 0)
			{
				// read value
				Memory_Read8(gSystem.Cpu.wTempStateValue16, &bValue);

				// call common handler
				if(CallCommonInstructionHandler(bValue, NULL) != 0)
				{
					return 1;
				}

				// finished
				gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
			}
			else
			{
				// page-cross
				gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS_PAGE_CROSSED;
			}
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// read-modify-write should always be treated as though a page-cross occurred
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS_PAGE_CROSSED;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_WRITE)
		{
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_WRITE_ADDRESS;
		}
		else
		{
			return 1;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS_PAGE_CROSSED)
	{
		// read value
		Memory_Read8(gSystem.Cpu.wTempStateValue16, &bValue);

		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ)
		{
			// call common handler
			if(CallCommonInstructionHandler(bValue, NULL) != 0)
			{
				return 1;
			}

			// finished
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_COMPLETE;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// store original value to modify
			gSystem.Cpu.bTempStateValue8 = bValue;
			gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_MODIFY;
		}
		else
		{
			return 1;
		}
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_XY_MODIFY)
	{
		// write original value back to address (this emulates real 6502 behaviour)
		Memory_Write8(gSystem.Cpu.wTempStateValue16, gSystem.Cpu.bTempStateValue8);

		// call common handler
		if(CallCommonInstructionHandler(gSystem.Cpu.bTempStateValue8, &gSystem.Cpu.bTempStateValue8) != 0)
		{
			return 1;
		}

		gSystem.Cpu.InstructionState = INSTRUCTION_STATE_ABSOLUTE_XY_WRITE_ADDRESS;
	}
	else if(gSystem.Cpu.InstructionState == INSTRUCTION_STATE_ABSOLUTE_XY_WRITE_ADDRESS)
	{
		if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_READ_MODIFY_WRITE)
		{
			// retrieve modified value
			bValue = gSystem.Cpu.bTempStateValue8;
		}
		else if(gSystem.Cpu.pInstructionAttributes->bFlags & INSTRUCTION_FLAGS_WRITE)
		{
			// call common handler
			if(CallCommonInstructionHandler(0, &bValue) != 0)
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}

		// write value
		Memory_Write8(gSystem.Cpu.wTempStateValue16, bValue);

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
