/* In-memory defaults for the ordinary static build. Reads never extract files.
   The first explicit write seeds a missing file before applying the edit. */
#include "StaticFiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include "StaticDefaults.inc"

static bool missing(const char *filename)
{
    if (!filename || GetFileAttributesA(filename) != INVALID_FILE_ATTRIBUTES) return false;
    DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

extern "C" const char *__cdecl NemuStaticDefaultData(const char *filename, unsigned int *size)
{
    if (!missing(filename)) return NULL;
    const char *name = filename;
    for (const char *p = filename; *p; ++p)
        if (*p == '\\' || *p == '/') name = p + 1;
    for (unsigned int i = 0; i < sizeof(defaultFiles) / sizeof(defaultFiles[0]); ++i)
        if (!lstrcmpiA(name, defaultFiles[i].name)) {
            *size = defaultFiles[i].size;
            return defaultFiles[i].data;
        }
    return NULL;
}

static bool seed(const char *filename)
{
    unsigned int size;
    const char *data = NemuStaticDefaultData(filename, &size);
    if (!data) return true;
    HANDLE file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS;
    DWORD written;
    bool ok = WriteFile(file, data, size, &written, NULL) && written == size;
    DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!ok) { DeleteFileA(filename); SetLastError(error); }
    return ok;
}

static std::string trim(const std::string &text)
{
    unsigned int begin = 0, end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t')) ++begin;
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) --end;
    return text.substr(begin, end - begin);
}
struct IniKey { std::string name, value; };
struct IniSection { std::string name; std::vector<IniKey> keys; };

static std::vector<IniSection> parse(const char *data, unsigned int size)
{
    std::vector<IniSection> sections;
    int current = -1;
    for (unsigned int pos = 0; pos < size;) {
        unsigned int end = pos;
        while (end < size && data[end] != '\n' && data[end] != '\x1a') ++end;
        std::string line = trim(std::string(data + pos, end - pos));
        pos = end < size && data[end] == '\x1a' ? size : end + 1;
        if (line.empty() || line[0] == ';' || line.compare(0, 2, "//") == 0) continue;
        if (line[0] == '[') {
            unsigned int close = line.find(']');
            current = -1;
            if (close == std::string::npos) continue;
            std::string name = trim(line.substr(1, close - 1));
            bool duplicate = false;
            for (unsigned int i = 0; i < sections.size(); ++i)
                if (!lstrcmpiA(name.c_str(), sections[i].name.c_str())) duplicate = true;
            if (duplicate) continue;
            IniSection section; section.name = name;
            sections.push_back(section); current = sections.size() - 1;
        } else if (current >= 0) {
            unsigned int equal = line.find('=');
            if (equal == std::string::npos) continue;
            IniKey key; key.name = trim(line.substr(0, equal)); key.value = trim(line.substr(equal + 1));
            bool duplicate = false;
            for (unsigned int i = 0; i < sections[current].keys.size(); ++i)
                if (!lstrcmpiA(key.name.c_str(), sections[current].keys[i].name.c_str())) duplicate = true;
            if (!duplicate) sections[current].keys.push_back(key);
        }
    }
    return sections;
}

extern "C" DWORD WINAPI NemuStaticGetPrivateProfileStringA(LPCSTR section, LPCSTR key,
    LPCSTR fallback, LPSTR buffer, DWORD capacity, LPCSTR filename)
{
    unsigned int size;
    const char *data = NemuStaticDefaultData(filename, &size);
    if (!data) return GetPrivateProfileStringA(section, key, fallback, buffer, capacity, filename);
    if (!buffer || !capacity) return 0;
    std::vector<IniSection> sections = parse(data, size);
    if (!section || !key) {
        std::string values;
        for (unsigned int i = 0; i < sections.size(); ++i) {
            if (!section) { values += sections[i].name; values += '\0'; }
            else if (!lstrcmpiA(section, sections[i].name.c_str()))
                for (unsigned int j = 0; j < sections[i].keys.size(); ++j) {
                    values += sections[i].keys[j].name; values += '\0';
                }
        }
        if (capacity == 1) { buffer[0] = 0; return 0; }
        unsigned int count = values.size();
        if (count > capacity - 2) {
            memcpy(buffer, values.data(), capacity - 2);
            buffer[capacity - 2] = buffer[capacity - 1] = 0;
            return capacity - 2;
        }
        memcpy(buffer, values.data(), count); buffer[count] = 0;
        if (!count) buffer[1] = 0;
        return count;
    }
    std::string value = fallback ? fallback : "";
    // Win32 trims trailing spaces from the caller's default value.
    while (!value.empty() && value[value.size() - 1] == ' ') value.resize(value.size() - 1);
    for (unsigned int i = 0; i < sections.size(); ++i)
        if (!lstrcmpiA(section, sections[i].name.c_str()))
            for (unsigned int j = 0; j < sections[i].keys.size(); ++j)
                if (!lstrcmpiA(key, sections[i].keys[j].name.c_str())) {
                    value = sections[i].keys[j].value;
                    if (value.size() >= 2 && (value[0] == '\'' || value[0] == '"') && value[value.size() - 1] == value[0])
                        value = value.substr(1, value.size() - 2);
                }
    unsigned int count = value.size();
    if (count >= capacity) count = capacity - 1;
    memcpy(buffer, value.data(), count); buffer[count] = 0;
    return count;
}

