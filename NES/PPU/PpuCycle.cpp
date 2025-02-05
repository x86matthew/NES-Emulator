#include "..\NES.h"

DWORD CyclePpu_ScanLine(BYTE bPreRenderLine)
{
	BYTE bCurrX = 0;
	BYTE bCurrY = 0;

	if(bPreRenderLine != 0)
	{
		// pre-render scan line
		if(gSystem.Ppu.wDotIndex == 1)
		{
			// clear v-blank flag
			PpuStatusFlag_Clear(PPU_STATUS_FLAG_VBLANK);

			// clear sprite overflow flag
			PpuStatusFlag_Clear(PPU_STATUS_FLAG_SPRITE_OVERFLOW);

			// clear sprite 0 hit flag
			PpuStatusFlag_Clear(PPU_STATUS_FLAG_SPRITE_0_HIT);
			gSystem.Ppu.bFrameSpriteZeroHit = 0;
		}
	}

	// check if background rendering is enabled
	gSystem.Ppu.pCurrPixelBackground = NULL;
	if(PpuMaskFlag_Get(PPU_MASK_FLAG_BACKGROUND_RENDERING_ENABLED) != 0)
	{
		// process background rendering
		if(PpuProcessScanLine_Background(bPreRenderLine) != 0)
		{
			return 1;
		}
	}

	// check if sprite rendering is enabled
	gSystem.Ppu.pCurrPixelSprite = NULL;
	if(PpuMaskFlag_Get(PPU_MASK_FLAG_SPRITE_RENDERING_ENABLED) != 0)
	{
		// process sprite rendering
		if(PpuProcessScanLine_Sprites(bPreRenderLine) != 0)
		{
			return 1;
		}
	}

	if(bPreRenderLine == 0)
	{
		// visible scan-line, cycles 1-256 render pixels
		if(gSystem.Ppu.wDotIndex >= 1 && gSystem.Ppu.wDotIndex <= 256)
		{
			// render current pixel
			bCurrX = gSystem.Ppu.wDotIndex - 1;
			bCurrY = (BYTE)gSystem.Ppu.wScanLineIndex;
			if(PpuRenderPixel(bCurrX, bCurrY) != 0)
			{
				return 1;
			}
		}
	}

	return 0;
}

DWORD CyclePpu_VBlankLine()
{
	if(gSystem.Ppu.wScanLineIndex == (VISIBLE_SCAN_LINE_COUNT + 1))
	{
		// first v-blank line
		if(gSystem.Ppu.wDotIndex == 1)
		{
			// set v-blank flag
			PpuStatusFlag_Set(PPU_STATUS_FLAG_VBLANK);

			// check if NMI control flag is enabled
			if(PpuControlFlag_Get(PPU_CONTROL_FLAG_NMI_ENABLED) != 0)
			{
				// set NMI
				gSystem.Cpu.bNmiPending = 1;
			}
		}
	}

	return 0;
}

DWORD CyclePpu()
{
	BYTE bEndOfLine = 0;
	BYTE bSkipFirstIdle = 0;

	if(gSystem.Ppu.wScanLineIndex == (gSystem.RegionSpecificSettings.dwPpuTotalScanLineCount - 1))
	{
		// pre-render scan line
		if(CyclePpu_ScanLine(1) != 0)
		{
			return 1;
		}
	}
	else if(gSystem.Ppu.wScanLineIndex < VISIBLE_SCAN_LINE_COUNT)
	{
		// visible line
		if(CyclePpu_ScanLine(0) != 0)
		{
			return 1;
		}
	}
	else if(gSystem.Ppu.wScanLineIndex == VISIBLE_SCAN_LINE_COUNT)
	{
		// post-render scan line - do nothing
	}
	else
	{
		// v-blank line
		if(CyclePpu_VBlankLine() != 0)
		{
			return 1;
		}
	}

	// increase dot index
	gSystem.Ppu.wDotIndex++;
	if(gSystem.Ppu.wDotIndex == DOTS_PER_SCAN_LINE)
	{
		// next scan line
		bEndOfLine = 1;

		// the first idle cycle should be skipped on the pre-render line for every odd frame (NTSC only)
		if(gSystem.RegionSpecificSettings.bPpuSkipFirstIdleOddFramePreRender != 0)
		{
			// check if this is the pre-render line
			if(gSystem.Ppu.wScanLineIndex == (gSystem.RegionSpecificSettings.dwPpuTotalScanLineCount - 1))
			{
				// check if this is an odd frame
				if((gSystem.Ppu.bFrameIndex % 2) != 0)
				{
					// odd frame - one less dot
					bSkipFirstIdle = 1;
				}
			}
		}
	}

	// check if the end of the current scan line has been reached
	if(bEndOfLine != 0)
	{
		// next line
		gSystem.Ppu.wDotIndex = 0;
		gSystem.Ppu.wScanLineIndex++;
		if(gSystem.Ppu.wScanLineIndex >= gSystem.RegionSpecificSettings.dwPpuTotalScanLineCount)
		{
			// end of frame
			DisplayWindow_FrameReady();

			// initialise next frame
			gSystem.Ppu.wScanLineIndex = 0;
			gSystem.Ppu.bFrameIndex++;

			if(bSkipFirstIdle != 0)
			{
				// skip first idle cycle
				gSystem.Ppu.wDotIndex++;
			}
		}
	}

	return 0;
}
