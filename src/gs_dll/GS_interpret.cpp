#include "StdAfx.h"
#include "GS_interpret.h"
#include "gs_dll.h"
#include "gs_exeDlg.h"
#include "RW_INI_files.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char romname[256] = {0};
unsigned char *RDRAM = 0;
char active_codenames[4096][256] = {0};
struct
{
    unsigned long address;
    unsigned short value;
} ActiveCodes[4096] = {0};
static int NumActiveCodes = 0;
static int NumActiveNames = 0;
static int Initialized = 0;

void CGs_exeDlg::GSUpdateList()
{
    POSITION pos;
    CStringList* list;
    CString str;
    char* text;
    int count, subcount, i, j, index;
    for (i = 0; i < 4096; i++)
    {
        ActiveCodes[i].address = ActiveCodes[i].value = 0;
    }
    index = 0;
    list = &Codes;
    count = ActiveList().GetCount();
    for (i = 0; i < count; i++)
    {
        ActiveList().GetText(i, str);
        strcpy(active_codenames[i], str);
        pos = list->Find(str);
        list->GetNext(pos);
        str = list->GetNext(pos);
        subcount = atoi(str);
        for (j = 0; j < subcount; j++)
        {
            str = list->GetNext(pos);
            text = str.GetBuffer(str.GetLength());
            sscanf(text, "%8X %4X", &ActiveCodes[index].address, &ActiveCodes[index].value);
            index++;
        }
    }
    NumActiveCodes = index;
    NumActiveNames = ActiveList().GetCount();
}

void CGs_exeDlg::UpdateActiveList()
{
    for (int i = 0; i < NumActiveNames; i++)
        ActiveList().AddString(active_codenames[i]);
}

void GSInitialize(char *Header, char *Ram, char *IniDirectory)
{
    char name[256];
    int i;

    if (!Initialized)
        Initialized = 1;
    memset(name, 0, 256);
    memset(romname, 0, 256);
    strncpy(name, Header + 32, 26);
    for (i = 0; i < 26; i += 4)
    {
        romname[i + 3] = name[i];
        romname[i + 2] = name[i + 1];
        romname[i + 1] = name[i + 2];
        romname[i] = name[i + 3];
    }
    i = strlen(romname) - 1;
    while (romname[i] == ' ')
        romname[i--] = 0;
    strcat(romname, ".Cheats");
    inifilename = IniDirectory;
    RDRAM = (unsigned char *)Ram;
    for (i = 0; i < 4096; i++)
    {
        memset(active_codenames[i], 0, 256);
        ActiveCodes[i].address = ActiveCodes[i].value = 0;
    }
    NumActiveCodes = NumActiveNames = 0;
}

void GSConfigure()
{
    if (!Initialized)
        return;
    COleObjectFactory::RegisterAll();
    AfxEnableControlContainer();
    CGs_exeDlg dlg;
    dlg.GameName = romname;
    dlg.DoModal();
}

void GSUpdate()
{
    int i;
    unsigned char type;

    if (!RDRAM)
        return;
    if (!NumActiveCodes)
        return;
    for (i = 0; i < NumActiveCodes; i++)
    {
        type = (ActiveCodes[i].address >> 24) & 255;
        switch (type)
        {
        case 0x80:
        case 0xa0:
            RDRAM[(ActiveCodes[i].address & 0xffffff) ^ 3] = ActiveCodes[i].value & 255;
            break;
        case 0x81:
        case 0xa1:
            *(unsigned short *)(RDRAM + ((ActiveCodes[i].address & 0xffffff) ^ 2)) = ActiveCodes[i].value;
            break;
        case 0xd0:
            if (RDRAM[(ActiveCodes[i].address & 0xffffff) ^ 3] != (ActiveCodes[i].value & 255))
                i++;
            break;
        case 0xd1:
            if (*(unsigned short *)(RDRAM + ((ActiveCodes[i].address & 0xffffff) ^ 2)) != ActiveCodes[i].value)
                i++;
            break;
        case 0xd2:
            if (RDRAM[(ActiveCodes[i].address & 0xffffff) ^ 3] == (ActiveCodes[i].value & 255))
                i++;
            break;
        case 0xd3:
            if (*(unsigned short *)(RDRAM + ((ActiveCodes[i].address & 0xffffff) ^ 2)) == ActiveCodes[i].value)
                i++;
            break;
        }
    }
}
