#include "..\NES.h"

DWORD PpuRenderPixel_SetValue(BYTE bX, BYTE bY, SystemPaletteEntryStruct *pCurrPixel)
{
	BYTE bRed = 0;
	BYTE bGreen = 0;
	BYTE bBlue = 0;
	BYTE bReducedRed = 0;
	BYTE bReducedGreen = 0;
	BYTE bReducedBlue = 0;

	// set initial values
	bRed = pCurrPixel->bRed;
	bGreen = pCurrPixel->bGreen;
	bBlue = pCurrPixel->bBlue;

	// calculate reduced values (25% reduction)
	bReducedRed = bRed - (bRed / 4);
	bReducedGreen = bGreen - (bGreen / 4);
	bReducedBlue = bBlue - (bBlue / 4);

	// check for red emphasis
	if(PpuMaskFlag_Get(PPU_MASK_FLAG_RED_EMPHASIS) != 0)
	{
		bGreen = bReducedGreen;
		bBlue = bReducedBlue;
	}

	// check for green emphasis
	if(PpuMaskFlag_Get(PPU_MASK_FLAG_GREEN_EMPHASIS) != 0)
	{
		bRed = bReducedRed;
		bBlue = bReducedBlue;
	}

	// check for blue emphasis
	if(PpuMaskFlag_Get(PPU_MASK_FLAG_BLUE_EMPHASIS) != 0)
	{
		bRed = bReducedRed;
		bGreen = bReducedGreen;
	}

	// draw pixel on screen
	DisplayWindow_SetPixel(bX, bY, bRed, bGreen, bBlue);

	return 0;
}

DWORD PpuRenderPixel(BYTE bCurrX, BYTE bCurrY)
{
	SystemPaletteEntryStruct *pCurrPixel = NULL;

	if(bCurrX < 8)
	{
		if(PpuMaskFlag_Get(PPU_MASK_FLAG_BACKGROUND_LEFT_8_VISIBLE) == 0)
		{
			// background hidden in the first 8 pixels
			gSystem.Ppu.pCurrPixelBackground = NULL;
		}

		if(PpuMaskFlag_Get(PPU_MASK_FLAG_SPRITE_LEFT_8_VISIBLE) == 0)
		{
			// sprites hidden in the first 8 pixels
			gSystem.Ppu.pCurrPixelSprite = NULL;
		}
	}

	// check priority - sprite, background, or transparent
	if(gSystem.Ppu.pCurrPixelBackground != NULL && gSystem.Ppu.pCurrPixelSprite != NULL)
	{
		// both sprite and background pixels selected
		if(gSystem.Ppu.bCurrPixelSpriteBehindBackground != 0)
		{
			pCurrPixel = gSystem.Ppu.pCurrPixelBackground;
		}
		else
		{
			pCurrPixel = gSystem.Ppu.pCurrPixelSprite;
		}

		// check if this is sprite 0
		if(gSystem.Ppu.bCurrPixelSpriteIndex == 0)
		{
			if(gSystem.Ppu.bFrameSpriteZeroHit == 0)
			{
				// set sprite 0 hit flag
				PpuStatusFlag_Set(PPU_STATUS_FLAG_SPRITE_0_HIT);

				// only set the flag once per frame
				gSystem.Ppu.bFrameSpriteZeroHit = 1;
			}
		}
	}
	else if(gSystem.Ppu.pCurrPixelSprite != NULL)
	{
		// sprite
		pCurrPixel = gSystem.Ppu.pCurrPixelSprite;
	}
	else if(gSystem.Ppu.pCurrPixelBackground != NULL)
	{
		// background
		pCurrPixel = gSystem.Ppu.pCurrPixelBackground;
	}
	else
	{
		// transparent pixel - use default value
		pCurrPixel = GetSystemColourFromPalette(0, 0);
		if(pCurrPixel == NULL)
		{
			return 1;
		}
	}

	// set pixel value
	if(PpuRenderPixel_SetValue(bCurrX, bCurrY, pCurrPixel) != 0)
	{
		return 1;
	}

	return 0;
}
