#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include "Memory.h"
#include "CPU.h"
#include "Plugins.h"
#include "Rom.h"
#include "PIF.h"
#include "Dynarec.h"
#include "WinMain.h"
#include "RomBrowser.h"


unsigned char *RDRAM;
unsigned RDRAMSize;
unsigned char *ROMImage;
unsigned ROMSize;
int ReadError;
int ShowErrors;
unsigned TranslationError;
unsigned TLBAddress;
unsigned TLBWriteAccess;
int TLBEnabled;
int TLBRamMapped;
TLBEntry TLB[32];
AIRegisters AIReg;
MIRegisters MIReg;
SPRegisters SPReg;
VIRegisters VIReg;
PIRegisters PIReg;
RIRegisters RIReg;
DPCRegisters DPCReg;
RDRAMRegisters RDRAMReg;
unsigned SIAddress;
unsigned FlashEnabled, FlashRamAddress, FlashStatus, FlashCommand;
unsigned HLETaskActive, HLETaskAddress, VICurrentLine;
unsigned RDPHistory[2048], RDPHistoryPosition;
unsigned DisplayListCount, DisplayListLastTime;
bool AllowRomWrites;
unsigned char PIFRam[64];
unsigned char PIFRom[0x7c0];
unsigned char RSPShadowMemory[0x2000];

// This is the original inline byte-order conversion, including its local
// result. It is symbolic assembly, not an embedded instruction-byte array.
static __forceinline unsigned SwapWord(unsigned value)
{
    unsigned result;
    __asm {
        mov eax, value
        bswap eax
        mov result, eax
    }
    return result;
}
static __forceinline unsigned short SwapHalf(unsigned short value)
{
    unsigned short result;
    __asm {
        mov ax, word ptr value
        xchg al, ah
        mov result, ax
    }
    return result;
}

// Original 00416A00; the error text identifies the historical helper ua_memcpy.
void CopyByteSwapped(void *destination, const void *source, unsigned length)
{
    if ((unsigned)destination & 2) {
        ReportCPUError("Error 0 in ua_memcpy");
        return;
    }
    if ((unsigned)source & 2) {
        for (unsigned i = 0; i < (length >> 1); ++i)
            *(unsigned short *)(((unsigned)destination + i * 2) ^ 2) =
                *(const unsigned short *)(((unsigned)source + i * 2) ^ 2);
        switch (length & 1) {
        case 1:
            *(unsigned char *)(((unsigned)destination + length - 1) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 1) ^ 3);
            break;
        }
    } else {
        switch (length & 3) {
        case 0:
            memcpy(destination, source, length);
            break;
        case 1:
            memcpy(destination, source, length - 1);
            *(unsigned char *)(((unsigned)destination + length - 1) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 1) ^ 3);
            break;
        case 2:
            memcpy(destination, source, length - 2);
            *(unsigned char *)(((unsigned)destination + length - 2) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 2) ^ 3);
            *(unsigned char *)(((unsigned)destination + length - 1) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 1) ^ 3);
            break;
        case 3:
            memcpy(destination, source, length - 3);
            *(unsigned char *)(((unsigned)destination + length - 3) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 3) ^ 3);
            *(unsigned char *)(((unsigned)destination + length - 2) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 2) ^ 3);
            *(unsigned char *)(((unsigned)destination + length - 1) ^ 3) =
                *(const unsigned char *)(((unsigned)source + length - 1) ^ 3);
            break;
        }
    }
}

// Original 00416B60.
void __stdcall TLBRead()
{
    unsigned index = CPU_COP0[CP0_Index] & 0x3f;
    CPU_COP0[CP0_PageMask] = TLB[index].PageMask;
    CPU_COP0[CP0_EntryHi] = TLB[index].EntryHi &
        ~TLB[CPU_COP0[CP0_Index]].PageMask & 0xffffefff;
    CPU_COP0[CP0_EntryLo1] = TLB[index].EntryLo1;
    CPU_COP0[CP0_EntryLo0] = TLB[index].EntryLo0;
}

