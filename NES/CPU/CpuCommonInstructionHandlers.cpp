#include "..\NES.h"

DWORD CommonInstructionHandler_BIT(BYTE bIn, BYTE *pbOut)
{
	StatusFlag_Clear(CPU_STATUS_FLAG_ZERO);
	if((gSystem.Cpu.Reg.bA & bIn) == 0)
	{
		// set zero flag
		StatusFlag_Set(CPU_STATUS_FLAG_ZERO);
	}

	StatusFlag_Clear(CPU_STATUS_FLAG_OVERFLOW);
	if(bIn & 0x40)
	{
		// set overflow flag
		StatusFlag_Set(CPU_STATUS_FLAG_OVERFLOW);
	}

	StatusFlag_Clear(CPU_STATUS_FLAG_NEGATIVE);
	if(bIn & 0x80)
	{
		// set negative flag
		StatusFlag_Set(CPU_STATUS_FLAG_NEGATIVE);
	}

	return 0;
}

DWORD CommonInstructionHandler_LDA(BYTE bIn, BYTE *pbOut)
{
	// load A
	InstructionUtils_Load(bIn, &gSystem.Cpu.Reg.bA);

	return 0;
}

DWORD CommonInstructionHandler_LDX(BYTE bIn, BYTE *pbOut)
{
	// load X
	InstructionUtils_Load(bIn, &gSystem.Cpu.Reg.bX);

	return 0;
}

DWORD CommonInstructionHandler_LDY(BYTE bIn, BYTE *pbOut)
{
	// load Y
	InstructionUtils_Load(bIn, &gSystem.Cpu.Reg.bY);

	return 0;
}

DWORD CommonInstructionHandler_STA(BYTE bIn, BYTE *pbOut)
{
	// store A
	*pbOut = gSystem.Cpu.Reg.bA;

	return 0;
}

DWORD CommonInstructionHandler_STX(BYTE bIn, BYTE *pbOut)
{
	// store X
	*pbOut = gSystem.Cpu.Reg.bX;

	return 0;
}

DWORD CommonInstructionHandler_STY(BYTE bIn, BYTE *pbOut)
{
	// store Y
	*pbOut = gSystem.Cpu.Reg.bY;

	return 0;
}

DWORD CommonInstructionHandler_DEC(BYTE bIn, BYTE *pbOut)
{
	// decrement
	InstructionUtils_Load(bIn - 1, pbOut);

	return 0;
}

DWORD CommonInstructionHandler_INC(BYTE bIn, BYTE *pbOut)
{
	// increment
	InstructionUtils_Load(bIn + 1, pbOut);

	return 0;
}

DWORD CommonInstructionHandler_ROL(BYTE bIn, BYTE *pbOut)
{
	BYTE bResult = 0;
	BYTE bSetLowBit = 0;

	// store carry flag value for later
	bSetLowBit = StatusFlag_Get(CPU_STATUS_FLAG_CARRY);

	StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
	if(bIn & 0x80)
	{
		// set carry flag
		StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
	}

	// shift left
	bResult = bIn << 1;
	if(bSetLowBit != 0)
	{
		// carry flag was set, set bit 0
		bResult |= 1;
	}

	// load value
	InstructionUtils_Load(bResult, pbOut);

	return 0;
}

DWORD CommonInstructionHandler_ROR(BYTE bIn, BYTE *pbOut)
{
	BYTE bResult = 0;
	BYTE bSetHighBit = 0;

	// store carry flag value for later
	bSetHighBit = StatusFlag_Get(CPU_STATUS_FLAG_CARRY);

	StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
	if(bIn & 1)
	{
		// set carry flag
		StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
	}

	// shift right
	bResult = bIn >> 1;
	if(bSetHighBit != 0)
	{
		// carry flag was set, set bit 7
		bResult |= 0x80;
	}

	// load value
	InstructionUtils_Load(bResult, pbOut);

	return 0;
}

DWORD CommonInstructionHandler_LSR(BYTE bIn, BYTE *pbOut)
{
	BYTE bResult = 0;

	// copy bit 0 into the carry flag
	StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
	if(bIn & 1)
	{
		StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
	}

	// shift right
	bResult = bIn >> 1;

	// load value
	InstructionUtils_Load(bResult, pbOut);

	return 0;
}

DWORD CommonInstructionHandler_ASL(BYTE bIn, BYTE *pbOut)
{
	BYTE bResult = 0;

	// copy bit 7 into the carry flag
	StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
	if(bIn & 0x80)
	{
		StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
	}

	// shift left
	bResult = bIn << 1;

	// load value
	InstructionUtils_Load(bResult, pbOut);

	return 0;
}

