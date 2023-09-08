#include <stdio.h>
#define EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Efl_Ui.h>
#include <Elementary.h>
#include "replay.h"
#include "home.h"

int opt_mallocd = -1;
char* game_path = "SSBM.iso";
char* dolphin_emu_file = "slippi-netplay-dolphin";
char* dolphin_replay_file = "slippi-playback-dolphin";

Evas_Object* mainer;
Evas_Object* win;
Evas_Object* _tab_curr;
//extern Evas_Object* tab_home;
//extern Evas_Object* tab_replays;
Evas_Object* tab_config;

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
_tab_switch_cb(void *_data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	Evas_Object** data = _data;
	elm_obj_box_unpack_all(mainer);
	evas_object_hide(_tab_curr);
	_tab_curr = *data;
	
	evas_object_size_hint_weight_set(_tab_curr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	
	//elm_obj_box_recalculate(mainer);
	evas_object_show(mainer);
	evas_object_show(_tab_curr);
	elm_obj_box_pack_end(mainer, _tab_curr);
	
	//evas_object_move(_tab_curr, 100, 15);
	//evas_object_resize(_tab_curr, 100, 100);
	
	//Evas_Object* test = elm_button_add(mainer);
	//elm_object_text_set(test, "Test button");	
	//evas_object_show(test);	
	//elm_obj_box_pack_end(mainer, test);
}

void
_launch_slippi_job_end_cb(void *data, Ecore_Thread *thread)
{
	//Re-enable button so we can start again
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

static void
_launch_slippi_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
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
	tab_home_setup(mainer);
	
	

	// BEGIN tab_replays
	tab_replays_setup(mainer);
	// END tab_replays
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
	// END main

	// BEGIN toolbar
	tb_box = elm_box_add(win);
	tb = elm_toolbar_add(tb_box);
	elm_box_horizontal_set(tb_box, EINA_TRUE);
	
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
	
	that = elm_button_add(tb_box);
	elm_object_text_set(that, "Play");
	evas_object_smart_callback_add(that, "clicked", _launch_slippi_cb, that);
	Evas_Object* icon = elm_icon_add(win);
	elm_icon_standard_set(icon, "input-gaming");
	elm_object_part_content_set(that, "icon", icon);
	evas_object_show(that);
	 elm_box_pack_end(tb_box, that);
	
	 elm_box_pack_end(tb_box, tb);
	
	that = elm_clock_add(tb_box);
	evas_object_show(that);
	 elm_box_pack_end(tb_box, that);
	
	 elm_box_pack_end(main, tb_box);
	
	evas_object_show(tb_box);
	evas_object_show(tb);
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
	elm_run();
	
	if (opt_mallocd >= 0) free(game_path);
	if (opt_mallocd >= 1) free(dolphin_emu_file);
	if (opt_mallocd >= 2) free(dolphin_replay_file);
	
	return 0;
}
ELM_MAIN()