// Original 00416BC0.
void __stdcall TLBWrite(unsigned index)
{
    if (index > 31)
        ReportCPUError("TLB Error 0 in TLBWI");
    TLB[index].PageMask = CPU_COP0[CP0_PageMask];
    TLB[index].EntryHi = CPU_COP0[CP0_EntryHi] & ~CPU_COP0[CP0_PageMask];
    TLB[index].EntryLo1 = CPU_COP0[CP0_EntryLo1] & 0xfffffffe;
    TLB[index].EntryLo0 = CPU_COP0[CP0_EntryLo0] & 0xfffffffe;
    TLB[index].ASID = CPU_COP0[CP0_EntryHi] & 0xff;
    if (TLB[index].ASID)
        ReportCPUError("Missing feature: ASID used by ROM");
    TLB[index].Global = (CPU_COP0[CP0_EntryLo0] & CPU_COP0[CP0_EntryLo1] & 1) != 0;
    TLB[index].Valid0 = (CPU_COP0[CP0_EntryLo0] >> 1) & 1;
    TLB[index].Valid1 = (CPU_COP0[CP0_EntryLo1] >> 1) & 1;
    TLB[index].Dirty0 = (CPU_COP0[CP0_EntryLo0] >> 2) & 1;
    TLB[index].Dirty1 = (CPU_COP0[CP0_EntryLo1] >> 2) & 1;
    TLB[index].PFN0 = (CPU_COP0[CP0_EntryLo0] >> 6) & 0xffffff;
    TLB[index].PFN1 = (CPU_COP0[CP0_EntryLo1] >> 6) & 0xffffff;
    switch (TLB[index].PageMask) {
    case 0:
        TLB[index].PageShift = 12;
        TLB[index].PFNShift = 12;
        TLB[index].AddressMask = 0xfffff000;
        TLB[index].OffsetMask = 0xfff;
        TLB[index].VPN = CPU_COP0[CP0_EntryHi] >> 13;
        break;
    case 0x6000:
        TLB[index].PageShift = 14;
        TLB[index].AddressMask = 0xffffc000;
        TLB[index].OffsetMask = 0x3fff;
        TLB[index].VPN = CPU_COP0[CP0_EntryHi] >> 15;
        TLB[index].PFNShift = 12;
        break;
    case 0x1e000:
        TLB[index].PageShift = 16;
        TLB[index].AddressMask = 0xffff0000;
        TLB[index].OffsetMask = 0xffff;
        TLB[index].VPN = CPU_COP0[CP0_EntryHi] >> 17;
        TLB[index].PFNShift = 12;
        break;
    case 0x7e000:
        TLB[index].PageShift = 18;
        TLB[index].AddressMask = 0xfffc0000;
        TLB[index].OffsetMask = 0x3ffff;
        TLB[index].VPN = CPU_COP0[CP0_EntryHi] >> 19;
        TLB[index].PFNShift = 12;
        break;
    case 0x1fe000:
        TLB[index].PageShift = 20;
        TLB[index].AddressMask = 0xfff00000;
        TLB[index].OffsetMask = 0xfffff;
        TLB[index].VPN = CPU_COP0[CP0_EntryHi] >> 21;
        TLB[index].PFNShift = 12;
        break;
    case 0x7fe000:
        TLB[index].PageShift = 22;
        TLB[index].AddressMask = 0xffc00000;
        TLB[index].OffsetMask = 0x3fffff;
        TLB[index].VPN = CPU_COP0[CP0_EntryHi] >> 23;
        TLB[index].PFNShift = 12;
        break;
    }
    if ((CPU_COP0[CP0_EntryHi] != 0xc0000000 || index != 31) &&
        (TLB[index].Valid0 || TLB[index].Valid1)) {
        if (!TLBEnabled) {
            TLBEnabled = 1;
            Dynarec_Use32BitMemoryAccess = 0;
            CPU_ActionFlags |= 0x400;
        }
    }
    if (TLBEnabled && !TLBRamMapped &&
        (TLB[index].Valid0 || TLB[index].Valid1) && TLB[index].EntryHi < RDRAMSize) {
        Dynarec_Use32BitMemoryAccess = CPU_Use32BitMemoryAccess;
        TLBRamMapped = 1;
        CPU_ActionFlags |= 0x400;
    }
}

// Original 00416EA0.
void __stdcall TLBProbe()
{
    for (unsigned index = 0; index < 32; ++index) {
        if (TLB[index].Global && TLB[index].VPN ==
            (((CPU_COP0[CP0_EntryHi] & TLB[index].AddressMask) >> TLB[index].PageShift) >> 1)) {
            CPU_COP0[CP0_Index] = index;
            return;
        }
    }
    CPU_COP0[CP0_Index] = 0x80000000;
}

// Original 00416F30. The protection length intentionally excludes the final
// byte, while the block-lookup clear covers the whole rounded page range.
void InvalidateRecompiledRange(unsigned address, unsigned length)
{
    if (Dynarec_Mode == 3) {
        unsigned start = address & 0xfffff000;
        unsigned size = ((address + length) | 0xfff) - start + 1;
        DWORD oldProtection;
        VirtualProtect(RDRAM + start, size - 1, PAGE_READWRITE, &oldProtection);
        memset(Dynarec_BlockCode + (start >> 2), 0, size);
    }
}

// Original 00416F90.
unsigned char __stdcall ReadPhysicalByte(unsigned address)
{
    if (address < RDRAMSize)
        return RDRAM[address ^ 3];
    if (address >= 0x04000000 && address <= 0x04001fff)
        return SPMemory[address - 0x04000000];
    if (address >= 0x1fc007c0 && address <= 0x1fc007ff)
        return PIFRam[address - 0x1fc007c0];
    if (address >= 0x1fc00000 && address <= 0x1fc007bf)
        return PIFRom[address - 0x1fc00000];
    if (address >= 0x10000000 && address <= 0x1fbfffff) {
        if (address <= ROMSize + 0x0ffffffe) {
            address -= 0x10000000;
            if ((address & 3) > 1)
                return ROMImage[(address + 2) ^ 3];
            return ROMImage[address ^ 3];
        }
    } else {
        ReadError = 1;
        ReportMemoryError("Trying to Read8 from unhandled or invalid memory", address);
    }
    return 0;
}

