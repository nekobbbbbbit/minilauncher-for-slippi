#include <stdio.h>
#define EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Efl_Ui.h>
#include <Elementary.h>
#include <libusb.h>
#include "replay.h"
#include "home.h"
#ifndef NO_SDL_INPUT
#	include "input.h"
#endif

int opt_mallocd = -1;
char* game_path = "SSBM.iso";
char* dolphin_emu_file = "slippi-netplay-dolphin";
char* dolphin_replay_file = "slippi-playback-dolphin";

Evas_Object* mainer;
Evas_Object* win;
Evas_Object* _tab_curr;
extern Evas_Object* tab_home;
extern Evas_Object* tab_replays;
Evas_Object* tab_scroller;
Evas_Object* _tabs[] = { NULL, NULL };
Evas_Object* _scrollers[] = { NULL, NULL };
int _tabs_len = 2;
int _tabs_i = 0; // home
Evas_Object* tab_config;

void update_tab(Evas_Object*);

void
prev_tab()
{
	--_tabs_i;
	if (_tabs_i == -1)
	{
		_tabs_i = _tabs_len-1;
	}
	update_tab(_tabs[_tabs_i]);
}

void
next_tab()
{
	++_tabs_i;
	if (_tabs_i == _tabs_len)
	{
		_tabs_i = 0;
	}
	update_tab(_tabs[_tabs_i]);
}

int
parse_config(char* file)
{
	FILE* CFG = fopen(file, "r");
	if (!CFG)
	{	
		perror("fopen");
		return 1;
	}
	
	int buf_len = 255;
	char buf[buf_len];
	char* rdpnt;
	for (int i = 0; fgets(buf, buf_len, CFG); ++i) {
		if ((rdpnt = strchr(buf, '\n')))
			*rdpnt = '\0';
		switch (i)
		{
			case 0: game_path = strdup(buf); break;
			case 1: dolphin_emu_file = strdup(buf); break;
			case 2: dolphin_replay_file = strdup(buf); break;
		}
    	++opt_mallocd;
    }
abort:	
	fclose(CFG);
	return 0;
}

