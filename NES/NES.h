#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <windows.h>

#define GET_BIT(VALUE, BIT) (((VALUE) >> (BIT)) & 1)
#define ELEMENT_COUNT(X) (sizeof(X) / sizeof(X[0]))
#define ERROR_CLEAN_UP(X) bError = X; goto CLEAN_UP;

// NTSC region-specific values
#define MASTER_CLOCK_SPEED_HZ_NTSC 21477272
#define MASTER_CYCLES_PER_CPU_CYCLE_NTSC 12
#define MASTER_CYCLES_PER_PPU_CYCLE_NTSC 4
#define MASTER_CYCLES_PER_APU_CYCLE_NTSC 24
#define APU_QUARTER_FRAME_CYCLE_COUNT_NTSC 3728
#define APU_FULL_FRAME_CYCLE_COUNT_4_STEP_NTSC 14915
#define APU_FULL_FRAME_CYCLE_COUNT_5_STEP_NTSC 18641
#define PPU_TOTAL_SCAN_LINE_COUNT_NTSC 262

// PAL region-specific values
#define MASTER_CLOCK_SPEED_HZ_PAL 26601712
#define MASTER_CYCLES_PER_CPU_CYCLE_PAL 16
#define MASTER_CYCLES_PER_PPU_CYCLE_PAL 5
#define MASTER_CYCLES_PER_APU_CYCLE_PAL 32
#define APU_QUARTER_FRAME_CYCLE_COUNT_PAL 4156
#define APU_FULL_FRAME_CYCLE_COUNT_4_STEP_PAL 16627
#define APU_FULL_FRAME_CYCLE_COUNT_5_STEP_PAL 20783
#define PPU_TOTAL_SCAN_LINE_COUNT_PAL 312

// cpu status flags
#define CPU_STATUS_FLAG_CARRY 0x1
#define CPU_STATUS_FLAG_ZERO 0x2
#define CPU_STATUS_FLAG_INTERRUPT_DISABLE 0x4
#define CPU_STATUS_FLAG_DECIMAL 0x8
#define CPU_STATUS_FLAG_BREAK 0x10
#define CPU_STATUS_FLAG_ALWAYS_ON 0x20
#define CPU_STATUS_FLAG_OVERFLOW 0x40
#define CPU_STATUS_FLAG_NEGATIVE 0x80

// cpu stack
#define STACK_BASE 0x100
#define INITIAL_STACK_PTR 0xFD

// cpu interrupt vector addresses
#define INTERRUPT_VECTOR_NMI 0xFFFA
#define INTERRUPT_VECTOR_RESET 0xFFFC
#define INTERRUPT_VECTOR_IRQ 0xFFFE

// cpu instruction flags
#define INSTRUCTION_FLAGS_READ 0x1
#define INSTRUCTION_FLAGS_WRITE 0x2
#define INSTRUCTION_FLAGS_READ_MODIFY_WRITE 0x4

// memory-mapped register addresses
#define PPU_REGISTER_PPUCTRL 0x2000
#define PPU_REGISTER_PPUMASK 0x2001
#define PPU_REGISTER_PPUSTATUS 0x2002
#define PPU_REGISTER_OAMADDR 0x2003
#define PPU_REGISTER_OAMDATA 0x2004
#define PPU_REGISTER_PPUSCROLL 0x2005
#define PPU_REGISTER_PPUADDR 0x2006
#define PPU_REGISTER_PPUDATA 0x2007
#define PPU_REGISTER_OAMDMA 0x4014
#define APU_REGISTER_SQUARE_WAVE_1_CTRL1 0x4000
#define APU_REGISTER_SQUARE_WAVE_1_CTRL2 0x4001
#define APU_REGISTER_SQUARE_WAVE_1_CTRL3 0x4002
#define APU_REGISTER_SQUARE_WAVE_1_CTRL4 0x4003
#define APU_REGISTER_SQUARE_WAVE_2_CTRL1 0x4004
#define APU_REGISTER_SQUARE_WAVE_2_CTRL2 0x4005
#define APU_REGISTER_SQUARE_WAVE_2_CTRL3 0x4006
#define APU_REGISTER_SQUARE_WAVE_2_CTRL4 0x4007
#define APU_REGISTER_TRIANGLE_WAVE_CTRL1 0x4008
#define APU_REGISTER_TRIANGLE_WAVE_CTRL2 0x400A
#define APU_REGISTER_TRIANGLE_WAVE_CTRL3 0x400B
#define APU_REGISTER_NOISE_CTRL1 0x400C
#define APU_REGISTER_NOISE_CTRL2 0x400E
#define APU_REGISTER_NOISE_CTRL3 0x400F
#define APU_REGISTER_DELTA_MODULATION_CTRL1 0x4010
#define APU_REGISTER_DELTA_MODULATION_CTRL2 0x4011
#define APU_REGISTER_DELTA_MODULATION_CTRL3 0x4012
#define APU_REGISTER_DELTA_MODULATION_CTRL4 0x4013
#define APU_REGISTER_STATUS 0x4015
#define APU_REGISTER_FRAME_COUNTER 0x4017
#define INPUT_REGISTER_CTRL 0x4016
#define INPUT_REGISTER_CONTROLLER_1_STATUS 0x4016
#define INPUT_REGISTER_CONTROLLER_2_STATUS 0x4017

