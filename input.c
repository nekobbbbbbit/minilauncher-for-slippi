#include <Ecore.h>
#include <Elementary.h>
#include <libusb.h>
#include "input.h"

Ecore_Event_Key* ev = NULL;
int event_ready = 0;
static SDL_GameController* gamepad;
extern Evas_Object* win, *mainer, *_tab_curr;
void _launch_slippi_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);
float y_mod = 0.0f;
//static libusb_device* gcadapter;


static libusb_device_handle* gcadapter = NULL;
int gc_endpoint_in;
int gc_endpoint_out;

void
free_ev(void* that, void* who_even_cares EINA_UNUSED)
{
	free(that);
}


Eina_Bool
_scroll_loop_cb(void* data)
{
	int x, y, w, h;
	//elm_scroller_region_get(mainer, &x, &y, &w, &h);
	y += y_mod;
	//elm_scroller_region_show(mainer, x, y, w, h);
	return EINA_TRUE;
}

void
input_init_threads()
{
	ecore_timer_add(.005, _scroll_loop_cb, NULL);
	ecore_thread_run(_input_sdl_setup_thread, NULL, NULL, NULL);
	ecore_thread_run(_input_gcadapter_setup_thread, NULL, NULL, NULL);
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
			
			if (fabsf(r_y) > deadzone)
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

// https://github.com/nekobbbbbbit/Ishiiruka/blob/slippi/Source/Core/InputCommon/GCAdapter.cpp
static void
input_gcadapter_eventloop(Ecore_Thread *thread)
{
}

void
input_gcadapter_setup()
{
	libusb_device** devs;
	size_t len;
	int ret;
	libusb_init(NULL);
	len = libusb_get_device_list(NULL, &devs);
	
	if (len < 0) {
		libusb_exit(NULL);
		return;
	}

	
	for (int i = 0; devs[i]; i++)
	{
		libusb_device* dev = devs[i];
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(dev, &desc);
		if (desc.idVendor == 0x057e && desc.idProduct == 0x0337)
		{
			printf("Found gamecube controller with vendor 0x0%X and product 0x0%X!\n",
			       desc.idVendor, desc.idProduct);
			//gcadapter = dev;
			libusb_open(dev, &gcadapter);
			if ((ret = libusb_kernel_driver_active(gcadapter, 0)) == 1)
			{
				// Slippi Ishiiruka temp patch: A fix is underway for Dolphin-emu, but this is workaround
				//  on said systems. Only on FreeBSD systems, not the other BSD's I think
#ifndef __FreeBSD__
				if ((ret = libusb_detach_kernel_driver(gcadapter, 0)) && ret != LIBUSB_ERROR_NOT_SUPPORTED)
				{
					ERROR_LOG(SERIALINTERFACE, "libusb_detach_kernel_driver failed with error: %d", ret);
				}
#endif
				ret = 0; // Need to work around.
			}
			
			// This call makes Nyko-brand (and perhaps other) adapters work.
			// However it returns LIBUSB_ERROR_PIPE with Mayflash adapters.

			const int transfer = libusb_control_transfer(gcadapter, 0x21, 11, 0x0001, 0, NULL, 0, 1000);
			if (transfer < 0)
				;
			
			if (ret != 0 && ret != LIBUSB_ERROR_NOT_SUPPORTED)
			{
				puts("Porbably bad.");
				return;
			}
			else if ((ret = libusb_claim_interface(gcadapter, 0))) {
				puts("Something happened. Check GCAdapter permissions.");
			}
			else {
				puts("And we're hooked!");
				
				// Now do more shit. We're way too nested :^)
				libusb_config_descriptor *config = NULL;
				libusb_get_config_descriptor(dev, 0, &config);
				for (int ic = 0; ic < config->bNumInterfaces; ic++)
				{
					const libusb_interface *interfaceContainer = &config->interface[ic];
					for (int i = 0; i < interfaceContainer->num_altsetting; i++)
					{
						const libusb_interface_descriptor *interface = &interfaceContainer->altsetting[i];
						for (int e = 0; e < interface->bNumEndpoints; e++)
						{
							const libusb_endpoint_descriptor *endpoint = &interface->endpoint[e];
							if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN)
								gc_endpoint_in = endpoint->bEndpointAddress;
							else
								gc_endpoint_out = endpoint->bEndpointAddress;
						}
					}
				}
				
				printf("in:0x%X out:0x%X\n", gc_endpoint_in, gc_endpoint_out);
				
				int tmp;
				unsigned char payload = 0x13;
				ret = libusb_interrupt_transfer(gcadapter, gc_endpoint_out, &payload,
				                          sizeof(payload), &tmp, 32);
				                          
				                          sleep(2);
				              
				while (1)
				{           
				unsigned char rumble[] = {0x11,1,1,1,1};
			                           
				ret = libusb_interrupt_transfer(gcadapter, gc_endpoint_out, rumble, sizeof(rumble), &tmp, 32);
				                          sleep(2);
				unsigned char rumble2[] = {0x11,0,0,0,0};
			                           
				ret = libusb_interrupt_transfer(gcadapter, gc_endpoint_out, rumble2, sizeof(rumble2), &tmp, 32);
				ret = libusb_interrupt_transfer(gcadapter, gc_endpoint_in, rumble2, sizeof(rumble2), &tmp, 32);
				}
				
				// END...
			}
		}
	}

	//libusb_free_device_list(devs, 1);
}


void
_input_sdl_setup_thread(void *data, Ecore_Thread *thread)
{
	//input_sdl_setup();
	//input_sdl_eventloop(thread);
}

void
_input_gcadapter_setup_thread(void *data, Ecore_Thread *thread)
{
	//input_gcadapter_setup();
	//input_gcadapter_eventloop(thread);
}
