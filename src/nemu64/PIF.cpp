#include "PIF.h"
#include "Memory.h"
#include "CPU.h"
#include <string.h>


inline unsigned ByteSwap(unsigned value)
{
    unsigned result;
    __asm { mov eax, value
            bswap eax
            mov result, eax }
    return result;
}

void FindCICInput(FILE *file, const unsigned char *data)
{
    unsigned char previous[16];
    while (fread(previous, 1, 16, file)) {
        if (memcmp(data, previous, 16) == 0) {
            fseek(file, -16, SEEK_CUR);
            return;
        }
    }
}

void SaveCICInput(const unsigned char *data)
{
    FILE *file = fopen("d:\\n64\\lastin.cic", "r+b");
    if (!file) {
        file = fopen("d:\\n64\\lastin.cic", "wb");
        if (!file) return;
    }
    fseek(file, 0, SEEK_SET);
    FindCICInput(file, data);
    fwrite(data, 1, 16, file);
    fclose(file);
}

int CICUseTable1(const unsigned char *response)
{
    unsigned swapped = ByteSwap(*(const unsigned *)response);
    unsigned key = (((swapped >> 4) & 0xf00000) | (swapped & 0xf0000)) >> 4;
    key |= (swapped & 0xf) << 4;
    key |= swapped & 0xf00;
    for (int i = 0; i < 32768; ++i) {
        unsigned inputWord = ByteSwap(PifResponseHistory[i]);
        if (key == (inputWord & 0xfffffff0)) return 1;
    }
    return 0;
}

void CIC6105Challenge(unsigned char *command)
{
    unsigned char response[32];
    unsigned char challenge[32];
    if (CIC6105Table) {
        unsigned char *data = command + 2;
        SaveCICInput(data);
        for (unsigned i = 0; i < 15; ++i) {
            challenge[i * 2] = data[i] >> 4;
            challenge[i * 2 + 1] = data[i] & 15;
        }
        int swapped = ByteSwap(*(unsigned *)data);
        unsigned inputWord = ByteSwap(CIC6105Table[(swapped >> 12) & 0xfffff]);
        *(int *)data = inputWord;
        response[0] = (inputWord >> 28) & 15;
        response[1] = (inputWord >> 24) & 15;
        response[2] = (inputWord >> 20) & 15;
        response[3] = (inputWord >> 16) & 15;
        response[4] = (inputWord >> 12) & 15;
        for (unsigned j = 0; j + 5 < 31; ++j) {
            response[j + 5] = PifResponseTable0[(challenge[j + 5] & 15) + ((response[j + 4] & 15) << 4)];
            if (response[j + 4] == 1 || response[j + 4] == 9 ||
                response[j + 4] == 11 || response[j + 4] == 14) {
                if (CICUseTable1(response + j))
                    response[j + 5] = PifResponseTable1[(challenge[j + 5] & 15) + ((response[j + 4] & 15) << 4)];
            }
        }
        for (unsigned k = 0; k < 15; ++k)
            data[k] = (response[k * 2 + 1] & 15) | (response[k * 2] << 4);
    }
}

unsigned char PIFDataCRC(const unsigned char *data)
{
    unsigned char crc = 0;
    for (int i = 0; i < 33; ++data, ++i) {
        for (int bit = 7; bit > -1; --bit) {
            unsigned char polynomial = (crc & 0x80) ? 0x85 : 0;
            crc <<= 1;
            if (i != 32)
                crc |= ((*data & (1 << bit)) != 0);
            crc ^= polynomial;
        }
    }
    return crc;
}

void InitializePIFCommandLengths()
{
    PIFCommandLengths[0].Tx = 1;
    PIFCommandLengths[0].Rx = 3;
    PIFCommandLengths[1].Tx = 1;
    PIFCommandLengths[1].Rx = 4;
    PIFCommandLengths[2].Tx = 3;
    PIFCommandLengths[2].Rx = 33;
    PIFCommandLengths[3].Tx = 35;
    PIFCommandLengths[3].Rx = 1;
    PIFCommandLengths[4].Tx = 2;
    PIFCommandLengths[4].Rx = 8;
    PIFCommandLengths[5].Tx = 10;
    PIFCommandLengths[5].Rx = 1;
    PIFCommandLengths[255].Tx = 1;
    PIFCommandLengths[255].Rx = 3;
}

