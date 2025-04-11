#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include <iostream>

#include "types.h"

void cleanup_render_context(RenderContext& context);

bool initialize_render_context(RenderContext& context, const std::string& dataPath,
                               bool fullscreen);
#endif
