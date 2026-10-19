/* Included only by the static frontend, after the reconstructed core headers. */
static BOOL __cdecl StaticDescribeGeneratedCode(DWORD address, char *text, unsigned capacity)
{
    const char *kind; DWORD offset;
    if (address >= (DWORD)Dynarec_MainCode && address-(DWORD)Dynarec_MainCode < sizeof(Dynarec_MainCode)) {
        kind = "main"; offset = address-(DWORD)Dynarec_MainCode;
    } else if (address >= (DWORD)Dynarec_FastCode && address-(DWORD)Dynarec_FastCode < sizeof(Dynarec_FastCode)) {
        kind = "fast"; offset = address-(DWORD)Dynarec_FastCode;
    } else return FALSE;
    if (text && capacity) {
        _snprintf(text,capacity,"dynrec!%s_buffer + 0x%lX (current guest PC=%08X)",kind,offset,CPU_PC);
        text[capacity-1] = 0;
    }
    return TRUE;
}
