// vaz_coef_dump.cpp — LoadLibrary Vaz2010Core.dll (32-bit) and call the reverb/decimator coefficient setters
// directly (Borland __fastcall: this in EAX, arg2 in EDX). This executes VAZ's real 80-bit x87 code, so the
// computed coef/damp ints written into the object are the EXACT values. Read them back at a grid of settings.
// Build (x86): cl /O2 /EHsc vaz_coef_dump.cpp
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

static uint8_t* g_base  = nullptr;
static int      g_sr    = 44100;
static int*     g_srPtr = &g_sr;      // decimator's [+0x1c] uses double indirection (**), reverb uses single (*)
static void* fnv (uint32_t va) { return g_base + (va - 0x400000); }   // PE preferred base 0x400000

// Fill the import address table by hand (GetProcAddress) so we can call the DLL's code without running its DllMain.
static bool resolveImports (uint8_t* base)
{
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*) base;
    IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*) (base + dos->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!rva) return true;
    for (IMAGE_IMPORT_DESCRIPTOR* imp = (IMAGE_IMPORT_DESCRIPTOR*) (base + rva); imp->Name; ++imp)
    {
        HMODULE dep = LoadLibraryA ((char*) (base + imp->Name));
        if (!dep) { printf ("dep fail: %s\n", (char*) (base + imp->Name)); return false; }
        DWORD* iat  = (DWORD*) (base + imp->FirstThunk);
        DWORD* intt = (DWORD*) (base + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        for (; *intt; ++intt, ++iat)
        {
            FARPROC p;
            if (*intt & 0x80000000) p = GetProcAddress (dep, (LPCSTR) (*intt & 0xffff));
            else { IMAGE_IMPORT_BY_NAME* n = (IMAGE_IMPORT_BY_NAME*) (base + *intt); p = GetProcAddress (dep, n->Name); }
            DWORD old; VirtualProtect (iat, 4, PAGE_READWRITE, &old);
            *iat = (DWORD) p;
        }
    }
    return true;
}

// FUN_00522c60(this=EAX, EDX=&sr): reverb length+coef+damp orchestrator.
static void reverbSetup (void* self)
{
    void* f = fnv (0x522c60); int* srp = &g_sr;
    __asm {
        mov eax, self
        mov edx, srp
        call f
    }
}
// FUN_0051dc7c(this=EAX): decimator smoothing-coef setter (reads [+0x260], [+0x1c]).
static void decimSmooth (void* self)
{
    void* f = fnv (0x51dc7c);
    __asm {
        mov eax, self
        call f
    }
}

int main ()
{
    HMODULE h = LoadLibraryExA ("C:\\APC\\y\\tools\\Vaz2010Core.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!h) { printf ("LOAD FAIL %lu\n", GetLastError ()); return 1; }
    g_base = (uint8_t*) h;
    if (!resolveImports (g_base)) { printf ("IMPORT RESOLVE FAIL\n"); return 1; }
    printf ("// Vaz2010Core.dll @ %p, imports resolved, sr=%d\n", (void*) h, g_sr); fflush (stdout);
    uint8_t* obj = (uint8_t*) VirtualAlloc (0, 0x40000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    const uint32_t combOff[10] = { 0x274,0x427c,0x8284,0xc28c,0x10294,0x1429c,0x182a4,0x1c2ac,0x202b4,0x242bc };
    const uint32_t lenOff[10]  = { 0x270,0x4278,0x8280,0xc288,0x10290,0x14298,0x182a0,0x1c2a8,0x202b0,0x242b8 };
    const int grid[5] = { 0, 64, 128, 192, 255 };

    (void) grid;
  __try {
    // Full 0..255 range so the clone can embed an EXACT LUT (no interpolation error). Parseable: prefix,param,vals…
    for (int s = 0; s <= 255; ++s)
    {
        memset (obj, 0, 0x40000);
        *(int*) (obj + 0x260) = s; *(int*) (obj + 0x264) = 127; *(void**) (obj + 0x1c) = &g_sr;
        reverbSetup (obj);
        if (s == 0) { printf ("VALID lens:"); for (int i = 0; i < 10; ++i) printf (" %d", *(int*) (obj + lenOff[i])); printf ("\n"); }
        printf ("R,%d", s);
        for (int i = 0; i < 10; ++i) printf (",%08X", *(uint32_t*) (obj + combOff[i]));
        printf ("\n");
    }
    for (int d = 0; d <= 255; ++d)
    {
        memset (obj, 0, 0x40000);
        *(int*) (obj + 0x260) = 128; *(int*) (obj + 0x264) = d; *(void**) (obj + 0x1c) = &g_sr;
        reverbSetup (obj);
        printf ("D,%d,%08X\n", d, *(uint32_t*) (obj + 0x302e4));   // damp2 (damp1 = 2^28 − damp2)
    }
    for (int p = 0; p <= 255; ++p)
    {
        memset (obj, 0, 0x40000);
        *(int*) (obj + 0x260) = p; *(void**) (obj + 0x1c) = &g_srPtr;   // ** double indirection for decimator
        decimSmooth (obj);
        printf ("M,%d,%08X,%d\n", p, *(uint32_t*) (obj + 0x280), *(int*) (obj + 0x26c));
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    printf ("CRASH in setup call — exception 0x%08X (cannot run the 80-bit x87 code standalone)\n", GetExceptionCode ());
    return 2;
  }
    return 0;
}
