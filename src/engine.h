#ifndef ENGINE_H
#define ENGINE_H

#include <string>

#include "types.h"

// Tear down window, GL context, audio/mixer, TTF, delete VAOs/VBOs and shaders.
void shutdownEngine(RenderContext& context);

// Initialize SDL, create window + GL context, load font + sound effects.
// Returns false on any failure.
bool initEngine(RenderContext& context, const std::string& dataPath, bool fullscreen);

// Expose compute_window_params for users of engine.h
WindowParams compute_window_params(bool fullscreen);

#endif  // ENGINE_H
