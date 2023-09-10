#ifndef REPLAY_H
#define REPLAY_H

#define EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Efl_Ui.h>
#include <Elementary.h>

extern Evas_Object* tab_replays;

Evas_Object*
tab_replays_setup(Evas_Object* parent);

#endif /* REPLAY_H */
