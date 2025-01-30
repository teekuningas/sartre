#ifndef CONSTANTS_H
#define CONSTANTS_H

// Vertex Shader Source for Text Rendering
const char* textVertexShaderSource =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

// Fragment Shader Source for Text Rendering
const char* textFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D textTexture;\n"
    "uniform vec4 textColor;\n"
    "void main() {\n"
    "    vec4 sampled = texture2D(textTexture, fragTexCoord);\n"
    "    gl_FragColor = textColor * sampled;\n"
    "}\n";

// Vertex Shader Source for Forest Rendering
const char* forestVertexShaderSource =
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

// Fragment Shader Source for Forest Rendering
const char* forestFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "    vec4 texColor = texture2D(ourTexture, fragTexCoord);\n"
    "    if (texColor.a <= 0.1) discard;\n"
    "    gl_FragColor = texColor;\n"
    "}\n";


const int KARTTA_LEVEYS = 2048;
const int KARTTA_KORKEUS = 2048;
const int HAHMO_LEVEYS = 256;
const int HAHMO_KORKEUS = 256;
const int MAA_KORKEUS = 50;

const float HAHMO_VX = 800.0f;
const float HAHMO_G = 4000.0f;
const float HAHMO_HYPPYNOPEUS = 2100.0f;

#endif