// Original 00417060.
unsigned short __stdcall ReadPhysicalHalf(unsigned address)
{
    if (address < RDRAMSize)
        return *(unsigned short *)(RDRAM + (address ^ 2));
    if (address >= 0x10000000 && address <= 0x1fbfffff) {
        address -= 0x10000000;
        if (address <= ROMSize) {
            if (address & 2)
                return *(unsigned short *)(ROMImage + ((address + 2) ^ 2));
            return *(unsigned short *)(ROMImage + (address ^ 2));
        }
    } else {
        if (address >= 0x1fc007c0 && address <= 0x1fc007ff)
            return SwapHalf(*(unsigned short *)(PIFRam + address - 0x1fc007c0));
        if (address >= 0x1fc00000 && address <= 0x1fc007bf)
            return SwapHalf(*(unsigned short *)(PIFRom + address - 0x1fc00000));
        if (address >= 0x04000000 && address <= 0x04001fff)
            return SwapHalf(*(unsigned short *)(SPMemory + address - 0x04000000));
        ReadError = 1;
        ReportMemoryError("Trying to Read16 from unhandled or invalid memory", address);
    }
    return 0;
}

extern unsigned VICurrentLine;

// Original 00417180. Switch order and unimplemented register behavior follow
// the executable; in particular unhandled AI registers return their address.
unsigned __stdcall ReadPhysicalWord(unsigned address)
{
    if (address < RDRAMSize)
        return *(unsigned *)(RDRAM + address);
    if (address >= 0x04000000 && address <= 0x04001fff)
        return SwapWord(*(unsigned *)(SPMemory + address - 0x04000000));
    if (address >= 0x1fc007c0 && address <= 0x1fc007ff)
        return SwapWord(*(unsigned *)(PIFRam + address - 0x1fc007c0));
    if (address >= 0x1fc00000 && address <= 0x1fc007bf)
        return SwapWord(*(unsigned *)(PIFRom + address - 0x1fc00000));
    if (address >= 0x10000000 && address <= 0x1fbfffff) {
        if (address - 0x10000000 < ROMSize)
            return *(unsigned *)(ROMImage + address - 0x10000000);
        unsigned offset = address - 0x13fcb4d0;
        if (offset < ROMSize)
            return *(unsigned *)(ROMImage + offset);
    }
    if (address >= 0x04700000 && address <= 0x0470001c) {
        switch (address) {
        case 0x04700000: return RIReg.Mode;
        case 0x04700004: return RIReg.Config;
        case 0x0470000c: return RIReg.Select;
        case 0x04700010: return RIReg.Refresh;
        case 0x04700014: return RIReg.Latency;
        case 0x04700018: return RIReg.ReadError;
        }
    }
    if (address >= 0x04300000 && address <= 0x0430000c) {
        switch (address) {
        case 0x04300000: return MIReg.Mode;
        case 0x04300004: return 0x02020202;
        case 0x04300008: return MIReg.Interrupt;
        case 0x0430000c: return MIReg.InterruptMask;
        }
    }
    if (address >= 0x04500000 && address <= 0x04500014) {
        unsigned result;
        switch (address) {
        case 0x04500004:
            {
                if (CPU_AudioEnabled) {
                    unsigned rate;
                    if (VideoNTSC) rate = 48681812; else rate = 49656530;
                    result = (int)((rate * (__int64)CPU_AudioCyclesRemaining / 46875000 /
                        (__int64)(unsigned)(AIReg.Dacrate + 1)) << 2);
                    if ((int)result < 0) result = 0;
                } else result = 0;
                AIReg.Length = result;
                break;
            }
        case 0x0450000c:
            result = AIGetReg(0x0450000c);
            break;
        }
        return result;
    }
    if (address >= 0x04600000 && address <= 0x04600030) {
        switch (address) {
        case 0x04600000: return PIReg.DramAddress;
        case 0x04600004: return PIReg.CartAddress;
        case 0x04600008: return PIReg.ReadLength;
        case 0x0460000c: return PIReg.WriteLength;
        case 0x04600010: return PIReg.Status;
        case 0x04600014: return PIReg.Domain1Latency;
        case 0x04600018: return PIReg.Domain1PulseWidth;
        case 0x0460001c: return PIReg.Domain1PageSize;
        case 0x04600020: return PIReg.Domain1Release;
        case 0x04600024: return PIReg.Domain2Latency;
        case 0x04600028: return PIReg.Domain2PulseWidth;
        case 0x0460002c: return PIReg.Domain2PageSize;
        case 0x04600030: return PIReg.Domain2Release;
        }
    }
    if (address == 0x04080000) return SPReg.PC;
    if (address == 0x04080004) return SPReg.Bist;
    if (address >= 0x03f00000 && address <= 0x03ffffff) {
        switch ((address & 0xff) | 0x03f00000) {
        case 0x03f00000:
            switch (address >> 10) {
            case 0:
            case 1: return 0x200000;
            case 2:
            case 3: return RDRAMSize == 0x800000 ? 0x200000 : 0;
            }
            break;
        case 0x03f00004: return RDRAMReg.DeviceID;
        case 0x03f00008: return RDRAMReg.Delay;
        case 0x03f0000c: return RDRAMReg.Mode;
        case 0x03f00010: return RDRAMReg.RefreshInterval;
        case 0x03f00014: return RDRAMReg.RefreshRow;
        case 0x03f00018: return RDRAMReg.RASInterval;
        case 0x03f0001c: return RDRAMReg.MinInterval;
        case 0x03f00020: return RDRAMReg.AddressSelect;
        case 0x03f00024: return RDRAMReg.Manufacturer;
        }
        return 0;
    }
    if (address >= 0x04800000 && address <= 0x04800018) {
        switch (address) {
        case 0x04800000: return SIAddress;
        case 0x04800018: return (MIReg.Interrupt & 2) << 11;
        }
    }
    if (address >= 0x04040000 && address <= 0x0404001c) {
        switch (address) {
        case 0x04040000: return SPReg.MemAddress;
        case 0x04040004: return SPReg.DramAddress;
        case 0x04040008: return SPReg.ReadLength;
        case 0x0404000c: return SPReg.WriteLength;
        case 0x04040010: return SPReg.Status;
        case 0x04040014: return (SPReg.Status >> 3) & 1;
        case 0x04040018: return (SPReg.Status >> 2) & 1;
        case 0x0404001c: {
            unsigned result = SPReg.Semaphore;
            SPReg.Semaphore = 1;
            return result;
        }
        }
    }
    if (address >= 0x04400000 && address <= 0x04400034) {
        switch (address) {
        case 0x04400000: return VIReg.Status;
        case 0x04400004: return VIReg.Origin;
        case 0x04400008: return VIReg.Width;
        case 0x0440000c: return VIReg.Interrupt;
        case 0x04400010:
            VICurrentLine += 2;
            return ((int)VICurrentLine >> 4) & 0x3ff;
        case 0x04400014: return VIReg.Burst;
        case 0x04400018: return VIReg.VSync;
        case 0x0440001c: return VIReg.HSync;
        case 0x04400020: return VIReg.Leap;
        case 0x04400024: return VIReg.HStart;
        case 0x04400028: return VIReg.VStart;
        case 0x0440002c: return VIReg.VBurst;
        case 0x04400030: return VIReg.XScale;
        case 0x04400034: return VIReg.YScale;
        }
    }
    if (address >= 0x04100000 && address <= 0x0410001c) {
        switch (address) {
        case 0x04100000: return DPCReg.Start;
        case 0x04100004: return DPCReg.End;
        case 0x04100008: return DPCReg.Current;
        case 0x0410000c: return DPCReg.Status;
        case 0x04100010: return DPCReg.Clock;
        case 0x04100014: return DPCReg.BufferBusy;
        case 0x04100018: return DPCReg.PipeBusy;
        case 0x0410001c: return DPCReg.Tmem;
        }
    }
    if (address >= 0x08000000 && address <= 0x0803ffff) {
        if (FlashCommand != 0xf0000000) return FlashStatus;
        unsigned result;
        __asm {
            mov eax, dword ptr SRAMData
            bswap eax
            mov result, eax
        }
        return result;
    }
    if (address != 0x06001010 && address != 0x06000000) {
        if (address == 0x05000508) return 0x05080508;
        if (address >= RDRAMSize + 0x400000 && address != 0x1ff00000) {
            ReadError = 1;
            ReportMemoryError("Trying to Read32 from unhandled or invalid memory", address);
        }
    }
    return 0;
}