void
update_tab(Evas_Object* newtab)
{
	elm_obj_box_unpack_all(mainer);
	evas_object_hide(_tab_curr);
	if (newtab)
	{
		_tab_curr = newtab;
		tab_scroller = _tab_curr;
		evas_object_size_hint_weight_set(_tab_curr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(mainer);
		evas_object_show(_tab_curr);
		elm_obj_box_pack_end(mainer, _tab_curr);
	}
}

void
_tab_switch_cb(void *_data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	Evas_Object** data = _data;
	update_tab(*data);
}

void _prev_tab_cb(void *_data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{ prev_tab(); }
void _next_tab_cb(void *_data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{ next_tab(); }

void
_launch_slippi_job_end_cb(void *data, Ecore_Thread *thread)
{
	//Re-enable button so we can start again
	if (data)
		elm_object_disabled_set(data, EINA_FALSE);
}

// Need to fork in a thread or EFL spergs out
void
_launch_slippi_job_cb(void *data, Ecore_Thread *thread)
{
	char const* argv[64] = {dolphin_emu_file, "-e", game_path, "-b", NULL};
	if (fork() == 0)
	{
		execvp(argv[0], argv);
		exit(0);
	}
	else {
		// Delay thread to prevent from launching again (button is disabled)
		usleep(2000000);
	}
}

void
_launch_slippi_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	if (data)
		elm_object_disabled_set(data, EINA_TRUE);
	ecore_thread_run(_launch_slippi_job_cb,
	                 _launch_slippi_job_end_cb,
	                 _launch_slippi_job_end_cb, data);
}

static void
_replays_tab_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	elm_object_item_disabled_set(*(Elm_Object_Item**)data, EINA_TRUE);
	ecore_thread_run(_launch_slippi_job_cb,
	                 _launch_slippi_job_end_cb,
	                 _launch_slippi_job_end_cb, data);
}


void
tabs_init()
{
	_tabs[0] = tab_home_setup(mainer);
	
	

	// BEGIN tab_replays
	_tabs[1] = tab_replays_setup(mainer);
	
	// Show home tab at start
	_tab_curr = tab_home;
	evas_object_show(_tab_curr);
	elm_obj_box_pack_end(mainer, _tab_curr);
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
	parse_config("minilauncher4slippi.cfg");
	
	/* = win
	 * |
	 * \== bx
	 *   |
	 *   \== tb (toolbar)
	 *   | |
	 *   | \== items
	 *   |   | ...
	 *   |
	 *   \== tb (table)
	 *     |
	 *     \== stuff
	 *
	 */
	Evas_Object *main, *bx, *tb, *tb_box, *that, *menu;
	Evas_Object *ph1, *ph2, *ph3, *ph4;
	Elm_Object_Item *tb_it;
	Elm_Object_Item *menu_it;

	win = elm_win_util_standard_add("minilauncher4slippi", "Minilauncher for Slippi");
	elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
	elm_win_autodel_set(win, EINA_TRUE);

	// BEGIN main
	main = elm_box_add(win);
	evas_object_size_hint_weight_set(main, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	
	elm_win_resize_object_add(win, main);
	evas_object_show(main);
	elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
	elm_win_focus_highlight_animate_set(win, EINA_TRUE);
	// Check for a 'b' character, aka big picture mode, big mode, whatever..
	int bigmode = 0;
	if (argc > 1 && argv[1] && tolower(argv[1][0]) == 'b')
	{
		bigmode = 1;
		elm_win_fullscreen_set(win, EINA_TRUE);
		elm_object_scale_set(win, 1.8);
	}
	// END main

	// BEGIN toolbar
	tb_box = elm_box_add(win);
	elm_box_horizontal_set(tb_box, EINA_TRUE);
	
	if (!bigmode)
	{
	tb = elm_toolbar_add(tb_box);
	
	elm_object_style_set(tb, "item_horizontal");
	elm_toolbar_homogeneous_set(tb, EINA_TRUE);
	elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
	evas_object_size_hint_weight_set(tb, 1.0, 0.0);
	evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(tb_box, 2.0, 0.0);
	evas_object_size_hint_align_set(tb_box, EVAS_HINT_FILL, 0.0);

	tb_it = elm_toolbar_item_append(tb, "home", "Home", _tab_switch_cb, &tab_home);
	elm_toolbar_item_priority_set(tb_it, 100);

	tb_it = elm_toolbar_item_append(tb, "media-seek-backward", "Replays", _tab_switch_cb, &tab_replays);
	elm_toolbar_item_priority_set(tb_it, -100);

	tb_it = elm_toolbar_item_append(tb, "preferences-system", "Settings", _tab_switch_cb, &tab_config);
	elm_toolbar_item_priority_set(tb_it, 150);

	tb_it = elm_toolbar_item_append(tb, "network-wireless", NULL, NULL, NULL);
	elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
	elm_toolbar_item_priority_set(tb_it, -9999);
	elm_toolbar_menu_parent_set(tb, win);
	}
	
	// Play button
	that = elm_button_add(tb_box);
	elm_object_text_set(that, "Play");
	evas_object_smart_callback_add(that, "clicked", _launch_slippi_cb, that);
	Evas_Object* icon = elm_icon_add(win);
	elm_icon_standard_set(icon, "input-gaming");
	elm_icon_resizable_set(icon, EINA_TRUE, EINA_FALSE);
	elm_object_part_content_set(that, "icon", icon);
	evas_object_show(that);
	elm_box_pack_end(tb_box, that);
	// End play button
	
	if (bigmode)
	{
	// Prev button
	that = elm_button_add(tb_box);
	elm_object_text_set(that, "");
	evas_object_smart_callback_add(that, "clicked", _prev_tab_cb, NULL);
	/*ICON*/icon = elm_icon_add(win);
	elm_icon_standard_set(icon, "go-previous");
	elm_icon_resizable_set(icon, EINA_TRUE, EINA_FALSE);
	elm_object_part_content_set(that, "icon", icon);
	/*END ICON*/
	evas_object_show(that);
	elm_box_pack_start(tb_box, that);
	
	// Next button
	that = elm_button_add(tb_box);
	elm_object_text_set(that, "");
	evas_object_smart_callback_add(that, "clicked", _next_tab_cb, that);
	/*ICON*/icon = elm_icon_add(win);
	elm_icon_standard_set(icon, "go-next");
	elm_icon_resizable_set(icon, EINA_TRUE, EINA_FALSE);
	elm_object_part_content_set(that, "icon", icon);
	/*END ICON*/
	evas_object_show(that);
	elm_box_pack_end(tb_box, that);
	}
	
	if (!bigmode)
	{
		elm_box_pack_end(tb_box, tb);
	
		that = elm_clock_add(tb_box);
		evas_object_size_hint_fill_set(that, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(that);
		elm_box_pack_end(tb_box, that);
		evas_object_show(tb);
	}
	
	elm_box_pack_end(main, tb_box);
	
	evas_object_show(tb_box);
	// END toolbar

	mainer = elm_box_add(win);
	//evas_object_size_hint_weight_set(mainer, 0.0, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(mainer, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(mainer, 1.0, 1.0);
	evas_object_size_hint_align_set(mainer, EVAS_HINT_FILL, EVAS_HINT_FILL);
	

	elm_box_pack_start(main, mainer);
	evas_object_show(mainer);
	tabs_init();

	evas_object_resize(win, 520 * elm_config_scale_get(),
    	                    300 * elm_config_scale_get());
	evas_object_show(win);

	printf("[Current config] %s, %s, %s\n", game_path, dolphin_emu_file, dolphin_replay_file);
	
	// Setup input
	input_init_threads();
	
	elm_run();
	
	if (opt_mallocd >= 0) free(game_path);
	if (opt_mallocd >= 1) free(dolphin_emu_file);
	if (opt_mallocd >= 2) free(dolphin_replay_file);
	
	return 0;
}
ELM_MAIN()
