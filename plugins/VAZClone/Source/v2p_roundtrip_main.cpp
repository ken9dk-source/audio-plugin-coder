// v2p_roundtrip_main.cpp — VazV2PRoundtrip (regression oracle for the .v2p save path).
// For each test .v2p: load it, EDIT every parameter to a distinctive value, buildV2P() (save),
// reload the saved bytes, and verify every parameter survived the round-trip. Params that revert
// are writer gaps (buildV2P only serialises ~26 of the ~63 loaded fields). Run:
//   VazV2PRoundtrip.exe [folder-with-.v2p]     (default: tools/generated-presets)
#include "PluginProcessor.h"
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>

static std::map<std::string, float> snapshot (VAZCloneAudioProcessor& proc)
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
    std::cout << "=== VazV2PRoundtrip: load -> EDIT all params -> buildV2P -> reload -> compare ===\n";
    std::cout << "folder: " << dir << "   .v2p files: " << files.size() << "\n\n";
    if (files.isEmpty()) { std::cout << "NO FILES — pass a folder of .v2p as argv[1].\n"; return 2; }

    VAZCloneAudioProcessor proc;
    int nOK = 0, nFail = 0, nLoadFail = 0;
    std::map<std::string, int> driftCount;   // paramID -> # files it drifts in
    int nParams = 0;

    for (auto& f : files)
    {
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb) || ! proc.loadV2P (mb)) { ++nLoadFail; continue; }

        // EDIT: drive every parameter to a distinctive, deterministic value, then read back (quantised).
        std::map<std::string, float> target;
        for (auto* p : proc.getParameters())
            if (auto* pid = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            {
                const float v = 0.15f + 0.70f * (float) ((std::hash<std::string>{} (pid->paramID.toStdString())) % 101) / 100.0f;
                p->setValueNotifyingHost (v);
                target[pid->paramID.toStdString()] = p->getValue();
            }
        nParams = (int) target.size();

        auto saved = proc.buildV2P();
        if (! proc.loadV2P (saved)) { std::cout << f.getFileName() << ": RELOAD of saved bytes FAILED\n"; ++nFail; continue; }
        auto after = snapshot (proc);

        std::vector<std::string> drift;
        for (auto& kv : target)
            if (std::abs (kv.second - after[kv.first]) > 1.0f / 255.0f) { drift.push_back (kv.first); driftCount[kv.first]++; }

        if (drift.empty()) ++nOK;
        else
        {
            ++nFail;
            std::cout << f.getFileName().toStdString() << ": " << drift.size() << "/" << nParams << " params LOST: ";
            for (size_t i = 0; i < drift.size() && i < 10; ++i) std::cout << drift[i] << " ";
            std::cout << (drift.size() > 10 ? "..." : "") << "\n";
        }
    }

    std::cout << "\n" << nOK << " round-trip CLEAN, " << nFail << " with lost params, " << nLoadFail << " load-fail (skipped).\n";
    std::cout << "params per patch: " << nParams << "\n";
    if (! driftCount.empty())
    {
        std::vector<std::pair<std::string,int>> ranked (driftCount.begin(), driftCount.end());
        std::sort (ranked.begin(), ranked.end(), [] (auto& a, auto& b) { return a.second > b.second; });
        std::cout << "\nParams the writer DROPS (not serialised by buildV2P), by frequency:\n";
        for (size_t i = 0; i < ranked.size(); ++i)
            std::cout << "  " << ranked[i].first << "  (" << ranked[i].second << " files)\n";
    }
    return nFail ? 1 : 0;
}