void __stdcall WritePhysicalByte(unsigned address, unsigned char value)
{
    if (address < RDRAMSize) {
        __try {
            RDRAM[address ^ 3] = value;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            InvalidateRecompiledRange(address, 4);
            RDRAM[address ^ 3] = value;
        }
        if (Dynarec_Mode == 5)
            Dynarec_BlockCode[address >> 2] = 0;
        return;
    }
    if (address >= 0x04000000 && address <= 0x04001fff) {
        SPMemory[address - 0x04000000] = value;
        return;
    }
    if (address >= 0x1fc007c0 && address <= 0x1fc007ff) {
        PIFRam[address - 0x1fc007c0] = value;
        ProcessPIF();
        return;
    }
    ReadError = 1;
    ReportMemoryError("Trying to Write8 to unhandled or invalid memory", address);
}

// Original 00417B30. The host fault handler unprotects RAM and retries the store.
void __stdcall WritePhysicalHalf(unsigned address, unsigned short value)
{
    if (address < RDRAMSize) {
        __try {
            *(unsigned short *)(RDRAM + (address ^ 2)) = value;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            InvalidateRecompiledRange(address, 4);
            *(unsigned short *)(RDRAM + (address ^ 2)) = value;
        }
        if (Dynarec_Mode == 5)
            Dynarec_BlockCode[address >> 2] = 0;
        return;
    }
    if (address >= 0x04000000 && address <= 0x04001fff) {
        *(unsigned short *)(SPMemory + address - 0x04000000) = SwapHalf(value);
        return;
    }
    if (address >= 0x1fc007c0 && address <= 0x1fc007ff) {
        *(unsigned short *)(PIFRam + address - 0x1fc007c0) = SwapHalf(value);
        ProcessPIF();
        return;
    }
    ReadError = 1;
    ReportMemoryError("Trying to Write16 to unhandled or invalid memory", address);
}

