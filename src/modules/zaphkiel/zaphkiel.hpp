#pragma once
#include "../Module.hpp"
#include <string>
#include <vector>
#include <atomic>
struct DumpEntry{ std::string name; uintptr_t addr=0; int vsize=0; };
struct ZaphkielModule : public Module {
    ZaphkielModule() : Module("Zaphkiel-NG", "Fast+Hook+Sig+Diff+Kurumi-Style") {}
    void onInit() override;
    void onFrame() override;
    void onMenuRegistered() override;
    bool onMenuConfigChanged(std::string_view key, std::string_view value) override;

    std::string mediaBase="/storage/emulated/0/Android/media/org.levimc.launcher/Zaphkiel";
    std::string cachePath="/storage/emulated/0/Android/media/org.levimc.launcher/Zaphkiel/rtti_cache.txt";
    std::vector<DumpEntry> entries;
    std::string status="Idle", hookCode="", sigCode="";
    std::atomic<float> prog{0};
    std::atomic<bool> running{false};
    std::atomic<int> found{0};
    int sel=-1;
    char filter[128]="";
    int methodIdx=12;
    bool showWindow=true;
    void loadAll(); void fastScan(); void genHook(); void genSig();
    void ensureFolders();
};