void ProcessPIF()
{
    InitializePIFCommandLengths();
    int position = 0;
    int inputUpdated = 0;
    int channel = -1;
    unsigned char tx, rx;
    if ((unsigned)CPU_PC >= 0xa4001000 && (unsigned)CPU_PC < 0xa4001fff) {
        PIFRam[63] |= 0x80;
        PIFRam[36] = 0;
        PIFRam[37] = 4;
        PIFRam[38] = 0x3f;
        PIFRam[39] = 0x3f;
        return;
    }
    if (CICState && !(PIFRam[63] & 2)) {
        CICState = 0;
        return;
    }
    if (PIFRam[63] & 2) {
        if (PIFRam[46] == 15 && PIFRam[47] == 15) {
            CIC6105Challenge(PIFRam + 46);
            PIFRam[63] &= 0xfc;
            CICState = 1;
            return;
        }
    }
    if (PIFRam[0] != 0xfe) {
        while (position < 64 && channel < 6) {
            while (PIFRam[position] == 0xff || (PIFRam[position] == 0 && channel < 6)) {
                if (PIFRam[position] == 0 || PIFRam[position] == 0xfd) ++channel;
                ++position;
            }
            tx = PIFRam[position];
            if (tx == 0xfe || channel > 4) break;
            rx = PIFRam[position + 1] & 0x3f;
            position += 2;
            ++channel;
            if (tx > 64 - position) tx = 64 - position;
            if (tx > 64) tx = 64;
            memcpy(PIFCommand, PIFRam + position, tx);
            if (PIFCommand[0] == 255) PIFCommand[0] = 0;
            position += tx;
            int valid = tx > 0;
            if (rx > PIFCommandLengths[PIFCommand[0]].Rx) {
                PIFRam[position - tx - 1] |= 0x40;
                rx = PIFCommandLengths[PIFCommand[0]].Rx;
            }
            if (tx > PIFCommandLengths[PIFCommand[0]].Tx) {
                PIFRam[position - tx - 1] |= 0x40;
                valid = 0;
            }
            if (valid) {
                switch (PIFCommand[0]) {
                case 0:
                    if (channel < 4) {
                        if ((!NetPlayActive && Controllers[channel].Present) ||
                            (NetPlayActive && channel < NetPlayPlayerCount)) {
                            PIFResponse[0] = 5;
                            PIFResponse[1] = 0;
                            PIFResponse[2] = (!NetPlayActive &&
                                (Controllers[channel].MemPak || Controllers[channel].RumblePak)) ? 1 : 2;
                            memcpy(PIFRam + position, PIFResponse, rx);
                        } else PIFRam[position - 2] |= 0x80;
                    } else if (channel < 6) {
                        if (EEPROMEnabled) {
                            PIFResponse[0] = 0;
                            PIFResponse[1] = EEPROMSize == 0x800 ? 0xc0 : 0x80;
                            PIFResponse[2] = 0;
                            memcpy(PIFRam + position, PIFResponse, rx);
                        } else PIFRam[position - 2] |= 0x80;
                    } else if (rx) PIFRam[position - 2] |= 0x80;
                    break;
                case 1:
                    if (channel < 4) {
                        if ((!NetPlayActive && Controllers[channel].Present) ||
                            (NetPlayActive && channel < NetPlayPlayerCount)) {
                            if (!inputUpdated && !NetPlayActive) {
                                CIGetJoyData();
                                inputUpdated = 1;
                            }
                            unsigned buttons = Controllers[channel].Buttons;
                            PIFResponse[0] = buttons >> 24;
                            PIFResponse[1] = buttons >> 16;
                            PIFResponse[2] = buttons >> 8;
                            PIFResponse[3] = buttons;
                            if (MovieRecord) RecordMovieInput();
                            else if (MoviePlay) PlayMovieInput();
                            memcpy(PIFRam + position, PIFResponse, rx);
                        } else if (rx) PIFRam[position - 2] |= 0x80;
                    } else PIFRam[position - tx - 1] |= 0x40;
                    break;
                case 2:
                    if (channel < 4) {
                        if ((!NetPlayActive && Controllers[channel].Present) ||
                            (NetPlayActive && channel < NetPlayPlayerCount)) {
                            int address = ((PIFCommand[1] << 8) | PIFCommand[2]) >> 5 & 0xffff;
                            if (address >= 0x400) {
                                for (int i = 0; i < 32; ++i)
                                    PackBlock[i] = !NetPlayActive && Controllers[channel].RumblePak ? 0x80 : 0;
                            } else if (!NetPlayActive && Controllers[channel].RumblePak) {
                                memset(PackBlock, 0x80, 32);
                            } else {
                                memcpy(PackBlock, MemPackData + address * 32, 32);
                                MemPackUsed = 1;
                            }
                            memcpy(PIFRam + position, PackBlock, 32);
                            if (!NetPlayActive && (Controllers[channel].MemPak || Controllers[channel].RumblePak))
                                PIFRam[position + rx - 1] = PIFDataCRC(PackBlock);
                            else PIFRam[position + rx - 1] = ~PIFDataCRC(PackBlock);
                        } else PIFRam[position - tx - 1] |= 0x80;
                    } else PIFRam[position - tx - 1] |= 0x40;
                    break;
                case 3:
                    if (channel < 4) {
                        if ((!NetPlayActive && Controllers[channel].Present) ||
                            (NetPlayActive && channel < NetPlayPlayerCount)) {
                            int address = ((PIFCommand[1] << 8) | PIFCommand[2]) >> 5 & 0xffff;
                            if (tx - 3 > 32) tx = 35;
                            if (tx - 3 < 0) MessageBox(0, "PIFRAM command write ram. tx too small.", "", 0);
                            memcpy(PackBlock, PIFCommand + 3, tx - 3);
                            if (address >= 0x400 && address == 0x600 && !NetPlayActive && Controllers[channel].RumblePak) {
                                Controllers[channel].RumbleActive = PIFCommand[5] == 1 && PIFCommand[6] == 1;
                                CIRumble(channel);
                            }
                            if ((!NetPlayActive && Controllers[channel].MemPak) || Controllers[channel].RumblePak) {
                                if (address < 0x400 && !Controllers[channel].RumblePak) {
                                    address *= 32;
                                    if (address + tx - 3 >= 0x8000)
                                        MessageBox(0, "PIFRAM command write ram. MemPackAddr too big.", "", 0);
                                    memcpy(MemPackData + address, PackBlock, tx - 3);
                                    SaveMemPack(address, tx - 3);
                                    MemPackUsed = 1;
                                }
                                PIFRam[position + rx - 1] = PIFDataCRC(PackBlock);
                            } else PIFRam[position + rx - 1] = ~PIFDataCRC(PackBlock);
                        } else PIFRam[position - tx - 1] |= 0x80;
                    } else PIFRam[position - tx - 1] |= 0x40;
                    break;
                case 4:
                    if (channel == 4 || channel == 5) {
                        memcpy(PIFRam + position, EEPROMData + PIFCommand[1] * 8, rx);
                        EEPROMUsed = 1;
                    } else ReportCPUError("Pif command read eeprom (unsupported channel).");
                    break;
                case 5:
                    if (channel == 4 || channel == 5) {
                        memcpy(EEPROMData + PIFCommand[1] * 8, PIFCommand + 2, tx - 2);
                        EEPROMUsed = 1;
                        SaveEEPROM(PIFCommand[1] * 8, tx - 2);
                        PIFRam[position] = 0;
                    } else ReportCPUError("Pif command write eeprom (unsupported channel).");
                    break;
                }
            }
            position += rx;
            if (PIFRam[position] == 0xfe) break;
        }
    }
    if (ForcePIFSuccess) PIFRam[63] = 0x80;
    else {
        PIFRam[63] = 0;
        if (CPU_PC == 0x1fc0056c) PIFRam[63] = 0x80;
    }
}

