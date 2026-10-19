/* Execute the small postfix unwind recipes emitted by the VC7 x86 compiler.
   This is an independent bounded evaluator, not a general x86 emulator. */
#pragma pack(push, 1)
struct FpoHeader { char magic[8]; DWORD count, strings, stringSize; };
struct FrameRecipe { DWORD begin, size, locals, params, maximum, program; WORD prolog, saved; DWORD flags; };
#pragma pack(pop)
static const FpoHeader *fpo;
static const FrameRecipe *findRecipe(DWORD pc)
{
    if (!fpo || pc < (DWORD)application) return NULL;
    DWORD rva = pc-(DWORD)application, low = 0, high = fpo->count;
    const FrameRecipe *entries = (const FrameRecipe *)(fpo+1);
    while (low < high) { DWORD mid = low+(high-low)/2; if (entries[mid].begin <= rva) low = mid+1; else high = mid; }
    if (!low || rva-entries[low-1].begin >= entries[low-1].size) return NULL;
    return entries+low-1;
}
static int recipeVariable(const char *name)
{
    static const char *names[] = {"$eax","$ecx","$edx","$ebx","$esp","$ebp","$esi","$edi","$eip",
        "$T0","$T1","$T2","$T3","$T4","$T5","$T6","$T7","$T8","$T9","$L","$P"};
    for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i) if (!lstrcmpA(name,names[i])) return i;
    return -1;
}
struct RecipeValue { DWORD value; int variable; };
static bool recipeValue(RecipeValue item, DWORD *variables, DWORD known, DWORD *value)
{
    if (item.variable < 0) { *value = item.value; return true; }
    if (!(known & (1UL<<item.variable))) return false;
    *value = variables[item.variable]; return true;
}
static bool returnSlot(DWORD start, DWORD *slot, bool *searched)
{
    // Recipes deliberately use .raSearchStart: compiler scheduling can leave
    // outgoing arguments on the stack until a later cleanup. Search only above
    // the metadata's local/saved-register area, and say when search was needed.
    for (unsigned extra = 0; extra <= 1024; extra += 4) {
        DWORD value;
        if (start+extra < start || !readStack(start+extra,&value,4)) return false;
        if (possibleReturn(value)) { *slot = start+extra; if (extra) *searched = true; return true; }
    }
    return false;
}
static bool applyRecipe(const FrameRecipe *frame, DWORD pc, CONTEXT *context, bool *searched)
{
    if (pc-(DWORD)application-frame->begin < frame->prolog) return false;
    const char *program = (const char *)fpo+fpo->strings+frame->program;
    DWORD vars[21] = {context->Eax,context->Ecx,context->Edx,context->Ebx,context->Esp,context->Ebp,
        context->Esi,context->Edi,context->Eip};
    DWORD known = 0x1ff;
    RecipeValue values[32]; unsigned depth = 0, operations = 0;
    while (*program && operations++ < 512) {
        while (*program == ' ') ++program;
        if (!*program) break;
        char token[48]; unsigned length = 0;
        while (*program && *program != ' ') { if (length+1 >= sizeof(token)) return false; token[length++] = *program++; }
        token[length] = 0;
        if (length == 1 && strchr("+-^=",token[0])) {
            DWORD right,left;
            if (!depth || !recipeValue(values[--depth],vars,known,&right)) return false;
            if (token[0] == '^') {
                if (!readStack(right,&right,4)) return false;
                values[depth].value = right; values[depth++].variable = -1;
            } else {
                if (!depth) return false;
                RecipeValue destination = values[--depth];
                if (token[0] == '=') {
                    if (destination.variable < 0) return false;
                    vars[destination.variable] = right; known |= 1UL<<destination.variable;
                } else {
                    if (!recipeValue(destination,vars,known,&left)) return false;
                    values[depth].value = token[0] == '+' ? left+right : left-right;
                    values[depth++].variable = -1;
                }
            }
        } else {
            if (depth == sizeof(values)/sizeof(values[0])) return false;
            RecipeValue &item = values[depth++]; item.variable = -1; item.value = 0;
            if (token[0] == '$') { item.variable = recipeVariable(token); if (item.variable < 0) return false; }
            else if (!lstrcmpA(token,".cbLocals")) item.value = frame->locals;
            else if (!lstrcmpA(token,".cbSavedRegs")) item.value = frame->saved;
            else if (!lstrcmpA(token,".cbParams")) item.value = frame->params;
            else if (!lstrcmpA(token,".cbCalleeParams")) return false; // don't invent missing caller state
            else if (!lstrcmpA(token,".raSearchStart")) {
                DWORD start = (known & (1UL<<11)) ? vars[11] : context->Esp+frame->locals+frame->saved;
                if (!returnSlot(start,&item.value,searched)) return false;
            } else {
                if (!length) return false;
                for (unsigned i = 0; i < length; ++i) {
                    if (token[i] < '0' || token[i] > '9' || item.value > 0x19999999) return false;
                    item.value = item.value*10+token[i]-'0';
                }
            }
        }
    }
    if (depth || operations >= 512 || vars[4] <= context->Esp || vars[4] > crash.stackHigh || !possibleReturn(vars[8])) return false;
    context->Eax=vars[0];context->Ecx=vars[1];context->Edx=vars[2];context->Ebx=vars[3];
    context->Esp=vars[4];context->Ebp=vars[5];context->Esi=vars[6];context->Edi=vars[7];context->Eip=vars[8];
    return true;
}
static void loadFpo(void)
{
    HRSRC resource = FindResourceA(application,"NEMU_CRASH_FPO",RT_RCDATA);
    if (!resource) return;
    DWORD size = SizeofResource(application,resource);
    const FpoHeader *header = (const FpoHeader *)LockResource(LoadResource(application,resource));
    if (!header || size < sizeof(*header) || memcmp(header->magic,"NFPO0001",8) ||
        header->count > (size-sizeof(*header))/sizeof(FrameRecipe) ||
        header->strings != sizeof(*header)+header->count*sizeof(FrameRecipe) || header->strings > size ||
        header->stringSize != size-header->strings) return;
    const FrameRecipe *entries = (const FrameRecipe *)(header+1);
    DWORD previous = 0;
    for (DWORD i = 0; i < header->count; ++i) {
        const FrameRecipe &entry = entries[i];
        if (entry.begin < previous || !entry.size || entry.begin+entry.size < entry.begin ||
            entry.program >= header->stringSize || entry.locals > 1024*1024 || entry.saved > 1024*1024) return;
        previous = entry.begin;
        unsigned available = header->stringSize-entry.program; if (available > 2049) available = 2049;
        if (!memchr((const char *)header+header->strings+entry.program,0,available)) return;
    }
    fpo = header;
}