// PPUSTATUS register flags
#define PPU_STATUS_FLAG_SPRITE_OVERFLOW 0x20
#define PPU_STATUS_FLAG_SPRITE_0_HIT 0x40
#define PPU_STATUS_FLAG_VBLANK 0x80

// PPUCTRL register flags
#define PPU_CONTROL_FLAG_ADDR_INCREASE_32 0x4
#define PPU_CONTROL_FLAG_SPRITE_PATTERN_TABLE_MODE 0x8
#define PPU_CONTROL_FLAG_BACKGROUND_PATTERN_TABLE_MODE 0x10
#define PPU_CONTROL_FLAG_SPRITE_SIZE_8X16 0x20
#define PPU_CONTROL_FLAG_NMI_ENABLED 0x80

// PPUMASK register flags
#define PPU_MASK_FLAG_GREYSCALE 0x1
#define PPU_MASK_FLAG_BACKGROUND_LEFT_8_VISIBLE 0x2
#define PPU_MASK_FLAG_SPRITE_LEFT_8_VISIBLE 0x4
#define PPU_MASK_FLAG_BACKGROUND_RENDERING_ENABLED 0x8
#define PPU_MASK_FLAG_SPRITE_RENDERING_ENABLED 0x10
#define PPU_MASK_FLAG_RED_EMPHASIS 0x20
#define PPU_MASK_FLAG_GREEN_EMPHASIS 0x40
#define PPU_MASK_FLAG_BLUE_EMPHASIS 0x80

// ppu constants
#define DOTS_PER_SCAN_LINE 341
#define VISIBLE_SCAN_LINE_COUNT 240
#define SYSTEM_PALETTE_COLOUR_COUNT 64
#define MAX_SPRITE_COUNT 64
#define MAX_SPRITES_PER_LINE 8
#define BACKGROUND_PIXEL_QUEUE_COUNT 16

// apu constants
#define AUDIO_CHANNEL_COUNT 5
#define MAX_VOLUME 15
#define APU_OUTPUT_BUFFER_BLOCK_COUNT 16
#define APU_SCALED_SAMPLE_COUNTER_MULTIPLIER 1024
#define APU_NOISE_RANDOM_SEQUENCE_LENGTH_LONG 32768
#define APU_NOISE_RANDOM_SEQUENCE_LENGTH_SHORT 64
#define DELTA_MODULATION_STEP_COUNT 2
#define DELTA_MODULATION_MAX_LEVEL 127

// 48khz, 8-bit audio output
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_BITS_PER_SAMPLE 8

// 1ms buffer block size, 50ms total buffer
#define PLAYBACK_BUFFER_SAMPLE_COUNT (AUDIO_SAMPLE_RATE / 1000)
#define PLAYBACK_LOOP_BUFFER_COUNT 50

// input keys
#define INPUT_KEY_COUNT 8

// display window constants
#define DISPLAY_WINDOW_WIDTH 256
#define DISPLAY_WINDOW_HEIGHT 240
#define WM_REDRAW_BITMAP WM_USER

enum RegionTypeEnum
{
	REGION_NTSC,
	REGION_PAL
};

enum AddressModeEnum
{
	ADDRESS_MODE_IMPLIED,
	ADDRESS_MODE_ACCUMULATOR,
	ADDRESS_MODE_IMMEDIATE,
	ADDRESS_MODE_ZERO_PAGE,
	ADDRESS_MODE_ZERO_PAGE_X,
	ADDRESS_MODE_ZERO_PAGE_Y,
	ADDRESS_MODE_RELATIVE,
	ADDRESS_MODE_ABSOLUTE,
	ADDRESS_MODE_ABSOLUTE_X,
	ADDRESS_MODE_ABSOLUTE_Y,
	ADDRESS_MODE_INDIRECT,
	ADDRESS_MODE_INDIRECT_X,
	ADDRESS_MODE_INDIRECT_Y,
	ADDRESS_MODE_PUSH,
	ADDRESS_MODE_POP,
	ADDRESS_MODE_INTERRUPT,
	ADDRESS_MODE_DMA,
	ADDRESS_MODE_SPECIAL
};

