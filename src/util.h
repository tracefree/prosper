#pragma once

#include <SDL3/SDL.h>

#define print SDL_Log

template <typename T>
using Ref = std::shared_ptr<T>;