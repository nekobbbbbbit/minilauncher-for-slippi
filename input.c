#include <Ecore.h>
#include <Elementary.h>
#include "input.h"

Ecore_Event_Key* ev = NULL;
int event_ready = 0;
static SDL_GameController* gamepad;
extern Evas_Object* win, *mainer, *_tab_curr;
void
_launch_slippi_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);

void
bye(void*x, void* y)
{
	printf("Im die\n");
}


Eina_Bool
_timer_cb(void *data)
{
	if (event_ready)
	{
	ev = calloc(1, sizeof(Ecore_Event_Key)*24);
	ev->timestamp = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);
	ev->window = elm_win_window_id_get(win);
	ev->event_window = elm_win_window_id_get(win);
	ev->modifiers = 0;
	ev->key = "Up";
	ev->compose = NULL;
	ev->keyname = "Up";
	ev->string = NULL;
	ev->data = NULL;
	ecore_event_add(ECORE_EVENT_KEY_DOWN, ev, bye, NULL);
	//evas_event_feed_key_down(win, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, ev->data);
	//ecore_event_add(ECORE_EVENT_KEY_UP, ev, NULL, NULL);
	printf("Event sent\n");
	event_ready = 0;
	}
	return EINA_TRUE;
}

static Eina_Bool
help(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *_ev)
{
	Ecore_Event_Key* ev = _ev;
	printf("ev->window: %d\n"
	       "ev->event_window: %d\n"
	       "ev->modifiers: %d\n"
	       "ev->key: %s\n"
	       "ev->compose: %s\n"
	       "ev->keyname: %s\n"
	       "ev->string: %s\n"
	       "ev->data: %p\n", 
	       ev->window, ev->event_window, ev->modifiers,
	       ev->key, ev->compose, ev->keyname, ev->string, ev->data);
	//ecore_main_loop_quit();
	return ECORE_CALLBACK_DONE;
}


void
input_sdl_init_thread()
{
	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, help, NULL);
	ecore_timer_add(.01, _timer_cb, NULL);
	//ev = calloc(1, sizeof(Ecore_Event_Key)*30);
	Ecore_Thread* thread = ecore_thread_run(_input_sdl_setup_thread, NULL, NULL, NULL);
	ecore_thread_local_data_set(thread, "win", win, NULL);
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
        		printf("Found\n");
        	}
    		break;
    	case SDL_CONTROLLERDEVICEREMOVED:
    		if (gamepad) {
            	SDL_GameControllerClose(gamepad);
        		gamepad = NULL;
        		printf("Removed.\n");
        	}
    		break;
		case SDL_CONTROLLERBUTTONDOWN:
		{
			Evas_Object* focused = ecore_thread_local_data_find(thread, "win");
			// Setup Event
			
			
			event_ready = 1;
			
			ecore_thread_main_loop_end();
			printf("Run it back.\n");
			continue;
			

			// Handle
			switch (e.cbutton.button)
			{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				//elm_object_focus_next(focused, ELM_FOCUS_UP);
				//ecore_event_add(ECORE_EVENT_KEY_DOWN, ev, NULL, NULL);
				printf("Up pressed!\n");
				//ecore_event_add(ECORE_EVENT_KEY_UP, );
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				elm_object_focus_next(focused, ELM_FOCUS_RIGHT);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				elm_object_focus_next(focused, ELM_FOCUS_DOWN);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				elm_object_focus_next(focused, ELM_FOCUS_LEFT);
				break;
			case SDL_CONTROLLER_BUTTON_A:
				evas_object_smart_callback_call(elm_object_focused_object_get(focused), "clicked", NULL);
				evas_object_smart_callback_call(elm_object_focused_object_get(focused), "activate", NULL);
				break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				prev_tab();
				break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				next_tab();
				break;
			case SDL_CONTROLLER_BUTTON_START:
				_launch_slippi_cb(NULL, NULL, NULL);
				break;
			}
			//evas_object_smart_callback_call(elm_object_focused_object_get(focused), "selected", elm_object_focused_object_get(focused));
			elm_object_focus_set(elm_object_focused_object_get(focused), EINA_TRUE);
			//elm_object_item_focus_set(elm_object_focused_object_get(focused), EINA_TRUE);
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
