#include <Ecore.h>
#include <Elementary.h>
#include "input.h"

static SDL_GameController* gamepad;
extern Evas_Object* win, *mainer, *_tab_curr;
void
_launch_slippi_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);

void
input_sdl_init_thread()
{
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
			switch (e.cbutton.button)
			{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				elm_object_focus_next(focused, ELM_FOCUS_UP);
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
