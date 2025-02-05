#include "..\NES.h"

DWORD Stack_Push(BYTE bValue)
{
	// push value to stack
	Memory_Write8(STACK_BASE + gSystem.Cpu.Reg.bSP, bValue);
	gSystem.Cpu.Reg.bSP--;

	return 0;
}

DWORD Stack_Pop(BYTE *pbValue)
{
	// pop value from stack
	gSystem.Cpu.Reg.bSP++;
	Memory_Read8(STACK_BASE + gSystem.Cpu.Reg.bSP, pbValue);

	return 0;
}

DWORD StatusFlag_Set(BYTE bFlag)
{
	// set flag value
	gSystem.Cpu.Reg.bStatus |= bFlag;

	return 0;
}

DWORD StatusFlag_Clear(BYTE bFlag)
{
	// clear flag value
	gSystem.Cpu.Reg.bStatus &= ~bFlag;

	return 0;
}

BYTE StatusFlag_Get(BYTE bFlag)
{
	// get flag value
	if(gSystem.Cpu.Reg.bStatus & bFlag)
	{
		// flag is set
		return 1;
	}

	return 0;
}

DWORD StatusFlag_SetAuto_Zero(BYTE bValue)
{
	// set zero flag from value
	StatusFlag_Clear(CPU_STATUS_FLAG_ZERO);
	if(bValue == 0)
	{
		StatusFlag_Set(CPU_STATUS_FLAG_ZERO);
	}

	return 0;
}

DWORD StatusFlag_SetAuto_Negative(BYTE bValue)
{
	// set negative flag from value
	StatusFlag_Clear(CPU_STATUS_FLAG_NEGATIVE);
	if(bValue & 0x80)
	{
		StatusFlag_Set(CPU_STATUS_FLAG_NEGATIVE);
	}

	return 0;
}

DWORD StatusFlag_FixAfterRestore()
{
	// discard break flag
	StatusFlag_Clear(CPU_STATUS_FLAG_BREAK);

	// set always-on flag
	StatusFlag_Set(CPU_STATUS_FLAG_ALWAYS_ON);

	return 0;
}

DWORD StatusFlag_SetPendingInterruptDisableFlagUpdate()
{
	// delay the effect of the interrupt-disable flag by 1 instruction - store the current value
	gSystem.Cpu.bPendingInterruptDisableFlagUpdate = 1;
	gSystem.Cpu.bPendingInterruptDisableFlagUpdate_OrigValue = StatusFlag_Get(CPU_STATUS_FLAG_INTERRUPT_DISABLE);

	return 0;
}

DWORD CallCommonInstructionHandler(BYTE bIn, BYTE *pbOut)
{
	BYTE bOut = 0;

	// ensure this instruction has a common handler function
	if(gSystem.Cpu.pInstructionAttributes->pCommonInstructionHandler == NULL)
	{
		return 1;
	}

	// call function
	if(gSystem.Cpu.pInstructionAttributes->pCommonInstructionHandler(bIn, &bOut) != 0)
	{
		return 1;
	}

	// copy output value if applicable
	if(pbOut != NULL)
	{
		*pbOut = bOut;
	}

	return 0;
}

DWORD InstructionUtils_Load(BYTE bValue, BYTE *pbDestination)
{
	// set zero and negative flags
	StatusFlag_SetAuto_Zero(bValue);
	StatusFlag_SetAuto_Negative(bValue);

	// copy value
	*pbDestination = bValue;

	return 0;
}

DWORD InstructionUtils_Compare(BYTE bValue1, BYTE bValue2)
{
	BYTE bResult = 0;

	// calculate result
	bResult = bValue1 - bValue2;

	StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
	if(bValue1 >= bValue2)
	{
		// set carry flag
		StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
	}

	// set zero and negative flags
	StatusFlag_SetAuto_Zero(bResult);
	StatusFlag_SetAuto_Negative(bResult);

	return 0;
}

DWORD CheckPageCross(WORD wAddress1, WORD wAddress2)
{
	// check if the high byte is different
	if((wAddress1 & 0xFF00) != (wAddress2 & 0xFF00))
	{
		// page-cross
		return 1;
	}

	return 0;
}
