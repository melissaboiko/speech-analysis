#include "sdl2.h"

#include <stdexcept>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#   include <emscripten.h>
extern const char *EMSCRIPTEN_CANVAS_NAME;
#endif

using namespace Module::Target;

std::vector<SDL_Event> SDL2::globalEvents;

SDL2::SDL2(Type rendererType)
    : AbstractBase { Type::OpenGL, Type::GLES, Type::Vulkan, Type::SDL2, Type::NanoVG },
      mRendererType(rendererType),
      mWindow(nullptr)
{
#if RENDERER_USE_OPENGL || RENDERER_USE_GLES
    setOpenGLProvider(new SDL2_OpenGL(&mWindow));
#endif

#ifdef RENDERER_USE_VULKAN
    setVulkanProvider(new SDL2_Vulkan(&mWindow));
#endif

#ifdef RENDERER_USE_SDL2
    setSDL2Provider(new SDL2_Renderer(&mWindow));
#endif

#ifdef RENDERER_USE_NVG
    setNvgProvider(new SDL2_NanoVG(&mWindow));
#endif
}

SDL2::~SDL2()
{
}

void SDL2::initialize()
{
    err = SDL_Init(SDL_INIT_VIDEO);
    checkError(err < 0);

#if RENDERER_USE_GLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);event subsystem thread
#elif RENDERER_USE_OPENGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

#if RENDERER_USE_NVG
#   if defined(NANOVG_DX11)
        SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "direct3d11", SDL_HINT_OVERRIDE);
#   elif defined(NANOVG_METAL)
        SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "metal", SDL_HINT_OVERRIDE);
#   elif defined(NANOVG_GLES2)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#   elif defined(NANOVG_GLES3)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#   elif defined(NANOVG_GL3)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#   endif

#   if defined(NANOVG_GL)
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
#   endif
#endif
}

void SDL2::terminate()
{
    SDL_Quit();
}

void SDL2::setTitle(const std::string& title)
{
    mTitle = title;

    if (mWindow != nullptr) {
        SDL_SetWindowTitle(mWindow, title.c_str());
    }
}

void SDL2::setSize(int width, int height)
{
    mWidth = width;
    mHeight = height;

#if ! ( defined(ANDROID) || defined(__ANDROID__) )
    if (mWindow != nullptr) {
        SDL_SetWindowSize(mWindow, width, height);
    }
#endif
}

void SDL2::getSize(int *pWidth, int *pHeight)
{
#ifdef __EMSCRIPTEN__
    *pWidth = EM_ASM_INT({
        var canvas = document.querySelector(UTF8ToString($0));
        if (canvas.width !== canvas.clientWidth) {
            canvas.width = canvas.clientWidth;
        }
        return canvas.clientWidth;
    }, EMSCRIPTEN_CANVAS_NAME);
    
    *pHeight = EM_ASM_INT({
        var canvas = document.querySelector(UTF8ToString($0));
        if (canvas.height !== canvas.clientHeight) {
            canvas.height = canvas.clientHeight;
        }
        return canvas.clientHeight;
    }, EMSCRIPTEN_CANVAS_NAME);
#else
    SDL_GetWindowSize(mWindow, pWidth, pHeight);
#endif
}

void SDL2::getSizeForRenderer(int *pWidth, int *pHeight)
{
#ifdef __EMSCRIPTEN__
    *pWidth = EM_ASM_INT({
        var canvas = document.querySelector(UTF8ToString($0));
        if (canvas.width !== canvas.clientWidth) {
            canvas.width = canvas.clientWidth;
        }
        return canvas.clientWidth;
    }, EMSCRIPTEN_CANVAS_NAME);
    
    *pHeight = EM_ASM_INT({
        var canvas = document.querySelector(UTF8ToString($0));
        if (canvas.height !== canvas.clientHeight) {
            canvas.height = canvas.clientHeight;
        }
        return canvas.clientHeight;
    }, EMSCRIPTEN_CANVAS_NAME);
#else
    if (mRendererType == Type::OpenGL || mRendererType == Type::GLES) {
        SDL_GL_GetDrawableSize(mWindow, pWidth, pHeight);
    }
    else if (mRendererType == Type::Vulkan) {
#ifdef RENDERER_USE_VULKAN
        SDL_Vulkan_GetDrawableSize(mWindow, pWidth, pHeight);
#endif
    }
    else if (mRendererType == Type::NanoVG) {
#ifdef NANOVG_GL
        SDL_GL_GetDrawableSize(mWindow, pWidth, pHeight);
#else
        SDL_GetWindowSize(mWindow, pWidth, pHeight);
#endif
    }
    else {
        SDL_GetWindowSize(mWindow, pWidth, pHeight);
    }
#endif
}

