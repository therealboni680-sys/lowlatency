// Low Latency Tweaks - mod de Geode para Geometry Dash 2.2081 (Windows)
// Reduce input lag y estabiliza FPS: VSync off, limite de FPS, prioridad,
// EcoQoS, temporizador de 1 ms y (opcional) glFinish tras cada frame.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCEGLView.hpp>

#include <Windows.h>

using namespace geode::prelude;

namespace {
    // Estado actual. Hasta que el juego termina de cargar no se fuerza nada.
    bool g_fpsOverride = false;
    int64_t g_fpsCap = 360;
    bool g_fpsForced = false;
    bool g_glFinish = false;
    bool g_timerActive = false;
    bool g_priorityChanged = false;
    DWORD g_originalPriority = NORMAL_PRIORITY_CLASS;

    template <class T>
    T setting(char const* key) {
        return Mod::get()->getSettingValue<T>(key);
    }

    // glFinish sin depender de cabeceras ni librerias extra.
    using GlFinishFn = void (WINAPI*)();
    GlFinishFn glFinishPtr() {
        static GlFinishFn fn = []() -> GlFinishFn {
            HMODULE gl = GetModuleHandleW(L"opengl32.dll");
            if (!gl) return nullptr;
            return reinterpret_cast<GlFinishFn>(GetProcAddress(gl, "glFinish"));
        }();
        return fn;
    }

    void applyVSync() {
        if (!setting<bool>("disable-vsync")) return;
        if (auto app = CCApplication::sharedApplication()) {
            app->toggleVerticalSync(false);
        }
        if (auto gm = GameManager::sharedState()) {
            gm->m_vsyncEnabled = false;
        }
    }

    void applyFps() {
        bool want = setting<bool>("fps-override");
        int64_t cap = setting<int64_t>("fps-cap");
        g_fpsOverride = want;
        g_fpsCap = cap;

        if (want && cap > 0) {
            if (auto app = CCApplication::sharedApplication()) {
                app->setAnimationInterval(1.0 / static_cast<double>(cap));
            }
            g_fpsForced = true;
        }
        else if (g_fpsForced) {
            // Se apago el override: que el juego recalcule su propio FPS.
            g_fpsForced = false;
            if (auto gm = GameManager::sharedState()) {
                gm->updateCustomFPS();
            }
        }
    }

    void applyPriority() {
        bool want = setting<bool>("high-priority");
        HANDLE proc = GetCurrentProcess();
        if (want && !g_priorityChanged) {
            g_originalPriority = GetPriorityClass(proc);
            if (g_originalPriority == 0) g_originalPriority = NORMAL_PRIORITY_CLASS;
            if (SetPriorityClass(proc, HIGH_PRIORITY_CLASS)) {
                g_priorityChanged = true;
            }
        }
        else if (!want && g_priorityChanged) {
            SetPriorityClass(proc, g_originalPriority);
            g_priorityChanged = false;
        }
    }

    // Equivale a PROCESS_POWER_THROTTLING_STATE de la API de Windows.
    struct PowerThrottlingState {
        ULONG Version;
        ULONG ControlMask;
        ULONG StateMask;
    };

    void applyThrottling() {
        using SetProcessInformationFn = BOOL (WINAPI*)(HANDLE, int, LPVOID, DWORD);
        HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
        if (!k32) return;
        auto fn = reinterpret_cast<SetProcessInformationFn>(GetProcAddress(k32, "SetProcessInformation"));
        if (!fn) return;

        // Version 1; 0x1 = EXECUTION_SPEED.
        // ControlMask=1 y StateMask=0 -> Windows NO puede limitar la CPU del proceso.
        // ControlMask=0 -> vuelve al modo automatico de Windows.
        PowerThrottlingState st{ 1, 0, 0 };
        if (setting<bool>("disable-throttling")) {
            st.ControlMask = 0x1;
        }
        // 4 = ProcessPowerThrottling
        fn(GetCurrentProcess(), 4, &st, sizeof(st));
    }

    void applyTimer() {
        using TimeFn = UINT (WINAPI*)(UINT);
        static HMODULE winmm = LoadLibraryW(L"winmm.dll");
        if (!winmm) return;
        auto begin = reinterpret_cast<TimeFn>(GetProcAddress(winmm, "timeBeginPeriod"));
        auto end = reinterpret_cast<TimeFn>(GetProcAddress(winmm, "timeEndPeriod"));
        if (!begin || !end) return;

        bool want = setting<bool>("timer-resolution");
        if (want && !g_timerActive) {
            if (begin(1) == 0) { // 0 = TIMERR_NOERROR
                g_timerActive = true;
            }
        }
        else if (!want && g_timerActive) {
            end(1);
            g_timerActive = false;
        }
    }

    void applyAll() {
        g_glFinish = setting<bool>("gl-finish");
        applyPriority();
        applyThrottling();
        applyTimer();
        applyVSync();
        applyFps();
        log::info(
            "Low Latency: vsync_off={} fps_override={} fps_cap={} prioridad_alta={} timer_1ms={} glFinish={}",
            setting<bool>("disable-vsync"), g_fpsOverride, g_fpsCap,
            g_priorityChanged, g_timerActive, g_glFinish
        );
    }
}

// Todas las rutas del juego que cambian el intervalo de frame pasan por aqui.
class $modify(LLApplication, CCApplication) {
    void setAnimationInterval(double interval) {
        if (g_fpsOverride && g_fpsCap > 0) {
            interval = 1.0 / static_cast<double>(g_fpsCap);
        }
        CCApplication::setAnimationInterval(interval);
    }
};

// Opcional: glFinish justo despues de intercambiar buffers.
class $modify(LLEGLView, CCEGLView) {
    void swapBuffers() {
        CCEGLView::swapBuffers();
        if (g_glFinish) {
            if (auto fn = glFinishPtr()) {
                fn();
            }
        }
    }
};

// Se dispara una sola vez, justo despues de crear el MenuLayer (la ventana ya existe).
$on_game(Loaded) {
    applyAll();

    for (char const* key : {
        "disable-vsync", "fps-override", "high-priority",
        "disable-throttling", "timer-resolution", "gl-finish"
    }) {
        (void) listenForSettingChanges<bool>(key, [](bool) { applyAll(); });
    }
    (void) listenForSettingChanges<int64_t>("fps-cap", [](int64_t) { applyAll(); });
}
