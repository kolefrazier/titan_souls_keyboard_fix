// SDL2_proxy.cpp

// Log file tail:
// cd E:\SteamLibrary\steamapps\common\Titan Souls
// Get-Content sdlproxy.log -Wait

#include "pch.h"
#include "forwards.h"
#include <fstream>
#include <windows.h>
#include <SDL.h>

std::ofstream logFile;

// Load the real SDL.dll file
static HMODULE realSDL = nullptr;

// Typedefs + static references for real functions
// SDL_PollEvent
typedef int (SDLCALL* SDL_PollEvent_t)(SDL_Event*);
static SDL_PollEvent_t real_SDL_PollEvent;

// SDL_GetKeyboardState
typedef const Uint8* (SDLCALL* SDL_GetKeyboardState_t)(int* numkeys);
static SDL_GetKeyboardState_t real_GetKeyboardState;

// SDL_GetMouseState
typedef Uint32(SDLCALL* SDL_GetMouseState_t)(int*, int*);
static SDL_GetMouseState_t real_SDL_GetMouseState;

// SDL_GetWindowSize
typedef void (SDLCALL* SDL_GetWindowSize_t)(SDL_Window*, int*, int*);
static SDL_GetWindowSize_t real_SDL_GetWindowSize;

// SDL_GetKeyboardFocus
typedef SDL_Window* (SDLCALL* SDL_GetKeyboardFocus_t)(void);
static SDL_GetKeyboardFocus_t real_SDL_GetKeyboardFocus;

// SDL_ShowCursor
typedef int (SDLCALL* SDL_ShowCursor_t)(int);
static SDL_ShowCursor_t real_SDL_ShowCursor;

// SDL_PushEvent
typedef int (SDLCALL* SDL_PushEvent_t)(SDL_Event*);
static SDL_PushEvent_t real_SDL_PushEvent;

// Static keyboard state buffer we return instead of the real one
static Uint8 patchedState[SDL_NUM_SCANCODES];

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        logFile.open("sdlproxy.log", std::ios::out | std::ios::trunc);

        realSDL = LoadLibraryA("SDL2_original.dll");
        logFile << ">>>>> DLL loaded <<<<<" << std::endl;
        logFile.flush();

        if (realSDL) {
            real_SDL_PollEvent = (SDL_PollEvent_t)GetProcAddress(realSDL, "SDL_PollEvent");
            real_GetKeyboardState = (SDL_GetKeyboardState_t)GetProcAddress(realSDL, "SDL_GetKeyboardState");
            real_SDL_GetMouseState = (SDL_GetMouseState_t)GetProcAddress(realSDL, "SDL_GetMouseState");
            real_SDL_GetWindowSize = (SDL_GetWindowSize_t)GetProcAddress(realSDL, "SDL_GetWindowSize");
            //real_SDL_CreateWindow = (SDL_CreateWindow_t)GetProcAddress(realSDL, "SDL_CreateWindow");
            real_SDL_GetKeyboardFocus = (SDL_GetKeyboardFocus_t)GetProcAddress(realSDL, "SDL_GetKeyboardFocus");
            real_SDL_PushEvent = (SDL_PushEvent_t)GetProcAddress(realSDL, "SDL_PushEvent");
            real_SDL_ShowCursor = (SDL_ShowCursor_t)GetProcAddress(realSDL, "SDL_ShowCursor");
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        if (realSDL) FreeLibrary(realSDL);
    }

    return TRUE;
}

// ----- Implementations -----
// New implementations need:
// * Add to SDL2.def
// * Comment out forward in header (forwards.h)
// 
// dllmain.cpp:
// * Typedef defined here for the original method
// * Static pointer to the original method
// * Register in DllMain -> DLL_PROCESS_ATTACH
// * Use `extern "C" __declspec(dllexport)` to prevent name mangling

