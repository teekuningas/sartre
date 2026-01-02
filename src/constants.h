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

static const char* forestFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform float u_nausea;      // [0,1], how “nauseated” we are\n"
    "uniform float u_bliss;       // [0,1], how “blissful” we are\n"
    "uniform float u_time;        // sec, used to animate the warp\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    float wt = u_time;\n"
    "    // nausea: subtle wavy (affects gameplay)\n"
    "    uv += (u_nausea * 0.015) * sin(uv.yx * 30.0 + wt);\n"
    "    vec4 col = texture2D(ourTexture, uv);\n"
    "    if (col.a <= 0.1) discard;\n"
    "    // Apply nausea effect (subtle desaturate - converts to grayscale)\n"
    "    float gray = dot(col.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    col.rgb = mix(col.rgb, vec3(gray), u_nausea * 0.3);\n"
    "    // Apply bliss effect (golden glow)\n"
    "    col.rgb = mix(col.rgb, vec3(1.0, 0.95, 0.7), u_bliss * 0.4);\n"
    "    col.rgb = clamp(col.rgb + u_bliss * 0.3, 0.0, 1.0);\n"
    "    gl_FragColor = col;\n"
    "}\n";

const int MAP_WIDTH = 2048;
const int MAP_HEIGHT = 2048;
const int EARTH_HEIGHT = 50;

const int SARTRE_WIDTH = 256;
const int SARTRE_HEIGHT = 256;

const float SARTRE_VX = 800.0f;
const float SARTRE_G = 4000.0f;
const float SARTRE_JUMP_VELOCITY = 2100.0f;

const int GAME_OBJECT_WIDTH = 128;
const int GAME_OBJECT_HEIGHT = 128;
const float GAME_OBJECT_AMPLITUDE = 200.0f;
const float GAME_OBJECT_FREQUENCY = 1.5f;

const int NUM_NAUSEA_LIMIT = 3;

const int PAGE_GOAL = 251;
// const int PAGE_GOAL = 5;

const int NUM_PAGES = 3;
const int INITIAL_NUM_NAUSEOUS_OBJECTS = 1;
const float NAUSEA_SPAWN_PROBABILITY_NORMAL = 0.05f;
const float NAUSEA_SPAWN_PROBABILITY_FAST = 0.15f;

const float INITIAL_SPEED_FACTOR = 0.5f;
const float SPEED_INCREMENT_PER_PAGE = 0.005f;
#endif  // CONSTANTS_H