static int hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
extern "C" BOOL WINAPI NemuStaticGetPrivateProfileStructA(LPCSTR section, LPCSTR key,
    LPVOID result, UINT size, LPCSTR filename)
{
    unsigned int ignored;
    if (!NemuStaticDefaultData(filename, &ignored))
        return GetPrivateProfileStructA(section, key, result, size, filename);
    std::vector<char> text((size + 1) * 2 + 1);
    if (NemuStaticGetPrivateProfileStringA(section, key, "", &text[0], text.size(), filename) != (size + 1) * 2) return FALSE;
    std::vector<unsigned char> bytes(size + 1);
    unsigned char checksum = 0;
    for (unsigned int i = 0; i <= size; ++i) {
        int high = hex(text[i * 2]), low = hex(text[i * 2 + 1]);
        if (high < 0 || low < 0) return FALSE;
        bytes[i] = high * 16 + low;
        if (i < size) checksum += bytes[i];
    }
    if (checksum != bytes[size]) return FALSE;
    memcpy(result, &bytes[0], size);
    return TRUE;
}
extern "C" BOOL WINAPI NemuStaticWritePrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR filename)
{
    if (section && !seed(filename)) return FALSE;
    return WritePrivateProfileStringA(section, key, value, filename);
}
extern "C" BOOL WINAPI NemuStaticWritePrivateProfileStructA(LPCSTR section, LPCSTR key, LPVOID value, UINT size, LPCSTR filename)
{
    if (section && !seed(filename)) return FALSE;
    return WritePrivateProfileStructA(section, key, value, size, filename);
}

struct NemuStaticText { FILE *disk; const char *data; unsigned int size, pos; bool eof; };
extern "C" NemuStaticText *__cdecl NemuStaticTextOpen(const char *filename)
{
    NemuStaticText *file = new NemuStaticText;
    file->data = NemuStaticDefaultData(filename, &file->size);
    file->disk = file->data ? NULL : fopen(filename, "rt");
    file->pos = 0; file->eof = false;
    if (!file->data && !file->disk) { delete file; return NULL; }
    return file;
}
extern "C" int __cdecl NemuStaticTextEof(NemuStaticText *file)
{ return file->disk ? feof(file->disk) : file->eof; }
static int get(NemuStaticText *file)
{
    if (file->disk) return fgetc(file->disk);
    if (file->pos >= file->size || file->data[file->pos] == '\x1a') { file->eof = true; return EOF; }
    char c = file->data[file->pos++];
    if (c == '\r' && file->pos < file->size && file->data[file->pos] == '\n') { ++file->pos; return '\n'; }
    return (unsigned char)c;
}
extern "C" char *__cdecl NemuStaticTextLine(NemuStaticText *file, char *buffer, unsigned int size)
{
    unsigned int pos = 0;
    int c;
    while (pos + 1 < size && (c = get(file)) != EOF) {
        buffer[pos++] = c;
        if (c == '\n') break;
    }
    if (!pos) return NULL;
    buffer[pos] = 0; return buffer;
}
extern "C" int __cdecl NemuStaticTextToken(NemuStaticText *file, char *buffer, unsigned int size)
{
    int c;
    unsigned int pos = 0;
    do { c = get(file); } while (c != EOF && isspace(c));
    if (c == EOF) return EOF;
    do {
        if (pos + 1 < size) buffer[pos++] = c;
        c = get(file);
    } while (c != EOF && !isspace(c));
    // fscanf("%s\\n") consumes trailing whitespace and looks ahead.
    while (c != EOF && isspace(c)) c = get(file);
    if (c != EOF) {
        if (file->disk) ungetc(c, file->disk); else --file->pos;
    }
    buffer[pos] = 0; return 1;
}
extern "C" void __cdecl NemuStaticTextClose(NemuStaticText *file)
{
    if (file->disk) fclose(file->disk);
    delete file;
}
