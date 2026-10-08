#include <jni.h>
#include <android/input.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>

#include <atomic>
#include <string_view>

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/Logger.hpp>
#include <pl/memory/Hook.hpp>

#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_android.h"
#include "ImGui/backends/imgui_impl_opengl3.h"

#define TAG "PanelMod"

namespace {

constexpr std::string_view kModuleId = "panelmod.main";

// libinput.so ka 'consume' symbol. Candidates try hote hain.
// Agar logcat mein "Input hook: FAIL" dikhe to Display-FPS ke main.cpp
// (line ~151) se EXACT string copy karke pehla candidate replace karo.
const char* kConsumeCandidates[] = {
    "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE",
    "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEbIPjPPNS_10InputEventE",
    "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEbPjPPNS_10InputEventE",
};

std::atomic_bool g_PanelOn{false};      // mod menu toggle
bool             g_PanelUIOpen = false; // ImGui window ka X
std::atomic_bool g_Initialized{false};
int g_Width = 0, g_Height = 0;
EGLContext g_TargetContext = EGL_NO_CONTEXT;
EGLSurface g_TargetSurface = EGL_NO_SURFACE;

static char g_TextBuf[128] = "";
static bool g_EffectOn = false;

using SwapFn    = EGLBoolean(*)(EGLDisplay, EGLSurface);
using ConsumeFn = int32_t(*)(void*, void*, bool, long, uint32_t*, AInputEvent**);
SwapFn    g_OrigSwap    = nullptr;
ConsumeFn g_OrigConsume = nullptr;
pl::memory::HookHandle g_SwapHook;
pl::memory::HookHandle g_ConsumeHook;

void ApplySimpleStyle() {
    ImGui::StyleColorsLight();                 // white background
    ImGuiStyle& st = ImGui::GetStyle();
    st.Colors[ImGuiCol_Button]        = ImVec4(0.74f, 0.74f, 0.74f, 1.0f); // grey
    st.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.69f, 0.31f, 1.0f); // green
    st.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.20f, 0.55f, 0.24f, 1.0f);
    st.TouchExtraPadding = ImVec2(6, 6);
    st.WindowRounding = 6.0f; st.FrameRounding = 4.0f; st.GrabRounding = 4.0f;
}

void Setup() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.IniFilename = nullptr;              // game cwd mein imgui.ini mat likho
    ImGui_ImplAndroid_Init();              // window default 0 (hamare paas window nahi)
    ImGui_ImplOpenGL3_Init(nullptr);       // Android pe auto "#version 300 es"
    ApplySimpleStyle();
    g_Initialized = true;
}