// Original 00417C70.
void __stdcall WritePhysicalWord(unsigned address, unsigned value)
{
    if (address < RDRAMSize) {
        __try {
            *(unsigned *)(RDRAM + address) = value;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            InvalidateRecompiledRange(address, 4);
            *(unsigned *)(RDRAM + address) = value;
        }
        if (Dynarec_Mode == 5)
            Dynarec_BlockCode[address >> 2] = 0;
        return;
    }
    if (address >= 0x04000000 && address <= 0x04001fff) {
        *(unsigned *)(SPMemory + address - 0x04000000) = SwapWord(value);
        return;
    }
    if (address >= 0x1fc007c0 && address <= 0x1fc007ff) {
        if (value == 8 && RuntimeLibraryEnabled)
            SaveFunctionSignatures((unsigned *)(RDRAM + (((unsigned)CPU_PC & 0xfff000) - 0x40000)));
        *(unsigned *)(PIFRam + address - 0x1fc007c0) = SwapWord(value);
        ProcessPIF();
        return;
    }
    if (address >= 0x04700000 && address <= 0x0470001c) {
        switch (address) {
        case 0x04700000: RIReg.Mode = value; return;
        case 0x04700004: RIReg.Config = value; return;
        case 0x04700008: RIReg.CurrentLoad = value; return;
        case 0x0470000c: RIReg.Select = value; return;
        case 0x04700010: RIReg.Refresh = value; return;
        case 0x04700014: RIReg.Latency = value; return;
        case 0x0470001c: RIReg.WriteError = value; return;
        }
    }
    if (address >= 0x04040000 && address <= 0x0404001c) {
        switch (address) {
        case 0x04040000: SPReg.MemAddress = value; return;
        case 0x04040004: SPReg.DramAddress = value; return;
        case 0x04040008: {
            SPReg.ReadLength = value;
            unsigned dram = SPReg.DramAddress & 0xffffff;
            unsigned task = ReadPhysicalWord(dram);
            unsigned length = (SPReg.ReadLength & 0xff8 | 7) + 1;
            if (dram + length >= RDRAMSize) {
                ReportCPUError("Error: Start+Size of sp dma not in rdram");
                return;
            }
            bool forceLowLevel = false;
            if (CurrentRom.CRC1 == 0xaa18b1a5 && CurrentRom.CRC2 == 0x07db6aeb &&
                ReadPhysicalWord(SPReg.DramAddress + 0x30) == 0)
                forceLowLevel = true;
            if (!forceLowLevel && SPReg.MemAddress == 0x04000fc0 &&
                ((task == 1 && !CPU_DisableVideoHLE) ||
                 (task == 2 && !CPU_DisableAudioHLE))) {
                HLETaskActive = 1;
                HLETaskAddress = SPReg.DramAddress;
                memcpy(RSPShadowMemory + 0xfc0, RDRAM + dram, length);
                return;
            }
            if (HLETaskActive) {
                memcpy(RSPShadowMemory + (SPReg.MemAddress & 0x1fff), RDRAM + dram, length);
                return;
            }
            unsigned count = (SPReg.ReadLength >> 12) & 0xff;
            unsigned skip = SPReg.ReadLength >> 20;
            unsigned sp = SPReg.MemAddress & 0x1fff;
            for (unsigned block = 0; block <= count; ++block) {
                if (sp + length > 0x2000 || dram + length > RDRAMSize) {
                    ReportCPUError("SP DMA out of bounds");
                    return;
                }
                CopyAndSwapWords(SPMemory + sp, RDRAM + dram, length);
                dram += skip + length;
                sp += length;
            }
            return;
        }
        case 0x0404000c: {
            SPReg.WriteLength = value;
            unsigned length = (value & 0xff8 | 7) + 1;
            unsigned count = ((value >> 12) & 0xff) + 1;
            unsigned stride = (value >> 20) + length;
            unsigned dram = SPReg.DramAddress & 0xffffff;
            unsigned sp = SPReg.MemAddress & 0x1fff;
            InvalidateRecompiledRange(dram, stride * count);
            do {
                CopyAndSwapWords(RDRAM + dram, SPMemory + sp, length);
                dram += stride;
                sp += length;
            } while (--count);
            return;
        }
        case 0x04040010:
            if (value & 0x0000001) SPReg.Status &= ~0x0001;
            if (value & 0x0000002) SPReg.Status |=  0x0001;
            if (value & 0x0000004) SPReg.Status &= ~0x0002;
            if (value & 0x0000008) MIReg.Interrupt &= ~1;
            if (value & 0x0000010) CPU_ActionFlags |= 8;
            if (value & 0x0000020) SPReg.Status &= ~0x0020;
            if (value & 0x0000040) SPReg.Status |=  0x0020;
            if (value & 0x0000080) SPReg.Status &= ~0x0040;
            if (value & 0x0000100) SPReg.Status |=  0x0040;
            if (value & 0x0000200) SPReg.Status &= ~0x0080;
            if (value & 0x0000400) SPReg.Status |=  0x0080;
            if (value & 0x0000800) SPReg.Status &= ~0x0100;
            if (value & 0x0001000) SPReg.Status |=  0x0100;
            if (value & 0x0002000) SPReg.Status &= ~0x0200;
            if (value & 0x0004000) SPReg.Status |=  0x0200;
            if (value & 0x0008000) SPReg.Status &= ~0x0400;
            if (value & 0x0010000) SPReg.Status |=  0x0400;
            if (value & 0x0020000) SPReg.Status &= ~0x0800;
            if (value & 0x0040000) SPReg.Status |=  0x0800;
            if (value & 0x0080000) SPReg.Status &= ~0x1000;
            if (value & 0x0100000) SPReg.Status |=  0x1000;
            if (value & 0x0200000) SPReg.Status &= ~0x2000;
            if (value & 0x0400000) SPReg.Status |=  0x2000;
            if (value & 0x0800000) SPReg.Status &= ~0x4000;
            if (value & 0x1000000) SPReg.Status |=  0x4000;
            if (SPReg.Status & 1) return;
            CPU_RSPCycles = 3001;
            if (!HLETaskActive) {
                CPU_ActionFlags |= 0x4000;
                return;
            }
            switch (ReadPhysicalWord(HLETaskAddress)) {
            case 1: {
                NotifyTools(4, 1);
                __try {
                    do {
                        if (SelectedGraphicsPlugin->ProcessDList && !CPU_ForceCFB)
                            SelectedGraphicsPlugin->ProcessDList();
                        if (!ActivePause) break;
                        if (CPU_ActionFlags & 0x200000) {
                            if (SelectedGraphicsPlugin->UpdateScreen)
                                SelectedGraphicsPlugin->UpdateScreen();
                            Sleep(100);
                        }
                    } while (ActivePause && (CPU_ActionFlags & 0x200000));
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ReportCPUError("The graphics plugin raised an exception. It might be incompatible with this game");
                }
                ++DisplayListCount;
                if (DisplayListCount > 11) {
                    DWORD now = timeGetTime();
                    if (now != DisplayListLastTime && DisplayListLastTime) {
                        char text[100];
                        sprintf(text, "DL/s: %3.2f", 12.0f / ((float)(now - DisplayListLastTime) * 0.001f));
                        if (!FullScreen) SetStatusText(2, text);
                    }
                    DisplayListCount = 0;
                    DisplayListLastTime = now;
                }
                CPU_ActionFlags |= 0x20;
                NotifyTools(4, 0);
                break;
            }
            case 2:
                NotifyTools(8, 1);
                AUDList(HLETaskAddress);
                NotifyTools(8, 0);
                break;
            case 4:
                NotifyTools(0x20, 1);
                NotifyTools(0x20, 0);
                break;
            case 7:
                break;
            default:
                ReportCPUError("Unhandled RCP List sent");
                break;
            }
            if (SPReg.Status & 0x40) CPU_ActionFlags |= 8;
            SPReg.Status |= 0x201;
            HLETaskActive = 0;
            return;
        case 0x0404001c:
            if (!value) SPReg.Semaphore = 0;
            return;
        }
    }
    if (address == 0x04080000) { SPReg.PC = value & 0xfff; return; }
    if (address == 0x04080004) { SPReg.Bist = value; return; }
    if (address >= 0x04300000 && address <= 0x0430000c) {
        if (address == 0x04300000) {
            MIReg.Mode ^= (MIReg.Mode ^ value) & 0x7f;
            if (value & 0x0080) MIReg.Mode &= ~0x080;
            if (value & 0x0100) MIReg.Mode |=  0x080;
            if (value & 0x0200) MIReg.Mode &= ~0x100;
            if (value & 0x0400) MIReg.Mode |=  0x100;
            if (value & 0x0800) MIReg.Interrupt &= ~0x20;
            if (value & 0x1000) MIReg.Mode &= ~0x200;
            if (value & 0x2000) MIReg.Mode |=  0x200;
            return;
        }
        if (address == 0x0430000c) {
            if (value & 0x001) MIReg.InterruptMask &= ~0x01;
            if (value & 0x002) MIReg.InterruptMask |=  0x01;
            if (value & 0x004) MIReg.InterruptMask &= ~0x02;
            if (value & 0x008) MIReg.InterruptMask |=  0x02;
            if (value & 0x010) MIReg.InterruptMask &= ~0x04;
            if (value & 0x020) MIReg.InterruptMask |=  0x04;
            if (value & 0x040) MIReg.InterruptMask &= ~0x08;
            if (value & 0x080) MIReg.InterruptMask |=  0x08;
            if (value & 0x100) MIReg.InterruptMask &= ~0x10;
            if (value & 0x200) MIReg.InterruptMask |=  0x10;
            if (value & 0x400) MIReg.InterruptMask &= ~0x20;
            if (value & 0x800) MIReg.InterruptMask |=  0x20;
            return;
        }
    }
    if (address >= 0x04500000 && address <= 0x04500014) {
        if (address == 0x04500004) {
            if (!AIReg.Length) {
                AIReg.Length = value & 0x3ffff;
                UpdateAudioTiming();
                AISetReg(0, 0);
            } else {
                AIReg.NextLength = value & 0x3ffff;
                AIReg.Status |= 0x80000000;
            }
            return;
        }
        if (address == 0x0450000c) MIReg.Interrupt &= ~4;
        AISetReg(address, value);
        return;
    }
    if (address >= 0x04400000 && address <= 0x04400034) {
        switch (address) {
        case 0x04400000:
            if (VIReg.Status != value) {
                VIReg.Status = value;
                if (SelectedGraphicsPlugin->ViStatusChanged)
                    SelectedGraphicsPlugin->ViStatusChanged();
            }
            return;
        case 0x04400004: VIReg.Origin = value; return;
        case 0x04400008:
            if (VIReg.Width != value) {
                VIReg.Width = value;
                if (SelectedGraphicsPlugin->ViWidthChanged)
                    SelectedGraphicsPlugin->ViWidthChanged();
            }
            return;
        case 0x0440000c: VIReg.Interrupt = value; return;
        case 0x04400010: VIReg.Current = value; MIReg.Interrupt &= ~8; return;
        case 0x04400014: VIReg.Burst = value; return;
        case 0x04400018: VIReg.VSync = value; CPU_VideoPeriod = value * 1500; return;
        case 0x0440001c: VIReg.HSync = value; return;
        case 0x04400020: VIReg.Leap = value; return;
        case 0x04400024: VIReg.HStart = value; return;
        case 0x04400028: VIReg.VStart = value; return;
        case 0x0440002c: VIReg.VBurst = value; return;
        case 0x04400030: VIReg.XScale = value; return;
        case 0x04400034: VIReg.YScale = value; return;
        }
    }
    if (address >= 0x03f00000 && address <= 0x03ffffff) {
        switch ((address & 0xff) | 0x03f00000) {
        case 0x03f00000: RDRAMReg.Config = value; return;
        case 0x03f00004: RDRAMReg.DeviceID = value; return;
        case 0x03f00008: RDRAMReg.Delay = value; return;
        case 0x03f0000c: RDRAMReg.Mode = value; return;
        case 0x03f00010: RDRAMReg.RefreshInterval = value; return;
        case 0x03f00014: RDRAMReg.RefreshRow = value; return;
        case 0x03f00018: RDRAMReg.RASInterval = value; return;
        case 0x03f0001c: RDRAMReg.MinInterval = value; return;
        case 0x03f00020: RDRAMReg.AddressSelect = value; return;
        case 0x03f00024: RDRAMReg.Manufacturer = value; return;
        }
    }
    if (address >= 0x04600000 && address <= 0x04600030) {
        switch (address) {
        case 0x04600000: PIReg.DramAddress = value & 0xfffffffc; return;
        case 0x04600004: PIReg.CartAddress = value & 0xfffffffe; return;
        case 0x04600008:
            PIReg.ReadLength = value;
            if (PIReg.CartAddress >= 0x08000000 && PIReg.CartAddress <= 0x0803ffff) {
                unsigned offset = (PIReg.CartAddress & 0xfffffffe) - 0x08000000;
                if (offset < 0x20000 && FlashEnabled)
                    FlashRamAddress = PIReg.DramAddress;
                else
                    WriteSRAM(offset, RDRAM + PIReg.DramAddress, (value + 1) & 0xffffff);
                CPU_ActionFlags |= 4;
            } else {
                ReportMemoryError("PI read (rdram to unhandled memory): ", PIReg.CartAddress);
                CPU_ActionFlags |= 4;
            }
            return;
        case 0x0460000c: {
            PIReg.WriteLength = value;
            unsigned length = (value + 1) & 0xffffff;
            unsigned dram = PIReg.DramAddress;
            if (dram + length >= RDRAMSize - 1)
                length = RDRAMSize - 1 - dram;
            InvalidateRecompiledRange(dram, length);
            unsigned cart = PIReg.CartAddress;
            if (cart >= 0x10000000 && cart <= 0x1fbfffff) {
                cart -= 0x10000000;
                if (cart + length > ROMSize) {
                    if (cart >= ROMSize) return;
                    length = ROMSize - 1 - cart;
                }
                CopyByteSwapped(RDRAM + dram, ROMImage + cart, length);
                if (Dynarec_Mode == 0 || Dynarec_Mode == 5) {
                    unsigned first = dram & 0xfffff000;
                    unsigned size = ((dram + length) | 0xfff) - first + 1;
                    memset(Dynarec_BlockCode + (first >> 2), 0, size);
                }
            } else if (cart >= 0x08000000 && cart <= 0x0803ffff) {
                ReadSRAM(RDRAM + dram, cart - 0x08000000, length);
                if (FlashEnabled && cart == 0x08000000 && FlashCommand != 0xf0000000) {
                    *(unsigned *)(RDRAM + dram) = FlashStatus;
                    *(unsigned *)(RDRAM + dram + 4) = 0x00c2001d;
                }
            } else if (cart < RDRAMSize) {
                CopyByteSwapped(RDRAM + dram, RDRAM + cart, length);
            } else {
                ReportMemoryError("PI write (unhandled cart memory to rdram): ", cart);
            }
            CPU_ActionFlags |= 4;
            return;
        }
        case 0x04600010:
            if (value & 2) MIReg.Interrupt &= ~0x10;
            return;
        case 0x04600014: PIReg.Domain1Latency = value & 0xff; return;
        case 0x04600018: PIReg.Domain1PulseWidth = value & 0xff; return;
        case 0x0460001c: PIReg.Domain1PageSize = value & 0x0f; return;
        case 0x04600020: PIReg.Domain1Release = value & 3; return;
        case 0x04600024: PIReg.Domain2Latency = value & 0xff; return;
        case 0x04600028: PIReg.Domain2PulseWidth = value & 0xff; return;
        case 0x0460002c: PIReg.Domain2PageSize = value & 0x0f; return;
        case 0x04600030: PIReg.Domain2Release = value & 3; return;
        }
    }
    if (address >= 0x04800000 && address <= 0x04800018) {
        switch (address) {
        case 0x04800000: SIAddress = value & 0xffffff; return;
        case 0x04800004:
            ProcessPIF();
            InvalidateRecompiledRange(SIAddress, 64);
            CopyAndSwapWords(RDRAM + SIAddress, PIFRam, 64);
            CPU_ActionFlags |= 2;
            return;
        case 0x04800010:
            CopyAndSwapWords(PIFRam, RDRAM + SIAddress, 64);
            CPU_ActionFlags |= 2;
            ProcessPIF();
            return;
        case 0x04800018: MIReg.Interrupt &= ~2; return;
        }
    }
    if (address >= 0x04100000 && address <= 0x0410001c) {
        if (address == 0x04100000) {
            DPCReg.Start = value & 0xffffff;
            DPCReg.Current = value & 0xffffff;
            return;
        }
        if (address == 0x04100004) {
            DPCReg.End = value & 0xffffff;
            while (DPCReg.Current < DPCReg.End) {
                unsigned first, second;
                if (DPCReg.Status & 1) {
                    first = ReadPhysicalWord(DPCReg.Current + 0x04000000);
                    second = ReadPhysicalWord(DPCReg.Current + 0x04000004);
                } else {
                    first = ReadPhysicalWord(DPCReg.Current);
                    second = ReadPhysicalWord(DPCReg.Current + 4);
                }
                if ((first >> 26) == 0x3a) CPU_ActionFlags |= 0x20;
                RDPHistory[RDPHistoryPosition & 0x7ff] = first;
                RDPHistoryPosition = (RDPHistoryPosition & 0x7ff) + 1;
                RDPHistory[RDPHistoryPosition & 0x7ff] = second;
                RDPHistoryPosition = (RDPHistoryPosition & 0x7ff) + 1;
                if ((first >> 26) == 0x3d && SelectedGraphicsPlugin->FillRectHack)
                    SelectedGraphicsPlugin->FillRectHack();
                DPCReg.Current += 8;
            }
            return;
        }
        if (address == 0x0410000c) {
            if (value & 0x001) DPCReg.Status &= ~1;
            if (value & 0x002) DPCReg.Status |= 1;
            if (value & 0x004) DPCReg.Status &= ~2;
            if (value & 0x010) DPCReg.Status &= ~4;
            if (value & 0x020) DPCReg.Status |= 4;
            if (value & 0x040) DPCReg.Tmem = 0;
            if (value & 0x080) DPCReg.PipeBusy = 0;
            if (value & 0x100) DPCReg.BufferBusy = 0;
            if (value & 0x200) DPCReg.Clock = 0;
            return;
        }
    }
    if (address >= 0x08000000 && address <= 0x0803ffff) {
        FlashEnabled = 1;
        FlashCommand = 0;
        if (value & 0xffff) FlashCommand = value;
        if ((value & 0xff000000) == 0x4b000000) FlashCommand = value;
        if (value == 0xd2000000) {
            if ((FlashStatus & 0xffff) == 0x80) FlashStatus = 0x00800080;
            else FlashStatus = (FlashStatus & 0xffff0000) | 0x80;
        }
        if (value == 0xe1000000) FlashStatus = 0x11118001;
        if (value == 0xf0000000) FlashCommand = value;
        if ((value & 0xff000000) == 0x78000000) FlashStatus = 0x00080008;
        if ((value & 0xff000000) == 0xa5000000) {
            WriteSRAM((value & 0xffff) << 7, RDRAM + FlashRamAddress, 128);
            FlashStatus = 0x00040004;
        }
        return;
    }
    if (address >= 0x10000000 && address <= 0x1fbfffff && AllowRomWrites) {
        if (address - 0x10000000 < ROMSize)
            *(unsigned *)(ROMImage + address - 0x10000000) = value;
        if (address - 0x13fcb4d0 < ROMSize)
            *(unsigned *)(ROMImage + address - 0x13fcb4d0) = value;
    } else if (address >= RDRAMSize + 0x400000) {
        ReadError = 1;
        ReportMemoryError("Trying to Write32 to unhandled or invalid memory", address);
    }
}

