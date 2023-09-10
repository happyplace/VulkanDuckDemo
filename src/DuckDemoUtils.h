#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <SDL_assert.h>
#include <SDL_log.h>
#include <SDL_messagebox.h>

struct SDL_Window;

#define DUCK_DEMO_SHOW_ERROR(title, message) \
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, std::string(title).c_str(), std::string(message).c_str(), DuckDemoUtils::GetWindow()) < 0) \
    { \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\t[SDL_ShowSimpleMessageBox] %s", SDL_GetError()); \
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\n\t[Title]   %s\n\t[Message] %s", std::string(title).c_str(), std::string(message).c_str()); \
    }

#define DUCK_DEMO_ASSERT SDL_assert

namespace DuckDemoUtils
{
    SDL_Window* GetWindow();

    template<typename ... Args>
    std::string format(const std::string& format, Args ... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if(size_s <= 0)
        { 
            throw std::runtime_error("Error during formatting.");
        }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }
}