void DrawMenu() {
    if (!g_PanelOn.load() || !g_PanelUIOpen) return;
    ImGui::SetNextWindowSize(ImVec2(420, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mera Panel", &g_PanelUIOpen, ImGuiWindowFlags_NoCollapse); // X free
    if (ImGui::Button(g_EffectOn ? "EFFECT: ON" : "EFFECT: OFF", ImVec2(200, 44)))
        g_EffectOn = !g_EffectOn;
    ImGui::InputText("Type here", g_TextBuf, sizeof(g_TextBuf));
    ImGui::Text("Tumne likha: %s", g_TextBuf);
    ImGui::End();
}

void Render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(g_Width, g_Height); // (x,y) overload ZAROORI hai
    ImGui::NewFrame();
    DrawMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

EGLBoolean hook_swap(EGLDisplay dpy, EGLSurface surf) {
    if (!g_OrigSwap) return EGL_FALSE;
    EGLContext ctx = eglGetCurrentContext();
    if (ctx == EGL_NO_CONTEXT) return g_OrigSwap(dpy, surf);
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w < 500 || h < 500) return g_OrigSwap(dpy, surf);   // choti surfaces skip
    if (g_TargetContext == EGL_NO_CONTEXT) {
        EGLint buf = 0;
        eglQuerySurface(dpy, surf, EGL_RENDER_BUFFER, &buf);
        if (buf == EGL_BACK_BUFFER) { g_TargetContext = ctx; g_TargetSurface = surf; }
    }
    if (ctx != g_TargetContext || surf != g_TargetSurface) return g_OrigSwap(dpy, surf);
    g_Width = w; g_Height = h;
    if (!g_Initialized.load()) Setup();
    Render();
    return g_OrigSwap(dpy, surf);
}

int32_t hook_consume(void* thiz, void* factory, bool b, long seq,
                     uint32_t* outSeq, AInputEvent** outEvent) {
    int32_t r = g_OrigConsume ? g_OrigConsume(thiz, factory, b, seq, outSeq, outEvent) : 0;
    if (r == 0 && outEvent && *outEvent && g_Initialized.load() && g_PanelOn.load()) {
        AInputEvent* ev = *outEvent;
        // Typing bridge: backend characters feed NAHI karta (source se verified),
        // isliye hardware keys se chars hum khud dete hain.
        if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_KEY &&
            AKeyEvent_getAction(ev) == AKEY_EVENT_ACTION_DOWN) {
            int32_t code = AKeyEvent_getKeyCode(ev);
            bool shift = (AKeyEvent_getMetaState(ev) & AMETA_SHIFT_ON) != 0;
            char c = 0;
            if (code >= AKEYCODE_A && code <= AKEYCODE_Z)
                c = (char)((shift ? 'A' : 'a') + (code - AKEYCODE_A));
            else if (code >= AKEYCODE_0 && code <= AKEYCODE_9)
                c = (char)('0' + (code - AKEYCODE_0));
            else if (code == AKEYCODE_SPACE) c = ' ';
            if (c) ImGui::GetIO().AddInputCharacter((unsigned int)c);
        }
        ImGui_ImplAndroid_HandleInputEvent(ev);  // touch + keys -> ImGui
    }
    return r;  // v1: game ko touch bhi milega (quirk documented)
}

void* MainThread(void*) {
    sleep(3);  // game libs load hone do
    void* egl = dlopen("libEGL.so", RTLD_NOW);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap)
            g_SwapHook = pl::memory::HookHandle(swap, (void*)&hook_swap, (void**)&g_OrigSwap);
        __android_log_print(ANDROID_LOG_INFO, TAG, "EGL hook: %s",
                            g_SwapHook.installed() ? "OK" : "FAIL");
    } else {
        __android_log_print(ANDROID_LOG_WARN, TAG, "libEGL.so open fail");
    }
    void* inp = dlopen("libinput.so", RTLD_NOW);
    if (inp) {
        void* sym = nullptr;
        for (const char* cand : kConsumeCandidates) { sym = dlsym(inp, cand); if (sym) break; }
        if (sym)
            g_ConsumeHook = pl::memory::HookHandle(sym, (void*)&hook_consume, (void**)&g_OrigConsume);
        __android_log_print(ANDROID_LOG_INFO, TAG, "Input hook: %s",
                            g_ConsumeHook.installed() ? "OK" : "FAIL");
    } else {
        __android_log_print(ANDROID_LOG_WARN, TAG, "libinput.so open fail");
    }
    return nullptr;
}

void onToggle(std::string_view moduleId, bool enabled) {
    if (moduleId != kModuleId) return;
    g_PanelOn = enabled;
    g_PanelUIOpen = enabled;
}

}  // namespace

class PanelMod {
public:
    static PanelMod& instance() { static PanelMod m; return m; }
    PanelMod() : mSelf(*ll::mod::NativeMod::current()) {}
    ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load() { getSelf().getLogger().info("PanelMod loaded"); return true; }

    bool enable() {
        pthread_t t;
        pthread_create(&t, nullptr, MainThread, nullptr);
        return pl::modmenu::ModuleBuilder(kModuleId, "Mera Panel")
            .modId(getSelf().getId())
            .description("ImGui panel: typing + buttons + X")
            .defaultEnabled(false)
            .onToggle(onToggle)
            .registerModule();
    }

    bool disable() {
        g_PanelOn = false;
        g_PanelUIOpen = false;
        pl::modmenu::unregisterModule(kModuleId);
        return true;   // hooks fail-soft rehne do (mid-frame unhook = crash risk)
    }

    bool unload() { return true; }

private:
    ll::mod::NativeMod& mSelf;
};

PL_REGISTER_MOD(PanelMod, PanelMod::instance())
