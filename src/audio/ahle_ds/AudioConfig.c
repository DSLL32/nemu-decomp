/* Configuration dialog for the DirectSound unit. The original source filename
   is not retained; control IDs and every branch come from audio.dll. */
#include "DirectSound.h"
#include <commctrl.h>

char AudioAboutText[256]; /* 1016bfe0 */
/* 10005760–10005f70, including switch tables and alignment. */
BOOL CALLBACK AudioConfigDialog(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    int oldRecordHLE = RecordHLE;
    int oldRecordAI = RecordAI;
    int oldRecordABI = RecordABI;
    int oldDumpMicrocode = DumpMicrocode;
    int position;
    switch (message) {
    case WM_INITDIALOG:
        SendDlgItemMessage(window, 1036, TBM_SETRANGEMIN, TRUE, 0);
        SendDlgItemMessage(window, 1036, TBM_SETRANGEMAX, TRUE, 3);
        SendDlgItemMessage(window, 1036, TBM_SETTICFREQ, TRUE, 0);
        switch (RequestedBufferSize) {
        case 0x4000: position = 0; break;
        case 0x8000: position = 1; break;
        case 0xc000: position = 2; break;
        case 0x10000: position = 3; break;
        default: RequestedBufferSize = 0x8000; position = 1; break;
        }
        SendDlgItemMessage(window, 1036, TBM_SETPOS, TRUE, position);
        CheckDlgButton(window, 1005, DummyMode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1006, DebugMode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1015, RecordHLE ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1016, RecordAI ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1014, DumpMicrocode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1019, FilterHLE ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1024, DirectHLEAudio ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1025, StereoReversed ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1026, Prebuffer ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1031, SyncAudio ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1029, HighQuality ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    case WM_NOTIFY:
        switch (SendDlgItemMessage(window, 1036, TBM_GETPOS, 0, 0)) {
        case 0: RequestedBufferSize = 0x4000; return TRUE;
        case 1: RequestedBufferSize = 0x8000; return TRUE;
        case 2: RequestedBufferSize = 0xc000; return TRUE;
        case 3: RequestedBufferSize = 0x10000; return TRUE;
        }
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDCANCEL:
            EndDialog(window, wparam);
            return TRUE;
        case IDOK:
            RecordHLE = IsDlgButtonChecked(window, 1015) ? 1 : 0;
            RecordAI = IsDlgButtonChecked(window, 1016) ? 1 : 0;
            DumpMicrocode = IsDlgButtonChecked(window, 1014) ? 1 : 0;
            FilterHLE = IsDlgButtonChecked(window, 1019) ? 1 : 0;
            if (RecordHLE != oldRecordHLE) {
                if (HLEWaveFile) fclose(HLEWaveFile);
                else {
                    HLEWaveFile = fopen("hle.wav", "wb");
                    if (!HLEWaveFile) RecordHLE = 0;
                    WriteWaveHeader(HLEWaveFile);
                }
            }
            if (RecordAI != oldRecordAI) {
                if (AIWaveFile) fclose(AIWaveFile);
                else {
                    AIWaveFile = fopen("ai.wav", "wb");
                    if (!AIWaveFile) RecordAI = 0;
                    WriteWaveHeader(AIWaveFile);
                }
            }
            if (DumpMicrocode != oldDumpMicrocode) {
                if (HLELogFile) fclose(HLELogFile);
                else {
                    HLELogFile = fopen("g:/roms/n64/hle.log", "wb");
                    if (!HLELogFile) DumpMicrocode = 0;
                }
            }
            if (RecordABI != oldRecordABI) {
                if (ABIFile) fclose(ABIFile);
                else {
                    ABIFile = fopen("Record.Abi", "wb");
                    if (!ABIFile) RecordAI = 0;
                }
            }
            lstrcpy(AudioIniKey, "ScrollBuffSize");
            WriteIniInt(AudioIniSection, AudioIniKey, RequestedBufferSize, AudioIniFile);
            DummyMode = IsDlgButtonChecked(window, 1005) ? 1 : 0;
            lstrcpy(AudioIniKey, "DummyMode");
            WriteIniInt(AudioIniSection, AudioIniKey, DummyMode, AudioIniFile);
            DirectHLEAudio = IsDlgButtonChecked(window, 1024) ? 1 : 0;
            lstrcpy(AudioIniKey, "HLEAudio");
            WriteIniInt(AudioIniSection, AudioIniKey, DirectHLEAudio, AudioIniFile);
            StereoReversed = IsDlgButtonChecked(window, 1025) ? 1 : 0;
            lstrcpy(AudioIniKey, "StereoReversed");
            WriteIniInt(AudioIniSection, AudioIniKey, StereoReversed, AudioIniFile);
            Prebuffer = IsDlgButtonChecked(window, 1026) ? 1 : 0;
            if (Prebuffer) ReleaseAudioBlock(0);
            lstrcpy(AudioIniKey, "HlePreBuffer");
            WriteIniInt(AudioIniSection, AudioIniKey, Prebuffer, AudioIniFile);
            HighQuality = IsDlgButtonChecked(window, 1029) ? 1 : 0;
            lstrcpy(AudioIniKey, "HighQuality");
            WriteIniInt(AudioIniSection, AudioIniKey, HighQuality, AudioIniFile);
            SyncAudio = IsDlgButtonChecked(window, 1031) ? 1 : 0;
            lstrcpy(AudioIniKey, "SyncAudio");
            WriteIniInt(AudioIniSection, AudioIniKey, SyncAudio, AudioIniFile);
            if (IsDlgButtonChecked(window, 1006)) {
                DebugMode = 1;
                OpenAudioLog();
            } else {
                DebugMode = 0;
                CloseAudioLog();
            }
            EndDialog(window, wparam);
            return TRUE;
        case 1033:
            ShowDriverInfo();
            return TRUE;
        case 1042:
            SendDlgItemMessage(window, 1036, TBM_SETPOS, TRUE, 1);
            CheckDlgButton(window, 1005, 0);
            CheckDlgButton(window, 1006, 0);
            CheckDlgButton(window, 1015, 0);
            CheckDlgButton(window, 1016, 0);
            CheckDlgButton(window, 1014, 0);
            CheckDlgButton(window, 1019, 0);
            CheckDlgButton(window, 1024, 1);
            CheckDlgButton(window, 1025, 0);
            CheckDlgButton(window, 1026, 1);
            CheckDlgButton(window, 1031, 1);
            CheckDlgButton(window, 1029, 1);
            return TRUE;
        case 1035:
            sprintf(AudioAboutText, "%s\nCompiled on:%s %5s\n\n%s",
                    AudioDescription, "Mar 22 2003", "23:20:37", AudioCredits);
            MessageBox(window, AudioAboutText, "About", MB_OK);
            return TRUE;
        }
        break;
    case WM_HSCROLL:
        return TRUE;
    }
    return FALSE;
}
/* 10005f70–10005fb0 */
void AIConfigure(HWND window)
{
    DialogBox(AudioInstance, MAKEINTRESOURCE(101), window, AudioConfigDialog);
    if (SoundBufferSize != RequestedBufferSize)
        MessageBox(NULL, "You have made changes that will not be effective until you reload a rom",
                   "Audio Warning", MB_OK);
}