void patchKeyboard(Uint8* patched, const Uint8* real) {
    // Movement - Map arrow keys to WASD, while still supporting arrow keys.
    patched[SDL_SCANCODE_UP] = real[SDL_SCANCODE_W] | real[SDL_SCANCODE_UP];
    patched[SDL_SCANCODE_LEFT] = real[SDL_SCANCODE_A] | real[SDL_SCANCODE_LEFT];
    patched[SDL_SCANCODE_DOWN] = real[SDL_SCANCODE_S] | real[SDL_SCANCODE_DOWN];
    patched[SDL_SCANCODE_RIGHT] = real[SDL_SCANCODE_D] | real[SDL_SCANCODE_RIGHT];

    // Rolling/Running - remap to space.
    patched[SDL_SCANCODE_X] = real[SDL_SCANCODE_SPACE] | real[SDL_SCANCODE_X];

    // Fire arrow - support enter and left mouse button.
    // Enter works on the menu.
    patched[SDL_SCANCODE_C] = real[SDL_SCANCODE_C]
        | real[SDL_SCANCODE_KP_ENTER]
        | ((real_SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0);
}

void mapKeyboardState() {
    // Replicates SDL_GetKeyboardState so that it updates on each frame
    const Uint8* realState = real_GetKeyboardState(NULL);
    memcpy(patchedState, realState, SDL_NUM_SCANCODES);

    patchKeyboard(patchedState, realState);
}

void setAimingAngle() {
    SDL_Window* window = real_SDL_GetKeyboardFocus();

    if (window) {
        int w, h;
        real_SDL_GetWindowSize(window, &w, &h);

        int mouseX, mouseY;
        real_SDL_GetMouseState(&mouseX, &mouseY);

        float dx = mouseX - (w / 2.0f);
        float dy = mouseY - (h / 2.0f);
        float len = sqrt(dx * dx + dy * dy);

        if (len > 0) {
            Sint16 axisX = (Sint16)((dx / len) * 32767);
            Sint16 axisY = (Sint16)((dy / len) * 32767);

            SDL_Event fakeX = {}, fakeY = {};

            fakeX.type = SDL_CONTROLLERAXISMOTION;
            fakeX.caxis.axis = 0;
            fakeX.caxis.value = axisX;
            fakeX.caxis.which = 0;

            fakeY.type = SDL_CONTROLLERAXISMOTION;
            fakeY.caxis.axis = 1;
            fakeY.caxis.value = axisY;
            fakeY.caxis.which = 0;

            real_SDL_PushEvent(&fakeX);
            real_SDL_PushEvent(&fakeY);

            logFile << "--> Injecting axis: X=" << axisX << " Y=" << axisY
                << " mouse=" << mouseX << "," << mouseY
                << " center=" << w / 2 << "," << h / 2 << std::endl;
        }
    }
}

extern "C" __declspec(dllexport)
int SDLCALL SDL_PollEvent(SDL_Event* event)
{
    int result = real_SDL_PollEvent(event);
    mapKeyboardState();

    if (event->type != SDL_CONTROLLERAXISMOTION && event->type != SDL_JOYAXISMOTION)
        setAimingAngle();

    //if (event->type == SDL_CONTROLLERAXISMOTION) {
    //    logFile << "Controller which=" << (int)event->caxis.which << std::endl;
    //}

    if (event->type == SDL_CONTROLLERAXISMOTION) {
        logFile << "<== CONTROLLERAXISMOTION out: axis=" << (int)event->caxis.axis
            << " value=" << (int)event->caxis.value
            << " which=" << (int)event->caxis.which << std::endl;
    }

    if (result && event)
    {
        if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
        {
            SDL_Keycode& key = event->key.keysym.sym;
            SDL_Scancode& scancode = event->key.keysym.scancode;
            
            //if(event->type == SDL_KEYDOWN)
            //    logFile << "Scancode: " << (int)scancode << " Keycode: " << (int)key << std::endl;
        }

        if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP)
        {
            Uint8& button = event->button.button;

            switch (button)
            {
            case SDL_BUTTON_LEFT:
                logFile << "SDL_PollEvent -> SDL_BUTTON_LEFT" << std::endl;
                button = SDLK_c;
                break;
            }
        }
    }

    logFile.flush();
    return result;
}

// This is being called once at startup and caching the pointer.
// The pointer it returns can then be manipulated in SDL_PollEvent per-frame.
extern "C" __declspec(dllexport)
const Uint8* SDLCALL SDL_GetKeyboardState(int* numkeys) {
    const Uint8* realState = real_GetKeyboardState(numkeys);
    memcpy(patchedState, realState, SDL_NUM_SCANCODES);
    patchKeyboard(patchedState, realState);
    return patchedState;
}

extern "C" __declspec(dllexport)
int SDLCALL SDL_ShowCursor(int toggle) {
    return real_SDL_ShowCursor(SDL_ENABLE);
}
