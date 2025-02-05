#include "NES.h"

DWORD UpdateKeyState(BYTE bVirtualKey, BYTE bKeyDown)
{
	// check if the key is mapped
	for(DWORD i = 0; i < INPUT_KEY_COUNT; i++)
	{
		if(gSystem.Input.KeyMappingList[i].bVirtualKey == bVirtualKey)
		{
			// set state
			gSystem.Input.KeyMappingList[i].bKeyDown = bKeyDown;
		}
	}

	return 0;
}

DWORD InputMemoryMappedRegisters_Read(WORD wAddress, BYTE *pbValue)
{
	BYTE bValue = 0;

	if(wAddress == INPUT_REGISTER_CONTROLLER_1_STATUS)
	{
		// get controller 1 state
		if(gSystem.Input.bPollEnabled == 0)
		{
			// read next stored key state
			bValue = gSystem.Input.bStoredValue & 1;
			gSystem.Input.bStoredValue >>= 1;

			// set high bit to 1
			gSystem.Input.bStoredValue |= 0x80;
		}
		else
		{
			// polling is still enabled - return the current state of first key in the sequence (A button)
			bValue = 0;
			if(gSystem.Input.KeyMappingList[0].bKeyDown != 0)
			{
				bValue = 1;
			}
		}
	}
	else if(wAddress == INPUT_REGISTER_CONTROLLER_2_STATUS)
	{
		// controller 2 not connected - set to 0
		bValue = 0;
	}
	else
	{
		return 1;
	}

	*pbValue = bValue;

	return 0;
}

DWORD InputMemoryMappedRegisters_Write(WORD wAddress, BYTE bValue)
{
	BYTE bCurrKeyFlag = 0;

	if(wAddress == INPUT_REGISTER_CTRL)
	{
		// enable/disable input controller polling
		gSystem.Input.bPollEnabled = 0;
		if(bValue & 1)
		{
			// input polling enabled
			gSystem.Input.bPollEnabled = 1;
		}
		else
		{
			// stop polling - store current key state
			gSystem.Input.bStoredValue = 0;
			bCurrKeyFlag = 1;
			for(DWORD i = 0; i < INPUT_KEY_COUNT; i++)
			{
				// get current key in sequence
				if(gSystem.Input.KeyMappingList[i].bKeyDown != 0)
				{
					// key down - set current bit
					gSystem.Input.bStoredValue |= bCurrKeyFlag;
				}

				// shift left
				bCurrKeyFlag <<= 1;
			}
		}
	}
	else
	{
		return 1;
	}
	
	return 0;
}

DWORD InitialiseInputKeyMappings()
{
	// A button
	gSystem.Input.KeyMappingList[0].bVirtualKey = 'A';

	// B button
	gSystem.Input.KeyMappingList[1].bVirtualKey = 'X';

	// select button
	gSystem.Input.KeyMappingList[2].bVirtualKey = VK_SPACE;

	// start button
	gSystem.Input.KeyMappingList[3].bVirtualKey = VK_RETURN;

	// up button
	gSystem.Input.KeyMappingList[4].bVirtualKey = VK_UP;

	// down button
	gSystem.Input.KeyMappingList[5].bVirtualKey = VK_DOWN;

	// left button
	gSystem.Input.KeyMappingList[6].bVirtualKey = VK_LEFT;

	// right button
	gSystem.Input.KeyMappingList[7].bVirtualKey = VK_RIGHT;

	return 0;
}
