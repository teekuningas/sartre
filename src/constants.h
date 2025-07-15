#ifndef CONSTANTS_H
#define CONSTANTS_H

static const char* textVertexShaderSource =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n"
;

static const char* textFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D textTexture;\n"
    "uniform vec4 textColor;\n"
    "void main() {\n"
    "    vec4 sampled = texture2D(textTexture, fragTexCoord);\n"
    "    gl_FragColor = textColor * sampled;\n"
    "}\n"
;

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
    "}\n"
;

static const char* forestFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "    vec4 texColor = texture2D(ourTexture, fragTexCoord);\n"
    "    if (texColor.a <= 0.1) discard;\n"
    "    gl_FragColor = texColor;\n"
    "}\n"
;

const int MAP_WIDTH = 2048;
const int MAP_HEIGHT = 2048;
const int EARTH_HEIGHT = 50;

// ---- game constants ----
const int SARTRE_WIDTH = 256;
const int SARTRE_HEIGHT = 256;

// —— Sartre movement & physics ——
const float SARTRE_VX = 800.0f;
const float SARTRE_G = 4000.0f;
const float SARTRE_JUMP_VELOCITY = 2100.0f;

const int GAME_OBJECT_WIDTH = 128;
const int GAME_OBJECT_HEIGHT = 128;
const float GAME_OBJECT_AMPLITUDE = 200.0f;
const float GAME_OBJECT_FREQUENCY = 1.5f;

const int NUM_PAGES = 3;
const int NUM_NAUSEOUS_OBJECTS = 5;
const int TOTAL_GAME_OBJECTS = NUM_PAGES + NUM_NAUSEOUS_OBJECTS;
#endif  // CONSTANTS_H
