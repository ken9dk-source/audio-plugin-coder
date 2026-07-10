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
// FUN_00521aa0(this=EAX, EDX=&srPtr): phaser coef-LUT builder. It calls FUN_004bed70(this) which sets [+0x1c]=EDX,
// then reads **[+0x1c] (double indirection) as SR → pass EDX=&g_srPtr so [+0x1c]=&g_srPtr and **=44100.
static void phaserBuildLut (void* self)
{
    void* f = fnv (0x521aa0); int** srpp = &g_srPtr;
    __asm {
        mov eax, self
        mov edx, srpp
        call f
    }
}
// FUN_0051c298(this=EAX, EDX=toneParam): delay tone→damp setter (reads [+0x1c] ** for SR, 80-bit). Set [+0x264]=0
// first so the trailing recursive host-notify branch is skipped.
static void delayToneDamp (void* self, int tone)
{
    void* f = fnv (0x51c298);
    __asm {
        mov eax, self
        mov edx, tone
        call f
    }
}
// FUN_00521b68(this=EAX, EDX=stagesRaw): phaser stages setter. +0x260=raw, +0x2a0=(raw+1)·2. Leaf, no x87 → safe.
static void phaserStages (void* self, int s)
{
    void* f = fnv (0x521b68);
    __asm {
        mov eax, self
        mov edx, s
        call f
    }
}
// FUN_00521c84(this=EAX, EDX=rateByte): phaser free-rate → inc[+0x294] = 3891.3559·e^(0.027·rate)·11025/SR
// (disasm 0x521c84: fild rate; ·0.027; e^x; ·3891.3559; ·11025/SR). Reads SR via [+0x1c]→[.] SINGLE indirection
// (NOT div-by-0 as earlier assumed) → set [+0x1c]=&g_sr, +0x274=0 (not synced) to take the compute branch.
static void phaserRate (void* self, int r)
{
    void* f = fnv (0x521c84);
    __asm {
        mov eax, self
        mov edx, r
        call f
    }
}
// FUN_00518ffc(this=EAX, EDX=rateByte): chorus LFO1 rate → inc[+0x288]. SAME exp curve as the phaser rate
// (3891.3559·e^(0.027·b)·11025/SR); SR via [+0x1c]→[.] SINGLE indirection; no sync guard (unconditional compute).
static void chorusRate1 (void* self, int r)
{
    void* f = fnv (0x518ffc);
    __asm {
        mov eax, self
        mov edx, r
        call f
    }
}
// FUN_00519098(this=EAX, EDX=rateByte): chorus LFO2 rate → inc[+0x290]. Same formula, second modulator.
static void chorusRate2 (void* self, int r)
{
    void* f = fnv (0x519098);
    __asm {
        mov eax, self
        mov edx, r
        call f
    }
}
// FUN_00517ee0(this=EAX, EDX=rateByte): autopan rate → inc[+0x27c] = 30.4012177·e^(0.036·b)·11025·256/SR.
// SR via [+0x1c]→[.] SINGLE indirection; guarded by +0x268==0 (sync flag off) → set it 0.
static void autopanRate (void* self, int r)
{
    void* f = fnv (0x517ee0);
    __asm {
        mov eax, self
        mov edx, r
        call f
    }
}
// FUN_00521d44(this=EAX, EDX=gainByte): phaser output gain. inGain[+0x298] = 2^((b−255)/60)·2^30 (self-contained
// x87: fld2/fldln2/fyl2x→ln2, (b−255)·ln2/60, exp, ·2^30). No SR, no globals → dumpable. Validates the disasm read.
static void phaserGain (void* self, int b)
{
    void* f = fnv (0x521d44);
    __asm {
        mov eax, self
        mov edx, b
        call f
    }
}
// NOTE: the chorus/phaser/autopan LFO-rate INCS (leaf setters FUN_00518ffc/FUN_00519098/FUN_005208f0/FUN_0051a5fc)
// are NOT dumpable — 80-bit AND they divide by DllMain-initialised global/object state = 0 standalone (int div-by-0).
// Only self-contained setup methods (reverb FUN_00522c60, phaser FUN_00521aa0 — SR from the object) work.

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
    // Phaser 512-entry coef LUT (FUN_00521aa0 @0x521aa0): (1 − i·5·ln2·440/255/sr)·2^30 clamp≥0. sr=44100.
    // Set sync-flag [+0x274]=1 so the tail FUN_00521c84 skips its 80-bit inc calc (would div-by-0 standalone).
    {
        memset (obj, 0, 0x40000);
        *(void**) (obj + 0x1c) = &g_srPtr;   // SR (** double indirection, like the chorus/decimator)
        *(int*) (obj + 0x274) = 1;           // sync flag = 1 → skip the 80-bit rate inc in the tail
        *(int*) (obj + 0x27c) = 0x40;        // rate param (unused when synced)
        phaserBuildLut (obj);
        printf ("PVALID inGain[+0x298]=%08X mix[+0x288]=%08X\n", *(uint32_t*) (obj + 0x298), *(uint32_t*) (obj + 0x288));
        for (int i = 0; i < 512; ++i) printf ("P,%d,%08X\n", i, *(uint32_t*) (obj + 0x310 + i * 4));
    }
    // Delay tone→damp curve (FUN_0051c298 @0x51c298): [+0x2a8] = 80-bit(tone [+0x280], SR). tone 0..255. sr=44100.
    for (int t = 0; t <= 255; ++t)
    {
        memset (obj, 0, 0x40000);
        *(void**) (obj + 0x1c) = &g_srPtr;   // ** double indirection (delay uses it)
        *(int*) (obj + 0x264) = 0;           // skip the recursive host-notify branch
        delayToneDamp (obj, t);
        printf ("T,%d,%08X\n", t, *(uint32_t*) (obj + 0x2a8));
    }
    // Phaser output-gain curve (FUN_00521d44 @0x521d44): inGain[+0x298] = round(2^((b−255)/60)·2^30). gain byte 0..255.
    for (int b = 0; b <= 255; ++b)
    {
        memset (obj, 0, 0x40000);
        phaserGain (obj, b);
        printf ("G,%d,%08X\n", b, *(uint32_t*) (obj + 0x298));
    }
    // Phaser CONSTRUCTOR default state (FUN_005216b8 @0x5216b8): base-init (FUN_004c398c) only writes +0x60..+0x25C,
    // leaving the param block +0x260+ zero-initialised; the ctor then writes ONLY stages=1, +0x278=0x60, mix +0x288
    // =0x80, and gain FUN_00521d44(0xe1). Replicate on a zeroed obj + read back → proves depth(+0x280)/feedback
    // (+0x29c)/center(+0x264) DEFAULT TO 0 (the clone's fb=0.5/depth=0.6 are assumed, not from the constructor).
    {
        memset (obj, 0, 0x40000);
        phaserStages (obj, 1);               // +0x260=1 → +0x2a0=(1+1)*2=4
        *(int*) (obj + 0x278) = 0x60;        // sync note/rate
        *(int*) (obj + 0x288) = 0x80;        // mix = 128 = 0.5
        phaserGain (obj, 0xe1);              // +0x28c=0xe1=225 → +0x298 = inGain(-3dB)
        printf ("C,stages260=%d,center264=%d,fbsign268=%d,fbphase26c=%d,fbmag270=%d,syncflag274=%d,note278=%d,"
                "rate27c=%d,depth280=%d,lrphase284=%d,mix288=%d,gainB28c=%d,inGain298=%08X,fbGain29c=%08X,numStages2a0=%d\n",
            *(int*)(obj+0x260), *(int*)(obj+0x264), *(int*)(obj+0x268), *(int*)(obj+0x26c), *(int*)(obj+0x270),
            *(int*)(obj+0x274), *(int*)(obj+0x278), *(int*)(obj+0x27c), *(int*)(obj+0x280), *(int*)(obj+0x284),
            *(int*)(obj+0x288), *(int*)(obj+0x28c), *(uint32_t*)(obj+0x298), *(uint32_t*)(obj+0x29c), *(int*)(obj+0x2a0));
    }
    // Phaser free-rate → inc curve (FUN_00521c84 @0x521c84): inc = 3891.3559·e^(0.027·rate)·11025/SR. rate 0..255.
    for (int r = 0; r <= 255; ++r)
    {
        memset (obj, 0, 0x40000);
        *(void**) (obj + 0x1c) = &g_sr;   // SINGLE indirection here ([+0x1c] → SR int)
        *(int*) (obj + 0x274) = 0;         // sync flag off → compute the free-rate inc
        phaserRate (obj, r);
        printf ("RA,%d,%08X\n", r, *(uint32_t*) (obj + 0x294));
    }
    // Chorus LFO1/LFO2 free-rate → inc (FUN_00518ffc→+0x288, FUN_00519098→+0x290): SAME exp curve as phaser.
    for (int r = 0; r <= 255; ++r)
    {
        memset (obj, 0, 0x40000); *(void**) (obj + 0x1c) = &g_sr;
        chorusRate1 (obj, r);
        printf ("CR1,%d,%08X\n", r, *(uint32_t*) (obj + 0x288));
    }
    for (int r = 0; r <= 255; ++r)
    {
        memset (obj, 0, 0x40000); *(void**) (obj + 0x1c) = &g_sr;
        chorusRate2 (obj, r);
        printf ("CR2,%d,%08X\n", r, *(uint32_t*) (obj + 0x290));
    }
    // Autopan free-rate → inc (FUN_00517ee0→+0x27c): 30.4012·e^(0.036·b)·11025·256/SR. +0x268=0 (sync off).
    for (int r = 0; r <= 255; ++r)
    {
        memset (obj, 0, 0x40000); *(void**) (obj + 0x1c) = &g_sr; *(int*) (obj + 0x268) = 0;
        autopanRate (obj, r);
        printf ("AR,%d,%08X\n", r, *(uint32_t*) (obj + 0x27c));
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
