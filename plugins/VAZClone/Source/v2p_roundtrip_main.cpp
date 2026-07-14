// v2p_roundtrip_main.cpp — VazV2PRoundtrip: regression oracle for the .v2p save path (buildV2P).
// For each v2.0 (ver>=200) test patch: set ALL params to 0 -> buildV2P -> reload -> P0; then set ALL
// to 1 -> buildV2P -> reload -> P1. A parameter is genuinely SERIALISED by the writer iff P0 != P1.
// Setting every param together co-varies the coupled ones (tune octave/coarse/fine share one totalC;
// *_amt_inv is the sign of *_amt), so this avoids the quantisation artefacts of editing them to
// arbitrary independent values, and directly catches "field dropped by the writer" gaps.
// ver<200 patches have FEWER fields (v2.0-only params don't exist in the old layout) so they are
// counted but not strictly asserted. Run: VazV2PRoundtrip.exe [folder]  (default tools/generated-presets)
#include "PluginProcessor.h"
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

static int findPRST (const juce::uint8* d, int n)
{
    for (int i = 0; i + 4 <= n; ++i)
        if (d[i]=='P' && d[i+1]=='R' && d[i+2]=='S' && d[i+3]=='T') return i;
    return -1;
}
static std::map<std::string, float> snap (VAZCloneAudioProcessor& proc)
{
    std::map<std::string, float> m;
    for (auto* p : proc.getParameters())
        if (auto* pid = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            m[pid->paramID.toStdString()] = p->getValue();
    return m;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String dir = argc > 1 ? juce::String (argv[1]) : juce::String ("C:/APC/y/tools/generated-presets");
    auto files = juce::File (dir).findChildFiles (juce::File::findFiles, true, "*.v2p");
    std::cout << "=== VazV2PRoundtrip: set-all-0 vs set-all-1, save->reload, every param must respond ===\n";
    std::cout << "folder: " << dir << "   .v2p files: " << files.size() << "\n\n";
    if (files.isEmpty()) { std::cout << "NO FILES\n"; return 2; }

    VAZCloneAudioProcessor proc;
    auto setAll = [&](float val){ for (auto* p : proc.getParameters()) p->setValueNotifyingHost (val); };
    // Params that legitimately cannot be independently serialised (documented in buildV2P):
    const std::set<std::string> SKIP = { "e2_dest",       // lossy: 8-value Env2-dest bitfield -> 4-value choice
                                         "ma2_am_amt" };  // VAZ Mod Amp 2 has no depth control (always full)

    int nV2 = 0, nOld = 0, nClean = 0, nGap = 0;
    std::map<std::string, int> gapCount;

    for (auto& f : files)
    {
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb)) continue;
        const auto* d = (const juce::uint8*) mb.getData(); const int n = (int) mb.getSize();
        const int prst = findPRST (d, n); if (prst < 0 || prst + 12 > n) continue;
        const int ver = d[prst+8] | (d[prst+9]<<8) | (d[prst+10]<<16) | (d[prst+11]<<24);
        if (ver < 200)
        {
            ++nOld;   // fewer fields exist — not strictly tested, but must not be CORRUPTED on save:
            proc.loadV2P (mb); auto saved = proc.buildV2P();
            if (proc.debugV2PConsumedEnd (saved) != proc.debugV2PConsumedEnd (mb))
                { std::cout << f.getFileName().toStdString() << ": ver" << ver << " ROUND-TRIP CHANGED STRUCTURE (corruption!)\n"; ++nGap; }
            continue;
        }
        ++nV2;

        proc.loadV2P (mb); setAll (0.0f); { auto s = proc.buildV2P(); proc.loadV2P (s); } auto P0 = snap (proc);
        proc.loadV2P (mb); setAll (1.0f); { auto s = proc.buildV2P(); proc.loadV2P (s); } auto P1 = snap (proc);

        std::vector<std::string> gaps;
        for (auto& kv : P0)
            if (! SKIP.count (kv.first) && std::abs (kv.second - P1[kv.first]) < 0.10f)
                { gaps.push_back (kv.first); gapCount[kv.first]++; }

        if (gaps.empty()) ++nClean;
        else { ++nGap; if (nGap <= 3) { std::cout << f.getFileName().toStdString() << ": " << gaps.size() << " param(s) NOT serialised: ";
                                        for (auto& g : gaps) std::cout << g << " "; std::cout << "\n"; } }
    }

    std::cout << "\nv2.0 patches: " << nClean << "/" << nV2 << " serialise EVERY param; " << nGap << " with gaps.  (ver<200 skipped: " << nOld << ")\n";
    if (! gapCount.empty())
    {
        std::vector<std::pair<std::string,int>> r (gapCount.begin(), gapCount.end());
        std::sort (r.begin(), r.end(), [](auto&a,auto&b){ return a.second>b.second; });
        std::cout << "\nParams the writer still DROPS:\n";
        for (auto& kv : r) std::cout << "  " << kv.first << "  (" << kv.second << " patches)\n";
    }
    return nGap ? 1 : 0;
}
