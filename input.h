#ifndef INPUT_H
#define INPUT_H
#include <SDL2/SDL.h>

void
input_init_threads();

void
input_sdl_setup();

void
_input_sdl_setup_thread();

void
input_gcadapter_setup();

void
_input_gcadapter_setup_thread(void *data, Ecore_Thread *thread);

#endif /* INPUT_H */