enum InstructionTypeEnum
{
	INSTRUCTION_TYPE_ADC,
	INSTRUCTION_TYPE_AND,
	INSTRUCTION_TYPE_ASL,
	INSTRUCTION_TYPE_BCC,
	INSTRUCTION_TYPE_BCS,
	INSTRUCTION_TYPE_BEQ,
	INSTRUCTION_TYPE_BIT,
	INSTRUCTION_TYPE_BMI,
	INSTRUCTION_TYPE_BNE,
	INSTRUCTION_TYPE_BPL,
	INSTRUCTION_TYPE_BRK,
	INSTRUCTION_TYPE_BVC,
	INSTRUCTION_TYPE_BVS,
	INSTRUCTION_TYPE_CLC,
	INSTRUCTION_TYPE_CLD,
	INSTRUCTION_TYPE_CLI,
	INSTRUCTION_TYPE_CLV,
	INSTRUCTION_TYPE_CMP,
	INSTRUCTION_TYPE_CPX,
	INSTRUCTION_TYPE_CPY,
	INSTRUCTION_TYPE_DEC,
	INSTRUCTION_TYPE_DEX,
	INSTRUCTION_TYPE_DEY,
	INSTRUCTION_TYPE_EOR,
	INSTRUCTION_TYPE_INC,
	INSTRUCTION_TYPE_INX,
	INSTRUCTION_TYPE_INY,
	INSTRUCTION_TYPE_JMP,
	INSTRUCTION_TYPE_JSR,
	INSTRUCTION_TYPE_LDA,
	INSTRUCTION_TYPE_LDX,
	INSTRUCTION_TYPE_LDY,
	INSTRUCTION_TYPE_LSR,
	INSTRUCTION_TYPE_NOP,
	INSTRUCTION_TYPE_ORA,
	INSTRUCTION_TYPE_PHA,
	INSTRUCTION_TYPE_PHP,
	INSTRUCTION_TYPE_PLA,
	INSTRUCTION_TYPE_PLP,
	INSTRUCTION_TYPE_ROL,
	INSTRUCTION_TYPE_ROR,
	INSTRUCTION_TYPE_RTI,
	INSTRUCTION_TYPE_RTS,
	INSTRUCTION_TYPE_SBC,
	INSTRUCTION_TYPE_SEC,
	INSTRUCTION_TYPE_SED,
	INSTRUCTION_TYPE_SEI,
	INSTRUCTION_TYPE_STA,
	INSTRUCTION_TYPE_STX,
	INSTRUCTION_TYPE_STY,
	INSTRUCTION_TYPE_TAX,
	INSTRUCTION_TYPE_TAY,
	INSTRUCTION_TYPE_TSX,
	INSTRUCTION_TYPE_TXA,
	INSTRUCTION_TYPE_TXS,
	INSTRUCTION_TYPE_TYA,
	INSTRUCTION_TYPE_STP,
	INSTRUCTION_TYPE_SLO,
	INSTRUCTION_TYPE_RLA,
	INSTRUCTION_TYPE_SRE,
	INSTRUCTION_TYPE_RRA,
	INSTRUCTION_TYPE_SAX,
	INSTRUCTION_TYPE_LAX,
	INSTRUCTION_TYPE_DCP,
	INSTRUCTION_TYPE_ISC,
	INSTRUCTION_TYPE_NMI,
	INSTRUCTION_TYPE_IRQ,
	INSTRUCTION_TYPE_DMA
};

