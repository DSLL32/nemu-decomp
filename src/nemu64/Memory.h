#ifndef NEMU_MEMORY_H
#define NEMU_MEMORY_H

// Names are inferred; original addresses and layout evidence are recorded in
// analysis/nemu-memory.json. These are host objects, not fixed-address aliases.
struct TLBEntry {
    unsigned EntryLo0, EntryHi, EntryLo1, PageMask;
    unsigned Valid0, Valid1, Global, Dirty0, Dirty1, ASID;
    unsigned VPN, PFN0, PFN1, PageShift, AddressMask, PFNShift, OffsetMask;
};
struct AIRegisters {
    unsigned DramAddress, NextDramAddress, Length, NextLength;
    unsigned Control, Status, Dacrate, Bitrate, Unknown20;
};
struct MIRegisters { unsigned Mode, InterruptMask, Interrupt; };
struct SPRegisters {
    unsigned MemAddress, DramAddress, ReadLength, WriteLength;
    unsigned Status, Semaphore, PC, Bist;
};
struct VIRegisters {
    unsigned Status, Origin, Width, Interrupt, Current, Burst, VSync;
    unsigned HSync, Leap, HStart, VStart, VBurst, XScale, YScale;
};
struct PIRegisters {
    unsigned DramAddress, CartAddress, ReadLength, WriteLength, Status;
    unsigned Domain1Latency, Domain1PulseWidth, Domain1PageSize, Domain1Release;
    unsigned Domain2Latency, Domain2PulseWidth, Domain2PageSize, Domain2Release;
};
struct RIRegisters {
    unsigned Mode, Config, CurrentLoad, Select, Refresh, Latency;
    unsigned ReadError, WriteError;
};
struct DPCRegisters {
    unsigned Start, End, Current, Status, Clock, BufferBusy, PipeBusy, Tmem;
};
struct RDRAMRegisters {
    unsigned Config, DeviceID, Delay, Mode, RefreshInterval, RefreshRow;
    unsigned RASInterval, MinInterval, AddressSelect, Manufacturer;
};

extern unsigned char *RDRAM;
extern unsigned RDRAMSize;
extern unsigned char *ROMImage;
extern unsigned ROMSize;
extern int ReadError;
extern int ShowErrors;
extern unsigned TranslationError;
extern unsigned TLBAddress;
extern unsigned TLBWriteAccess;
extern int TLBEnabled;
extern int TLBRamMapped;
extern TLBEntry TLB[32];
extern AIRegisters AIReg;
extern MIRegisters MIReg;
extern SPRegisters SPReg;
extern VIRegisters VIReg;
extern PIRegisters PIReg;
extern RIRegisters RIReg;
extern DPCRegisters DPCReg;
extern RDRAMRegisters RDRAMReg;
extern unsigned SIAddress;
extern unsigned FlashEnabled, FlashRamAddress, FlashStatus, FlashCommand;
extern unsigned HLETaskActive, HLETaskAddress, VICurrentLine;
extern unsigned RDPHistory[2048], RDPHistoryPosition;
extern unsigned DisplayListCount, DisplayListLastTime;
extern bool AllowRomWrites;
extern unsigned char SPMemory[0x4000];
extern unsigned char RSPShadowMemory[0x2000];
extern unsigned char PIFRam[64];
extern unsigned char PIFRom[0x7c0];

void CopyByteSwapped(void *destination, const void *source, unsigned length);
void __stdcall TLBRead();
void __stdcall TLBWrite(unsigned index);
void __stdcall TLBProbe();
// The native C helper retains its stack arguments across LTCG callers.
#ifdef __cplusplus
extern "C" {
#endif
void CopyAndSwapWords(void *destination, const void *source, unsigned length);
#ifdef __cplusplus
}
#endif
void InvalidateRecompiledRange(unsigned address, unsigned length);
unsigned char __stdcall ReadPhysicalByte(unsigned address);
unsigned short __stdcall ReadPhysicalHalf(unsigned address);
unsigned __stdcall ReadPhysicalWord(unsigned address);
void __stdcall WritePhysicalByte(unsigned address, unsigned char value);
void __stdcall WritePhysicalHalf(unsigned address, unsigned short value);
void __stdcall WritePhysicalWord(unsigned address, unsigned value);
void __stdcall WritePhysicalDoubleWord(unsigned address, unsigned lowWord, unsigned highWord);
unsigned __stdcall TranslateTLB(unsigned address, unsigned char writing);

#endif