void SDL2::getDisplayDPI(float *hdpi, float *vdpi, float *ddpi)
{
    int display = SDL_GetWindowDisplayIndex(mWindow);
    if (display < 0) {
        throw std::runtime_error(std::string("Target::SDL2] Could not retrieve window display index: ") + SDL_GetError());
    }

    if (SDL_GetDisplayDPI(display, ddpi, hdpi, vdpi) < 0) {
        // std::cout << "Target::SDL2] Could not retrieve display DPI: " << SDL_GetError() << std::endl;

        constexpr int dpi = 96;

        *hdpi = *vdpi = dpi;
        *ddpi = sqrt(2 * dpi * dpi);
    }
}

void SDL2::create()
{
    uint32_t backendFlag = 0;

    switch (mRendererType) {
    case Type::OpenGL:
    case Type::GLES:
    case Type::SDL2:
#ifdef NANOVG_GL
    case Type::NanoVG:
#endif
        backendFlag = SDL_WINDOW_OPENGL; 
        break;
    case Type::Vulkan:
        backendFlag = SDL_WINDOW_VULKAN;
        break;
    }

#if defined(ANDROID) || defined(__ANDROID__)
    SDL_DisplayMode mode;
    SDL_GetDisplayMode(0, 0, &mode);
    mWidth = mode.w;
    mHeight = mode.h;
#endif

    mWindow = SDL_CreateWindow(
            mTitle.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            mWidth,
            mHeight,
            backendFlag | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    checkError(mWindow == nullptr);

    mGotQuitEvent = false;
    mGotCloseEvent = false;
    mWindowSizeChanged = false;
}

void SDL2::show()
{
    SDL_ShowWindow(mWindow);
}

void SDL2::hide()
{
    SDL_HideWindow(mWindow);
}

void SDL2::close()
{
    SDL_DestroyWindow(mWindow);
}

void SDL2::processEvents()
{
    uint32_t windowId = SDL_GetWindowID(mWindow);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        globalEvents.push_back(event);
    }

    auto it = globalEvents.begin();
    while (it != globalEvents.end()) {
        auto& event = *it;

        bool deleteEvent = true;

        if (event.type == SDL_QUIT) {
            mGotQuitEvent = true;
        }
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.windowID == windowId) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    mGotCloseEvent = true;
                }
                else {
                    mKeyState[event.key.keysym.scancode] = true;
                }
            }
            else {
                deleteEvent = false;
            }
        }
        else if (event.type == SDL_KEYUP) {
            if (event.key.windowID == windowId) {
                if (event.key.keysym.scancode != SDL_SCANCODE_ESCAPE) {
                    mKeyState[event.key.keysym.scancode] = false;
                }
            }
            else {
                deleteEvent = false;
            }
        }
        else if (event.type == SDL_WINDOWEVENT) {
            if (event.window.windowID == windowId) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    mGotCloseEvent = true;
                }
                else if (event.window.event == SDL_WINDOWEVENT_RESIZED
                        || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    mWindowSizeChanged = true;
                }
            }
            else {
                deleteEvent = false;
            }
        }
        else {
            deleteEvent = true;
        }

        if (deleteEvent) {
            it = globalEvents.erase(it);
        }
        else {
            ++it;
        }
    }

    mMouseBitmask = SDL_GetMouseState(&mMouseX, &mMouseY);
}

bool SDL2::shouldQuit()
{
    return mGotQuitEvent;
}

bool SDL2::shouldClose()
{
    return mGotCloseEvent;
}

bool SDL2::sizeChanged()
{
    if (mWindowSizeChanged) {
        mWindowSizeChanged = false;
        return true;
    }
    return false;
}

bool SDL2::isKeyPressed(uint32_t key)
{
    auto it = mKeyState.find((SDL_Scancode) key);
    if (it == mKeyState.end()) {
        return false;
    }
    return it->second;
}

bool SDL2::isMousePressed(uint32_t button)
{
    return mMouseBitmask & SDL_BUTTON(button);
}

std::pair<int, int> SDL2::getMousePosition()
{
    return {mMouseX, mMouseY};
}

void SDL2::checkError(bool cond)
{
    if (cond) {
        throw std::runtime_error(std::string("Target::SDL2] ") + SDL_GetError());
    }
}