enum InstructionStateEnum
{
	INSTRUCTION_STATE_COMPLETE,
	INSTRUCTION_STATE_IMPLIED_BEGIN,
	INSTRUCTION_STATE_IMMEDIATE_BEGIN,
	INSTRUCTION_STATE_ABSOLUTE_BEGIN,
	INSTRUCTION_STATE_ABSOLUTE_READ_HIGH,
	INSTRUCTION_STATE_ABSOLUTE_READ_ADDRESS,
	INSTRUCTION_STATE_ABSOLUTE_WRITE_ADDRESS,
	INSTRUCTION_STATE_ABSOLUTE_MODIFY,
	INSTRUCTION_STATE_RELATIVE_BEGIN,
	INSTRUCTION_STATE_RELATIVE_CONDITION_MET,
	INSTRUCTION_STATE_RELATIVE_PAGE_CROSSED,
	INSTRUCTION_STATE_ZERO_PAGE_BEGIN,
	INSTRUCTION_STATE_ZERO_PAGE_READ_ADDRESS,
	INSTRUCTION_STATE_ZERO_PAGE_WRITE_ADDRESS,
	INSTRUCTION_STATE_ZERO_PAGE_MODIFY,
	INSTRUCTION_STATE_INDIRECT_Y_BEGIN,
	INSTRUCTION_STATE_INDIRECT_Y_READ_LOW,
	INSTRUCTION_STATE_INDIRECT_Y_READ_HIGH,
	INSTRUCTION_STATE_INDIRECT_Y_READ_ADDRESS,
	INSTRUCTION_STATE_INDIRECT_Y_READ_ADDRESS_PAGE_CROSSED,
	INSTRUCTION_STATE_INDIRECT_Y_WRITE_ADDRESS,
	INSTRUCTION_STATE_INDIRECT_Y_MODIFY,
	INSTRUCTION_STATE_JSR_BEGIN,
	INSTRUCTION_STATE_JSR_NOP,
	INSTRUCTION_STATE_JSR_PUSH_PC_HIGH,
	INSTRUCTION_STATE_JSR_PUSH_PC_LOW,
	INSTRUCTION_STATE_JSR_UPDATE_PC,
	INSTRUCTION_STATE_PUSH_BEGIN,
	INSTRUCTION_STATE_PUSH_WRITE,
	INSTRUCTION_STATE_ACCUMULATOR_BEGIN,
	INSTRUCTION_STATE_POP_BEGIN,
	INSTRUCTION_STATE_POP_NOP,
	INSTRUCTION_STATE_POP_READ,
	INSTRUCTION_STATE_RTS_BEGIN,
	INSTRUCTION_STATE_RTS_NOP,
	INSTRUCTION_STATE_RTS_POP_PC_LOW,
	INSTRUCTION_STATE_RTS_POP_PC_HIGH,
	INSTRUCTION_STATE_RTS_INCREASE_PC,
	INSTRUCTION_STATE_RTI_BEGIN,
	INSTRUCTION_STATE_RTI_NOP,
	INSTRUCTION_STATE_RTI_POP_STATUS,
	INSTRUCTION_STATE_RTI_POP_PC_LOW,
	INSTRUCTION_STATE_RTI_POP_PC_HIGH,
	INSTRUCTION_STATE_INTERRUPT_BEGIN,
	INSTRUCTION_STATE_INTERRUPT_PUSH_PC_HIGH,
	INSTRUCTION_STATE_INTERRUPT_PUSH_PC_LOW,
	INSTRUCTION_STATE_INTERRUPT_PUSH_STATUS,
	INSTRUCTION_STATE_INTERRUPT_READ_PC_LOW,
	INSTRUCTION_STATE_INTERRUPT_READ_PC_HIGH,
	INSTRUCTION_STATE_ABSOLUTE_XY_BEGIN,
	INSTRUCTION_STATE_ABSOLUTE_XY_READ_HIGH,
	INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS,
	INSTRUCTION_STATE_ABSOLUTE_XY_READ_ADDRESS_PAGE_CROSSED,
	INSTRUCTION_STATE_ABSOLUTE_XY_WRITE_ADDRESS,
	INSTRUCTION_STATE_ABSOLUTE_XY_MODIFY,
	INSTRUCTION_STATE_ZERO_PAGE_XY_BEGIN,
	INSTRUCTION_STATE_ZERO_PAGE_XY_ADD_INDEX,
	INSTRUCTION_STATE_ZERO_PAGE_XY_READ_ADDRESS,
	INSTRUCTION_STATE_ZERO_PAGE_XY_WRITE_ADDRESS,
	INSTRUCTION_STATE_ZERO_PAGE_XY_MODIFY,
	INSTRUCTION_STATE_INDIRECT_BEGIN,
	INSTRUCTION_STATE_INDIRECT_READ_ADDRESS_HIGH,
	INSTRUCTION_STATE_INDIRECT_READ_POINTER_LOW,
	INSTRUCTION_STATE_INDIRECT_READ_POINTER_HIGH,
	INSTRUCTION_STATE_INDIRECT_X_BEGIN,
	INSTRUCTION_STATE_INDIRECT_X_ADD_X,
	INSTRUCTION_STATE_INDIRECT_X_READ_LOW,
	INSTRUCTION_STATE_INDIRECT_X_READ_HIGH,
	INSTRUCTION_STATE_INDIRECT_X_READ_ADDRESS,
	INSTRUCTION_STATE_INDIRECT_X_WRITE_ADDRESS,
	INSTRUCTION_STATE_INDIRECT_X_MODIFY,
	INSTRUCTION_STATE_DMA_BEGIN,
	INSTRUCTION_STATE_DMA_READ,
	INSTRUCTION_STATE_DMA_WRITE,
	INSTRUCTION_STATE_STP_BEGIN
};

enum PpuAddressRegisterTypeEnum
{
	PPU_ADDRESS_REGISTER_TYPE_V,
	PPU_ADDRESS_REGISTER_TYPE_T
};

enum PpuAddressRegisterFieldEnum
{
	PPU_ADDRESS_REGISTER_FIELD_TILE_POS_X,
	PPU_ADDRESS_REGISTER_FIELD_TILE_POS_Y,
	PPU_ADDRESS_REGISTER_FIELD_TILE_OFFSET_Y,
	PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE,
	PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_X,
	PPU_ADDRESS_REGISTER_FIELD_NAME_TABLE_Y,
	PPU_ADDRESS_REGISTER_FIELD_ADDR_HIGH,
	PPU_ADDRESS_REGISTER_FIELD_ADDR_LOW
};

enum NameTableMirrorModeEnum
{
	NAME_TABLE_MIRROR_MODE_VERTICAL,
	NAME_TABLE_MIRROR_MODE_HORIZONTAL
};

struct CpuOpcodeStruct
{
	BYTE bOpcode;
	InstructionTypeEnum InstructionType;
	AddressModeEnum AddressMode;
};

struct CpuInstructionAttributesStruct
{
	InstructionTypeEnum InstructionType;
	char *pName;
	BYTE bFlags;
	DWORD (*pCommonInstructionHandler)(BYTE bIn, BYTE *pbOut);
};

