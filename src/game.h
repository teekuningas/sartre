#ifndef GAME_H
#define GAME_H

// forward‐declare your big state
struct GameLoopData;

// one function that drives each frame
void main_loop_iteration(GameLoopData& data);

#endif  // GAME_H
