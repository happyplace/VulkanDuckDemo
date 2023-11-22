#include "DuckDemoUtils.h"

#include <iostream>
#include <fstream>

#include "Game.h"

SDL_Window* DuckDemoUtils::GetWindow()
{
    return Game::Get()->GetWindow();
}

std::unique_ptr<DuckDemoFile> DuckDemoUtils::LoadFileFromDisk(const std::string& path)
{
    std::ifstream file(path, std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
        std::unique_ptr<DuckDemoFile> loadedFile = std::make_unique<DuckDemoFile>();
        loadedFile->bufferSize = file.tellg();
        loadedFile->buffer = std::make_unique<char*>(new char[loadedFile->bufferSize]);
        file.seekg(0, std::ios::beg);
        file.read(*loadedFile->buffer.get(), loadedFile->bufferSize);
        file.close();
        return loadedFile;
    }
    return nullptr;
}