struct CpuRegistersStruct
{
	BYTE bA;
	BYTE bX;
	BYTE bY;
	WORD wPC;
	BYTE bSP;
	BYTE bStatus;
};

struct SystemPaletteEntryStruct
{
	BYTE bRed;
	BYTE bGreen;
	BYTE bBlue;
};

struct SquareWaveSequenceStruct
{
	BYTE bSequence[8];
};

struct AudioChannelLengthCounterStruct
{
	BYTE bCurrCounter;
	BYTE bHalt;
};

struct AudioChannelVolumeDecayStruct
{
	BYTE bEnabled;

	BYTE bResetFlag;
	BYTE bOrigCounter;
	BYTE bCurrCounter;
	BYTE bLoop;

	BYTE *pbVolume;
};

struct AudioChannelSweepStruct
{
	BYTE bEnabled;

	BYTE bResetFlag;
	BYTE bOrigCounter;
	BYTE bCurrCounter;
	BYTE bNegate;
	BYTE bNegateAdditional;
	BYTE bShiftCount;

	WORD *pwFrequencyTimer;
};

struct AudioChannelLinearCounterStruct
{
	BYTE bControlFlag;

	BYTE bOrigCounter;
	BYTE bCurrCounter;

	BYTE bResetFlag;
};

struct AudioChannelState_SquareWaveStruct
{
	WORD wFrequencyTimer;

	DWORD dwCurrPhaseSamples;
	DWORD dwCurrPhaseSamplesRemaining;
	BYTE bCurrSequenceAdditionalSamples[8];
	BYTE bCurrSquareWaveSequenceMode;
	BYTE bCurrSquareWavePhaseIndex;
	BYTE bCurrSampleOn;
	BYTE bResetPhase;

	AudioChannelLengthCounterStruct LengthCounter;
	AudioChannelVolumeDecayStruct VolumeDecay;
	AudioChannelSweepStruct Sweep;
};

struct AudioChannelState_TriangleWaveStruct
{
	WORD wFrequencyTimer;

	DWORD dwCurrPhaseSamples;
	DWORD dwCurrPhaseSamplesRemaining;
	BYTE bCurrSequenceAdditionalSamples[32];
	BYTE bCurrTriangleWavePhaseIndex;
	BYTE bResetPhase;

	AudioChannelLengthCounterStruct LengthCounter;
	AudioChannelLinearCounterStruct LinearCounter;
};

struct AudioChannelState_NoiseStruct
{
	WORD wFrequencyTimer;

	BYTE bShortMode;
	DWORD dwCurrRandomSequenceIndex;
	DWORD dwCurrSamplesRemaining;
	BYTE bCurrSampleOn;

	AudioChannelLengthCounterStruct LengthCounter;
	AudioChannelVolumeDecayStruct VolumeDecay;
};

struct AudioChannelState_DeltaModulationStruct
{
	BYTE bIrqEnabled;
	BYTE bLoop;

	BYTE bOutputLevel;
	WORD wSampleAddress;
	WORD wSampleLength;

	BYTE bOrigCounter;
	BYTE bCurrCounter;

	BYTE bCurrByte;
	BYTE bCurrByte_BitsRemaining;

	WORD wCurrAddress;
	WORD wBytesRemaining;
};

struct AudioChannelStateStruct
{
	BYTE bAudioChannelIndex;
	BYTE bChannelEnabled;

	DWORD (*pCycleHandler)(AudioChannelStateStruct *pAudioChannel);
	DWORD (*pQuarterFrameHandler)(AudioChannelStateStruct *pAudioChannel);
	DWORD (*pHalfFrameHandler)(AudioChannelStateStruct *pAudioChannel);
	DWORD (*pWriteRegister)(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue);
	BYTE (*pGetNextSample)(AudioChannelStateStruct *pAudioChannel);

	WORD wOutputFrequency;
	BYTE bVolume;

	union
	{
		AudioChannelState_SquareWaveStruct SquareWave;
		AudioChannelState_NoiseStruct Noise;
		AudioChannelState_TriangleWaveStruct TriangleWave;
		AudioChannelState_DeltaModulationStruct DeltaModulation;
	};
};

struct WavePlaybackBufferStruct
{
	WAVEHDR WaveHeader;
	BYTE bBuffer[PLAYBACK_BUFFER_SAMPLE_COUNT * (AUDIO_BITS_PER_SAMPLE / 8)];
};

struct MasterClockStruct
{
	UINT64 qwCyclesWaiting;
	BYTE bCpuCycle;
	BYTE bPpuCycle;
	BYTE bApuCycle;

	BYTE bReady;
	UINT64 qwLastRemainder;
	UINT64 qwPrevCounter;
	UINT64 qwFrequency;
};

struct CpuStruct
{
	CpuRegistersStruct Reg;

	CpuOpcodeStruct *pCurrInstruction;
	CpuInstructionAttributesStruct *pInstructionAttributes;

	DWORD (*pInstructionCycleHandler)();
	InstructionStateEnum InstructionState;

	BYTE bCpuCounter;