DWORD CommonInstructionHandler_AND(BYTE bIn, BYTE *pbOut)
{
	// bitwise and
	InstructionUtils_Load(gSystem.Cpu.Reg.bA & bIn, &gSystem.Cpu.Reg.bA);

	return 0;
}

DWORD CommonInstructionHandler_EOR(BYTE bIn, BYTE *pbOut)
{
	// bitwise xor
	InstructionUtils_Load(gSystem.Cpu.Reg.bA ^ bIn, &gSystem.Cpu.Reg.bA);

	return 0;
}

DWORD CommonInstructionHandler_ORA(BYTE bIn, BYTE *pbOut)
{
	// bitwise or
	InstructionUtils_Load(gSystem.Cpu.Reg.bA | bIn, &gSystem.Cpu.Reg.bA);

	return 0;
}

DWORD CommonInstructionHandler_ADC(BYTE bIn, BYTE *pbOut)
{
	BYTE bResult = 0;
	BYTE bSetCarry = 0;

	// add with carry
	bResult = gSystem.Cpu.Reg.bA + bIn;
	if(gSystem.Cpu.Reg.bA > bResult)
	{
		// overflowed
		bSetCarry = 1;
	}

	if(StatusFlag_Get(CPU_STATUS_FLAG_CARRY) != 0)
	{
		// carry flag set - add 1
		bResult++;
		if(bResult == 0)
		{
			// overflowed
			bSetCarry = 1;
		}
	}

	// check for unsigned overflow
	StatusFlag_Clear(CPU_STATUS_FLAG_CARRY);
	if(bSetCarry != 0)
	{
		// set carry flag
		StatusFlag_Set(CPU_STATUS_FLAG_CARRY);
	}

	// check for signed overflow
	StatusFlag_Clear(CPU_STATUS_FLAG_OVERFLOW);
	if(GET_BIT(gSystem.Cpu.Reg.bA, 7) == GET_BIT(bIn, 7))
	{
		if(GET_BIT(gSystem.Cpu.Reg.bA, 7) != GET_BIT(bResult, 7))
		{
			// set overflow flag
			StatusFlag_Set(CPU_STATUS_FLAG_OVERFLOW);
		}
	}

	// load value
	InstructionUtils_Load(bResult, &gSystem.Cpu.Reg.bA);

	return 0;
}

DWORD CommonInstructionHandler_SBC(BYTE bIn, BYTE *pbOut)
{
	// subtract with carry
	CommonInstructionHandler_ADC(~bIn, NULL);

	return 0;
}

DWORD CommonInstructionHandler_CMP(BYTE bIn, BYTE *pbOut)
{
	// compare A
	InstructionUtils_Compare(gSystem.Cpu.Reg.bA, bIn);

	return 0;
}

DWORD CommonInstructionHandler_CPX(BYTE bIn, BYTE *pbOut)
{
	// compare X
	InstructionUtils_Compare(gSystem.Cpu.Reg.bX, bIn);

	return 0;
}

DWORD CommonInstructionHandler_CPY(BYTE bIn, BYTE *pbOut)
{
	// compare Y
	InstructionUtils_Compare(gSystem.Cpu.Reg.bY, bIn);

	return 0;
}

DWORD CommonInstructionHandler_DCP(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_DEC(bIn, pbOut);
	CommonInstructionHandler_CMP(*pbOut, NULL);

	return 0;
}

DWORD CommonInstructionHandler_ISC(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_INC(bIn, pbOut);
	CommonInstructionHandler_SBC(*pbOut, NULL);

	return 0;
}

DWORD CommonInstructionHandler_LAX(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_LDA(bIn, NULL);
	CommonInstructionHandler_LDX(bIn, NULL);

	return 0;
}

DWORD CommonInstructionHandler_RLA(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_ROL(bIn, pbOut);
	CommonInstructionHandler_AND(*pbOut, NULL);

	return 0;
}

DWORD CommonInstructionHandler_RRA(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_ROR(bIn, pbOut);
	CommonInstructionHandler_ADC(*pbOut, NULL);

	return 0;
}

DWORD CommonInstructionHandler_SAX(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	*pbOut = gSystem.Cpu.Reg.bA & gSystem.Cpu.Reg.bX;

	return 0;
}

DWORD CommonInstructionHandler_SLO(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_ASL(bIn, pbOut);
	CommonInstructionHandler_ORA(*pbOut, NULL);

	return 0;
}

DWORD CommonInstructionHandler_SRE(BYTE bIn, BYTE *pbOut)
{
	// undocumented instruction
	CommonInstructionHandler_LSR(bIn, pbOut);
	CommonInstructionHandler_EOR(*pbOut, NULL);

	return 0;
}

DWORD CommonInstructionHandler_NOP(BYTE bIn, BYTE *pbOut)
{
	// do nothing
	return 0;
}
