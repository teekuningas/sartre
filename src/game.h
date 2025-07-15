#ifndef GAME_H
#define GAME_H

// forward‐declare your big state
struct GameLoopData;

#ifdef __cplusplus
extern "C" {
#endif

// one function that drives each frame
void main_loop_iteration(GameLoopData& data);

#ifdef __cplusplus
}
#endif

#endif  // GAME_H