	BYTE bNmiPending;
	BYTE bIrqPending_ApuDmc;
	BYTE bIrqPending_ApuFrame;

	BYTE bPendingInterruptDisableFlagUpdate;
	BYTE bPendingInterruptDisableFlagUpdate_OrigValue;

	BYTE bDmaPending;
	WORD wDmaBaseAddress;

	WORD wTempStateValue16;
	BYTE bTempStateValue8;
};

struct KeyMappingEntryStruct
{
	BYTE bVirtualKey;
	BYTE bKeyDown;
};

struct InputStruct
{
	KeyMappingEntryStruct KeyMappingList[INPUT_KEY_COUNT];
	BYTE bPollEnabled;
	BYTE bStoredValue;
};

struct BackgroundPixelQueueStruct
{
	BYTE bPaletteEntryIndex;
	BYTE bPaletteIndex;
};

struct PpuInternalRegistersStruct
{
	WORD wV;
	WORD wT;
	BYTE bX;
	BYTE bW;
};

struct ObjectAttributeEntryStruct
{
	BYTE bY;
	BYTE bTile;
	BYTE bAttributes;
	BYTE bX;
};

struct PpuStruct
{
	PpuInternalRegistersStruct Reg;

	WORD wDotIndex;
	WORD wScanLineIndex;
	BYTE bFrameIndex;

	BYTE bPpuControl;
	BYTE bPpuMask;
	BYTE bPpuStatus;

	BYTE bReadBuffer;

	BYTE bCurrNameTableEntry;
	WORD wCurrPatternTableAddress;
	BYTE bCurrTileLow;
	BYTE bCurrAttributeValue;

	BackgroundPixelQueueStruct BackgroundPixelQueue[BACKGROUND_PIXEL_QUEUE_COUNT];

	NameTableMirrorModeEnum NameTableMirrorMode;

	BYTE bInternalMemory[0x4000];

	BYTE bObjectAttributeMemoryAddress;
	ObjectAttributeEntryStruct ObjectAttributeMemory[MAX_SPRITE_COUNT];
	ObjectAttributeEntryStruct CurrLineSpriteList[MAX_SPRITES_PER_LINE];
	BYTE bCurrLineSpriteIndexList[MAX_SPRITES_PER_LINE];
	BYTE bCurrLineSpriteCount;

	BYTE bFrameSpriteZeroHit;

	SystemPaletteEntryStruct *pCurrPixelBackground;
	SystemPaletteEntryStruct *pCurrPixelSprite;
	BYTE bCurrPixelSpriteIndex;
	BYTE bCurrPixelSpriteBehindBackground;
};

struct ApuStruct
{
	HANDLE hThread;

	DWORD dwScaledSampleCounter;
	BYTE bQuarterFrameIndex;
	DWORD dwFrameCycleCounter;
	BYTE b5StepFrameMode;

	BYTE bOutputBufferCriticalSectionReady;
	CRITICAL_SECTION OutputBufferCriticalSection;

	BYTE bOutputBuffer[PLAYBACK_BUFFER_SAMPLE_COUNT * APU_OUTPUT_BUFFER_BLOCK_COUNT];
	DWORD dwOutputBufferSampleCount;
	HANDLE hOutputBufferEvent;

	BYTE bNoiseRandomSequence_Long[APU_NOISE_RANDOM_SEQUENCE_LENGTH_LONG];
	BYTE bNoiseRandomSequence_Short[APU_NOISE_RANDOM_SEQUENCE_LENGTH_SHORT];

	AudioChannelStateStruct Channels[AUDIO_CHANNEL_COUNT];
};

struct DeltaModulationOutputRateTableStruct
{
	BYTE bRateTable[16];
};

struct NoiseFrequencyTimerTableStruct
{
	WORD wFrequencyTimerTable[16];
};

struct RegionSpecificSettingsStruct
{
	DWORD dwMasterClockSpeedHz;
	DWORD dwCpuCyclePeriod;
	DWORD dwPpuCyclePeriod;
	DWORD dwApuCyclePeriod;

	DWORD dwApuQuarterFrameCycleCount;
	DWORD dwApuFullFrameCycleCount_4Step;
	DWORD dwApuFullFrameCycleCount_5Step;
	NoiseFrequencyTimerTableStruct ApuNoiseFrequencyTimerTable;
	DeltaModulationOutputRateTableStruct ApuDeltaModulationOutputRateTable;

	DWORD dwPpuTotalScanLineCount;
	BYTE bPpuSkipFirstIdleOddFramePreRender;
};

struct DisplayWindowStruct
{
	HANDLE hThread;
	HWND hWnd;
	HDC hBitmapDC;
	VOID *pBitmapPixelData;
	BYTE bWindowClosed;

	HANDLE hRedrawBitmapThread;

	CRITICAL_SECTION CurrentFrameBufferCriticalSection;
	BYTE bCurrentFrameBufferCriticalSectionReady;
	RGBQUAD CurrentFrameBuffer[DISPLAY_WINDOW_WIDTH * DISPLAY_WINDOW_HEIGHT];
	BYTE bCurrentFrameReady;

