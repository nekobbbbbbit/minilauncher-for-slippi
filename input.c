#include <Ecore.h>
#include <Elementary.h>
#include "input.h"

Ecore_Event_Key* ev = NULL;
int event_ready = 0;
static SDL_GameController* gamepad;
extern Evas_Object* win, *mainer, *_tab_curr, *tab_scroller;
void _launch_slippi_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);
float y_mod = 0.0f;

void
free_ev(void* that, void* who_even_cares EINA_UNUSED)
{
	free(that);
}


Eina_Bool
_scroll_loop_cb(void* data)
{
	int x, y, w, h;
	elm_scroller_region_get(tab_scroller, &x, &y, &w, &h);
	y += y_mod;
	elm_scroller_region_show(tab_scroller, x, y, w, h);
	return EINA_TRUE;
}

void
input_sdl_init_thread()
{
	ecore_timer_add(.005, _scroll_loop_cb, NULL);
	Ecore_Thread* thread = ecore_thread_run(_input_sdl_setup_thread, NULL, NULL, NULL);
}

static void
input_sdl_eventloop(Ecore_Thread *thread)
{
	SDL_Event e;
	while (1)
	{
	while (SDL_PollEvent(&e))
	{
		ecore_thread_main_loop_begin();
		switch (e.type) {
		case SDL_CONTROLLERDEVICEADDED:
			if (!gamepad) {
        		gamepad = SDL_GameControllerOpen(e.cdevice.which);
        		printf("Found controller.\n");
        	}
    		break;
    	case SDL_CONTROLLERDEVICEREMOVED:
    		if (gamepad) {
            	SDL_GameControllerClose(gamepad);
        		gamepad = NULL;
        		printf("Removed controller.\n");
        	}
    		break;
		case SDL_CONTROLLERAXISMOTION:
		{
			int deadzone = 3000;
			float r_y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);
			
			if (abs(r_y) > deadzone)
			{
				y_mod = r_y / 4000;
			}
			else y_mod = 0;
		}
		
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		{
			Evas_Object* focused = win;
			// Setup Event
			ev = calloc(1, sizeof(Ecore_Event_Key));
			ev->timestamp = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);
			ev->window = elm_win_window_id_get(win);
			ev->event_window = elm_win_window_id_get(win);

			// Handle
			switch (e.cbutton.button)
			{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				ev->key = "Up";
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				ev->key = "Right";
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				ev->key = "Down";
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				ev->key = "Left";
				break;
			case SDL_CONTROLLER_BUTTON_A:
				ev->key = "Return";
				break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				prev_tab();
				break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				next_tab();
				break;
			case SDL_CONTROLLER_BUTTON_START:
				// Just what it says on the box
				_launch_slippi_cb(NULL, NULL, NULL);
				break;
			default:
				free(ev);
				ecore_thread_main_loop_end();
				continue;
			}
			ev->keyname = ev->key;
			ecore_event_add(ECORE_EVENT_KEY_DOWN, ev, free_ev, ev);
			break;
		}
		default:
			break;
		}
		ecore_thread_main_loop_end();
	}
	}
}

void
input_sdl_setup()
{
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
	{
        fprintf(stderr, "Couldn't initialize SDL Gamepad: %s\n", SDL_GetError());
        return;
    }
    
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
    	if (SDL_IsGameController(i))
    	{
    		gamepad = SDL_GameControllerOpen(i);
    		printf("%i gamepads were found. Using %s (#%i).\n",
    		       SDL_NumJoysticks(), SDL_GameControllerName(gamepad), i+1);
    		break;
    	}
    }
}

void
_input_sdl_setup_thread(void *data, Ecore_Thread *thread)
{
	input_sdl_setup();
	input_sdl_eventloop(thread);
}
