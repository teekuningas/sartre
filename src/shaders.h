#ifndef SHADERS_H
#define SHADERS_H

static const char* textVertexShaderSource =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

static const char* textFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D textTexture;\n"
    "uniform vec4 textColor;\n"
    "void main() {\n"
    "    vec4 sampled = texture2D(textTexture, fragTexCoord);\n"
    "    gl_FragColor = textColor * sampled;\n"
    "}\n";

static const char* forestVertexShaderSource =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 model;\n"
    "void main() {\n"
    "    gl_Position = projection * model * vec4(position, 0.0, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

// Fragment shader with named constants instead of magic numbers
static const char* forestFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    // Uniforms
    "uniform float u_nausea;      // [0,1]\n"
    "uniform float u_bliss;       // [0,1]\n"
    "uniform float u_beams;       // [0,1]\n"
    "uniform float u_warpTime;    // sec\n"
    "uniform float u_beamTime;    // sec\n"
    "uniform sampler2D ourTexture;\n"
    "varying vec2 fragTexCoord;\n"
    // Constants
    "#define NAUSEA_SCALE 0.015\n"
    "#define NAUSEA_FREQ 30.0\n"
    "#define GRAY_R 0.299\n"
    "#define GRAY_G 0.587\n"
    "#define GRAY_B 0.114\n"
    "#define BLISS_R 1.0\n"
    "#define BLISS_G 0.98\n"
    "#define BLISS_B 0.85\n"
    "#define BEAM_CENTER_X 0.5\n"
    "#define BEAM_CENTER_Y -0.1\n"
    "#define BEAM_FREQ 6.0\n"
    "#define BEAM_SPEED 0.1\n"
    "#define BEAM_INTENSITY 1.0\n"

    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    float wt = u_warpTime;\n"

    "    // Nausea warp\n"
    "    uv += (u_nausea * NAUSEA_SCALE) * sin(uv.yx * NAUSEA_FREQ + wt);\n"

    "    vec4 col = texture2D(ourTexture, uv);\n"
    "    if (col.a <= 0.1) discard;\n"

    "    // Nausea grayscale\n"
    "    float gray = dot(col.rgb, vec3(GRAY_R, GRAY_G, GRAY_B));\n"
    "    col.rgb = mix(col.rgb, vec3(gray), u_nausea * 0.3);\n"

    "    // Bliss golden glow\n"
    "    col.rgb = mix(col.rgb, vec3(BLISS_R, BLISS_G, BLISS_B), u_bliss * 0.6);\n"
    "    col.rgb += u_bliss * 0.5;\n"

    "    // Light beams\n"
    "    if (u_beams > 0.01) {\n"
    "      vec2 center = vec2(BEAM_CENTER_X, BEAM_CENTER_Y);\n"
    "      vec2 dir = uv - center;\n"
    "      float angle = atan(dir.x, dir.y);\n"
    "      float beams = cos(angle * BEAM_FREQ + u_beamTime * BEAM_SPEED);\n"
    "      beams = beams * beams;\n"
    "      float radial = length(dir);\n"
    "      beams *= smoothstep(1.5, 0.0, radial);\n"
    "      col.rgb += u_beams * beams * BEAM_INTENSITY;\n"
    "    }\n"

    "    col.rgb = clamp(col.rgb, 0.0, 1.0);\n"
    "    gl_FragColor = col;\n"
    "}\n";

#endif  // SHADERS_H