	BYTE bFpsCounterReady;
	DWORD dwFpsPreviousTime;
	DWORD dwFpsCounter;
};

struct SystemStateStruct
{
	MasterClockStruct MasterClock;
	RegionSpecificSettingsStruct RegionSpecificSettings;
	BYTE bMemory[0x10000];
	CpuStruct Cpu;
	PpuStruct Ppu;
	ApuStruct Apu;
	InputStruct Input;
	DisplayWindowStruct DisplayWindow;
	HANDLE hShutDownEvent;
};

struct NesRomFileHeaderStruct
{
	BYTE bSignature[4];
	BYTE bPrgRomBlockCount;
	BYTE bChrRomBlockCount;
	BYTE bFlags[5];
	BYTE bPadding[5];
};

struct OpcodeLookupTableEntryStruct
{
	CpuOpcodeStruct *pCpuOpcode;
	CpuInstructionAttributesStruct *pCpuInstructionAttributes;
};

extern SystemStateStruct gSystem;
extern DWORD Memory_Read8(WORD wAddress, BYTE *pbValue);
extern DWORD Memory_ReadRange(WORD wAddress, VOID *pData, DWORD dwLength);
extern DWORD Memory_Read16(WORD wAddress, WORD *pwValue);
extern DWORD Memory_Write8(WORD wAddress, BYTE bValue);
extern DWORD Memory_WriteRange(WORD wAddress, VOID *pData, DWORD dwLength);
extern DWORD Memory_Reset();
extern DWORD CycleCpu();
extern DWORD LoadROM(char *pFilePath);
extern DWORD MasterClockLoop();
extern DWORD LookupInstruction(BYTE bOpcode, CpuOpcodeStruct **ppCpuOpcode, CpuInstructionAttributesStruct **ppCpuInstructionAttributes);
extern DWORD Stack_Push(BYTE bValue);
extern DWORD Stack_Pop(BYTE *pbValue);
extern DWORD StatusFlag_Set(BYTE bFlag);
extern DWORD StatusFlag_Clear(BYTE bFlag);
extern BYTE StatusFlag_Get(BYTE bFlag);
extern DWORD CallCommonInstructionHandler(BYTE bIn, BYTE *pbOut);
extern DWORD StatusFlag_SetAuto_Zero(BYTE bValue);
extern DWORD StatusFlag_SetAuto_Negative(BYTE bValue);
extern DWORD StatusFlag_FixAfterRestore();
extern DWORD HandleInstructionCycle_Implied();
extern DWORD HandleInstructionCycle_Immediate();
extern DWORD HandleInstructionCycle_Absolute();
extern DWORD HandleInstructionCycle_AbsoluteXY();
extern DWORD HandleInstructionCycle_Relative();
extern DWORD HandleInstructionCycle_ZeroPage();
extern DWORD HandleInstructionCycle_ZeroPageXY();
extern DWORD HandleInstructionCycle_Indirect();
extern DWORD HandleInstructionCycle_IndirectX();
extern DWORD HandleInstructionCycle_IndirectY();
extern DWORD HandleInstructionCycle_JSR();
extern DWORD HandleInstructionCycle_Push();
extern DWORD HandleInstructionCycle_Accumulator();
extern DWORD HandleInstructionCycle_Pop();
extern DWORD HandleInstructionCycle_RTS();
extern DWORD HandleInstructionCycle_RTI();
extern DWORD HandleInstructionCycle_Interrupt();
extern DWORD HandleInstructionCycle_DMA();
extern DWORD CommonInstructionHandler_BIT(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_AND(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_LDA(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_LDX(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_STA(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_STX(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_STY(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_LDY(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_DEC(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_CMP(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_CPX(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_CPY(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_ADC(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_INC(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_EOR(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_ROL(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_ROR(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_ORA(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_LSR(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_ASL(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_SBC(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_DCP(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_ISC(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_LAX(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_RLA(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_RRA(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_SAX(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_SLO(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_SRE(BYTE bIn, BYTE *pbOut);
extern DWORD CommonInstructionHandler_NOP(BYTE bIn, BYTE *pbOut);
extern DWORD InstructionUtils_Compare(BYTE bValue1, BYTE bValue2);
extern DWORD InstructionUtils_Load(BYTE bValue, BYTE *pbDestination);
extern DWORD CyclePpu();
extern DWORD PpuMemoryMappedRegisters_Read(WORD wAddress, BYTE *pbValue);
extern DWORD PpuMemoryMappedRegisters_Write(WORD wAddress, BYTE bValue);
extern DWORD PpuStatusFlag_Set(BYTE bFlag);
extern DWORD PpuStatusFlag_Clear(BYTE bFlag);
extern BYTE PpuControlFlag_Get(BYTE bFlag);
extern DWORD PpuAddressRegister_IncreaseV();
extern DWORD PpuMemory_Read8(WORD wAddress, BYTE *pbValue);
extern DWORD PpuMemory_Write8(WORD wAddress, BYTE bValue);
extern DWORD DisplayWindow_SetPixel(WORD wX, WORD wY, BYTE bRed, BYTE bGreen, BYTE bBlue);
extern DWORD DisplayWindow_FrameReady();
extern DWORD PpuMemory_WriteRange(WORD wAddress, VOID *pData, DWORD dwLength);
extern DWORD PpuAddressRegister_V_Get(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE *pbValue);
extern DWORD PpuAddressRegister_V_Set(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE bValue);
extern DWORD PpuAddressRegister_T_Get(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE *pbValue);
extern DWORD PpuAddressRegister_T_Set(PpuAddressRegisterFieldEnum PpuAddressRegisterField, BYTE bValue);
extern DWORD LookupInstruction_Placeholder(InstructionTypeEnum InstructionType, CpuOpcodeStruct **ppCpuOpcode, CpuInstructionAttributesStruct **ppCpuInstructionAttributes);
extern BYTE PpuMaskFlag_Get(BYTE bFlag);
extern DWORD ApuMemoryMappedRegisters_Write(WORD wAddress, BYTE bValue);
extern AudioChannelStateStruct *GetAudioChannel(BYTE bAudioChannelIndex);
extern DWORD CycleApu();
extern DWORD ApuMemoryMappedRegisters_Write_SquareWave(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue);
extern DWORD ApuQuarterFrame_SquareWave(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuHalfFrame_SquareWave(AudioChannelStateStruct *pAudioChannel);
extern BYTE GetNextSample_SquareWave(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuLengthCounter_Set(AudioChannelLengthCounterStruct *pLengthCounter, BYTE bLengthTableIndex);
extern DWORD ApuLengthCounter_Process(AudioChannelLengthCounterStruct *pLengthCounter);
extern DWORD ApuVolumeDecay_Set(AudioChannelVolumeDecayStruct *pVolumeDecay, BYTE bOrigCounter, BYTE bLoop);
extern DWORD ApuVolumeDecay_Process(AudioChannelVolumeDecayStruct *pVolumeDecay);
extern DWORD ApuSweep_Set(AudioChannelSweepStruct *pSweep, BYTE bOrigCounter, BYTE bNegate, BYTE bNegateAdditional, BYTE bShiftCount);
extern DWORD ApuSweep_Process(AudioChannelSweepStruct *pSweep);
extern DWORD ApuMemoryMappedRegisters_Write_Noise(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue);
extern BYTE GetNextSample_Noise(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuQuarterFrame_Noise(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuHalfFrame_Noise(AudioChannelStateStruct *pAudioChannel);
extern BYTE GetNextSample_TriangleWave(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuQuarterFrame_TriangleWave(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuHalfFrame_TriangleWave(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuMemoryMappedRegisters_Write_TriangleWave(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue);
extern DWORD ApuLinearCounter_Process(AudioChannelLinearCounterStruct *pLinearCounter);
extern DWORD CycleApu_QuarterFrame();
extern DWORD CycleApu_HalfFrame();
extern DWORD StatusFlag_SetPendingInterruptDisableFlagUpdate();
extern DWORD ApuMemoryMappedRegisters_Read(WORD wAddress, BYTE *pbValue);
extern DWORD ApuMemoryMappedRegisters_Write_DeltaModulation(AudioChannelStateStruct *pAudioChannel, WORD wApuRegisterAddress, BYTE bValue);
extern BYTE GetNextSample_DeltaModulation(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuCycle_DeltaModulation(AudioChannelStateStruct *pAudioChannel);
extern DWORD ApuGenerateOutputSample();
extern DWORD InputMemoryMappedRegisters_Read(WORD wAddress, BYTE *pbValue);
extern DWORD InputMemoryMappedRegisters_Write(WORD wAddress, BYTE bValue);
extern DWORD InitialiseAudioChannels();
extern DWORD ApuLinearCounter_Set(AudioChannelLinearCounterStruct *pLinearCounter, BYTE bControlFlag, BYTE bLinearCounter);
extern DWORD InitialiseInputKeyMappings();
extern SystemPaletteEntryStruct *GetSystemColourFromPalette(BYTE bPaletteIndex, BYTE bPaletteEntryIndex);
extern DWORD PpuProcessScanLine_Sprites(BYTE bPreRenderLine);
extern DWORD PpuProcessScanLine_Background(BYTE bPreRenderLine);
extern DWORD CheckPageCross(WORD wAddress1, WORD wAddress2);
extern DWORD PpuRenderPixel(BYTE bCurrX, BYTE bCurrY);
extern DWORD InitialiseDisplayWindow();
extern DWORD InitialiseRegionSpecificSettings(RegionTypeEnum RegionType);
extern DWORD PopulateOpcodeLookupTable();
extern DWORD UpdateKeyState(BYTE bVirtualKey, BYTE bKeyDown);
extern DWORD HandleInstructionCycle_STP();
extern DWORD PpuObjectAttributeMemory_Read(BYTE *pbValue);
extern DWORD PpuObjectAttributeMemory_Write(BYTE bValue);