void SaveEEPROM(unsigned offset, unsigned length)
{
    if (DisableSaveWrites) return;
    FILE *file = fopen(EEPROMFilename, "r+b");
    if (!file) {
        file = fopen(EEPROMFilename, "wb");
        if (!file) return;
        fwrite(EEPROMData, 0x800, 1, file);
        fflush(file);
        fclose(file);
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(EEPROMData + offset, length, 1, file);
    fflush(file);
    fclose(file);
}

void SaveMemPack(unsigned offset, unsigned length)
{
    if (DisableSaveWrites) return;
    FILE *file = fopen(MemPackFilename, "r+b");
    if (!file) {
        file = fopen(MemPackFilename, "wb");
        if (!file) return;
        fwrite(MemPackData, 0x8000, 1, file);
        fflush(file);
        fclose(file);
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(MemPackData + offset, length, 1, file);
    fflush(file);
    fclose(file);
}

void RecordMovieInput()
{
    if (!MovieFile) {
        MovieFile = fopen("$$$temp.tmp", "wb");
        fwrite(&Controllers[0], 1, 8, MovieFile);
        fwrite(&Controllers[1], 1, 8, MovieFile);
        fwrite(&Controllers[2], 1, 8, MovieFile);
        fwrite(&Controllers[3], 1, 8, MovieFile);
        if (!MovieFile) return;
    }
    if (*(unsigned *)PIFResponse == MovieLastButtons && !MovieNewRun) {
        ++MovieRepeatCount;
        fseek(MovieFile, -6, SEEK_CUR);
    } else MovieRepeatCount = 0;
    fwrite(PIFResponse, 1, 4, MovieFile);
    fwrite(&MovieRepeatCount, 1, 2, MovieFile);
    MovieLastButtons = *(unsigned *)PIFResponse;
    MovieNewRun = 0;
}

void PlayMovieInput()
{
    unsigned buttons = 0;
    if (!MovieFile) {
        MovieFile = fopen("$$$temp.tmp", "rb");
        fread(&Controllers[0], 1, 8, MovieFile);
        fread(&Controllers[1], 1, 8, MovieFile);
        fread(&Controllers[2], 1, 8, MovieFile);
        fread(&Controllers[3], 1, 8, MovieFile);
        if (!MovieFile) return;
    }
    if (MovieRepeatCount > 0) {
        --MovieRepeatCount;
        *(unsigned *)PIFResponse = MovieLastButtons;
    } else {
        fread(&buttons, 1, 4, MovieFile);
        fread(&MovieRepeatCount, 1, 2, MovieFile);
        MovieLastButtons = buttons;
        *(unsigned *)PIFResponse = buttons;
    }
}

void StopMovie()
{
    fclose(MovieFile);
    MovieFile = 0;
    MovieNewRun = 1;
    MovieLastButtons = 0;
    MovieRepeatCount = 0;
    MoviePlay = 0;
    MovieRecord = 0;
}

void SaveSRAM(unsigned offset, unsigned length)
{
    if (DisableSaveWrites) return;
    FILE *file = fopen(SRAMFilename, "r+b");
    if (!file) {
        file = fopen(SRAMFilename, "wb");
        if (!file) return;
        fwrite(SRAMData, 0x40000, 1, file);
        fflush(file);
        fclose(file);
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(SRAMData + offset, length, 1, file);
    fflush(file);
    fclose(file);
}

void WriteSRAM(unsigned offset, const void *source, unsigned length)
{
    if (offset + length <= 0x40000) {
        CopyAndSwapWords(SRAMData + offset, source, length);
        SaveSRAM(offset, length);
        SRAMUsed = 1;
    }
}

void ReadSRAM(void *destination, unsigned offset, unsigned length)
{
    if (offset + length <= 0x40000) {
        CopyAndSwapWords(destination, SRAMData + offset, length);
        SRAMUsed = 1;
    }
}
