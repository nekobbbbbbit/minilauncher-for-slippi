#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <assert.h>
#include "replay.h"

Evas_Object* tab_replays = NULL;
char* replays_dir = "replays/";
Ecore_Exe* dolphin_replay_exe;
extern char* dolphin_replay_file, *game_path;

struct replay
{
	char* filename;
	char* p1;
	char* p1code;
	char* p2;
	char* p2code;
	int game_state;
}* replays = NULL;
size_t replays_len = 0;

void
_launch_slippi_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED);
                  
Eina_Bool
_launch_restore_btn(void* data);

unsigned
dec_uint_be(unsigned char* arr, size_t len)
{
        unsigned res = 0;
        for (int i = len-1, j = 0; i >= 0; (--i, ++j))
        {
                res |= arr[i] << (j*8);
        }
        return res;
}

long
fsize(FILE* FP)
{
        long _ = ftell(FP);
        fseek(FP, 0, SEEK_END);
        long new_size = ftell(FP);
        fseek(FP, _, SEEK_SET);
        return new_size;
}

char*
sstrstr(char* haystack, char* needle, size_t length)
{
	size_t needle_length = strlen(needle);
	size_t i;
	for (i = 0; i < length; i++) {
		if (i + needle_length > length) {
			return NULL;
		}
		if (strncmp(&haystack[i], needle, needle_length) == 0) {
			return &haystack[i];
		}
	}
	return NULL;
}

char*
ubjson_search(unsigned char* buf, long length, char const* key, long* offset)
{
	size_t key_len = strlen(key);
	char* start = sstrstr(buf, key, length);
	if (!start) return NULL;
	start += key_len;
	// Just 8 bits for now...
	uint8_t value_len = start[0];
	start++;
	
	char* value = calloc(1, value_len + 1);
	memcpy(value, start, value_len);
	start += value_len;
	
	if (offset)
		*offset = (long)start - (long)buf;
	
	return value;
}

static int
parse_replay(struct replay* rpy)
{
	unsigned char buf[1024] = {0};
	char* filename;
	asprintf(&filename, "%s%s", replays_dir, rpy->filename);
	FILE* SLP = fopen(filename, "r");
	if (!SLP)
	{	
		perror("fopen");
		return 1;
	}
	// We used stat at some point, but a filename comparison is easier to write
	//assert(stat(filename, &(rpy->attr)) == 0);

	long filesize = fsize(SLP);
	
	// Skip begin header and raw data
	fseek(SLP, 11, SEEK_CUR);
	fread(buf, 1, 4, SLP);
	fseek(SLP, dec_uint_be(buf, 4), SEEK_CUR);
	fseek(SLP, -6, SEEK_CUR);
	fread(buf, 1, 1, SLP);
	fseek(SLP, 5, SEEK_CUR);
	rpy->game_state = buf[0];
	
	// debug
	long length = filesize - ftell(SLP);
	fread(buf, 1, length, SLP);	
	long off;
	char* bufx = buf;
	char* p1name = ubjson_search(bufx, length, "netplaySU", &off);
	length -= off; bufx += off;
	char* p1code = ubjson_search(bufx, length, "codeSU", &off);
	length -= off; bufx += off;
	
	char* p2name = ubjson_search(bufx, length, "netplaySU", &off);
	length -= off; bufx += off;
	char* p2code = ubjson_search(bufx, length, "codeSU", &off);
	length -= off; bufx += off;
	
	rpy->p1 = p1name;
	rpy->p1code = p1code;
	rpy->p2 = p2name;
	rpy->p2code = p2code;
	
	//printf("Reading %s\n", filename);
abort:
	fclose(SLP);
	return 0;
}

int
order_filename_greatest(void const* _rpy1, void const* _rpy2)
{
	struct replay* rpy1 = _rpy1, * rpy2 = _rpy2;
	return -strcmp(rpy1->filename, rpy2->filename);
}

static int
recurse_replay_files()
{
	DIR *dp;
	struct dirent *ep;
	dp = opendir(replays_dir);
	if (dp != NULL)
	{
    	while ((ep = readdir(dp)) != NULL)
    	{
    		if (ep->d_name && ep->d_name[0] != '.')
    		{
    			replays = realloc(replays, sizeof(struct replay) * (replays_len + 2));
    			struct replay* rpy = replays + replays_len++;
    			rpy->filename = strdup(ep->d_name);

    			// We used to parse replays as is, but now we sort by date.
    			parse_replay(rpy);
			}
		}
		
		// Lol. Lmao, even.
		qsort(replays, replays_len, sizeof(struct replay),
		      order_filename_greatest);
		      
		closedir(dp);
		return 0;
	}
}

char const*
gameend2str(int code)
{
	switch (code)
	{
	case 0: return "Unresolved";
	case 1: return "TIME!";
	case 2: return "GAME!";
	case 3: return "Resolved.";
	case 7: return "No Contest.";
	}
}

static char*
replays_strings(void* data, Evas_Object* obj, const char* part)
{
	long idx = data;
	// Check this is text for the part we're expecting
	if (strcmp(part, "elm.text") == 0 &&
	    /* DQ after join */
	    (replays[idx].p1 != NULL && replays[idx].p2 != NULL))
	{
		char* c;
		asprintf(&c, "%s (%s) Vs. %s (%s)",
		         replays[idx].p1, replays[idx].p1code,
		         replays[idx].p2, replays[idx].p2code);
		return c;
	}
	else if (replays[idx].p1 == NULL || replays[idx].p2 == NULL)
		printf("[Replay Warning] %s is a incomplete replay. Good idea to delete it.\n",
		       replays[idx].filename);
	else
		return NULL;
}


static void
_launch_replay_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	//if (data)
	//	elm_object_disabled_set(data, EINA_TRUE);
	
	//ecore_timer_add(3, _launch_restore_btn, data);
	
	Eina_Strbuf* exe = eina_strbuf_new();
	eina_strbuf_append_printf(exe, "%s -e %s -i play.json -b", dolphin_replay_file, game_path);
	dolphin_replay_exe = ecore_exe_run(eina_strbuf_release(exe), data);
}

static void
_item_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	long idx = data;
	char* jason;
	asprintf(&jason, "{\"replay\":\"%s%s\"}\n", replays_dir, replays[idx].filename);
	
	// Write to file
	FILE* outfile = fopen("play.json", "w");
	fwrite(jason, 1, strlen(jason), outfile);
	fclose(outfile);
	
	_launch_replay_cb(obj, obj, NULL);
	
	free(jason);
}

Evas_Object*
tab_replays_setup(Evas_Object* parent)
{
	tab_replays = elm_genlist_add(parent);
	elm_genlist_homogeneous_set(tab_replays, EINA_TRUE);
	evas_object_resize(tab_replays, 40, 40);
	Elm_Genlist_Item_Class *_itc = elm_genlist_item_class_new();
	_itc->item_style = "default";
	_itc->func.text_get = replays_strings;
	_itc->func.content_get = NULL;
	_itc->func.state_get = NULL;
	_itc->func.del = NULL;
	
   	recurse_replay_files();
   	
	for (int i = 0; i < replays_len; ++i)
	{
		elm_genlist_item_append(tab_replays,
			_itc,
			i,                    // Item data
			NULL,                    // Parent item for trees, NULL if none
			ELM_GENLIST_ITEM_NONE,   // Item type; this is the common one
			_item_select_cb,                    // Callback on selection of the item
			(long)i                     // Data for that callback function
		);
	}
	
   	evas_object_size_hint_align_set(tab_replays, EVAS_HINT_FILL, EVAS_HINT_FILL);
   	return tab_replays;
}