// Original 00419550.
void __stdcall WritePhysicalDoubleWord(unsigned address, unsigned lowWord, unsigned highWord)
{
    WritePhysicalWord(address, highWord);
    WritePhysicalWord(address + 4, lowWord);
}

// Original 00419580.
unsigned __stdcall TranslateTLB(unsigned address, unsigned char writing)
{
    unsigned index = 0;
    TranslationError = 0;
    for (; index < 32; ++index) {
        if (!TLB[index].Global) continue;
        unsigned page = (address & TLB[index].AddressMask) >> TLB[index].PageShift;
        if (TLB[index].VPN != (page >> 1)) continue;
        if ((page & 1) == 0) {
            if (!TLB[index].Valid0)
                TranslationError = 2;
            else if (writing && !TLB[index].Dirty0)
                TranslationError = 4;
            else
                return (TLB[index].PFN0 << TLB[index].PFNShift) |
                       (address & TLB[index].OffsetMask);
        } else {
            if (!TLB[index].Valid1)
                TranslationError = 3;
            else if (writing && !TLB[index].Dirty1)
                TranslationError = 5;
            else
                return (TLB[index].PFN1 << TLB[index].PFNShift) |
                       (address & TLB[index].OffsetMask);
        }
        goto failed;
    }
    TranslationError = 1;
failed:
    TLBAddress = address;
    TLBWriteAccess = writing;
    return 0x800001;
}
