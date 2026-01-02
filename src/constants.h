#ifndef CONSTANTS_H
#define CONSTANTS_H

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
const float NAUSEA_SPAWN_PROBABILITY_FAST = 0.05f;

const float INITIAL_SPEED_FACTOR = 0.5f;
const float SPEED_INCREMENT_PER_PAGE = 0.005f;
#endif  // CONSTANTS_H
