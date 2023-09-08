#include <stdio.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdint.h>
#include <md4c-html.h>
#include "replay.h"

Evas_Object* tab_home = NULL;
static Evas_Object* tab_home_scr = NULL;
static Evas_Object* tab_home_content = NULL;
char const* home_url = "https://api.github.com/repos/project-slippi/Ishiiruka/releases";

struct memory_chunk {
	char* data;
	size_t size;
};

size_t
write_callback(char* ptr, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct memory_chunk* mem = userp;

	char *nptr = realloc(mem->data, mem->size + realsize + 1);
	if(!nptr) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}
	
	mem->data = nptr;
	memcpy(&(mem->data[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->data[mem->size] = 0;
	return realsize;
}

int
releases_result(struct memory_chunk chunk)
{
	cJSON* json = cJSON_ParseWithLength(chunk.data, chunk.size);
	if (!json)
	{
		fprintf(stderr, "Something happened.\n");
		return 1;
	}
	
	cJSON* release = NULL;
	//cJSON_ArrayForEach(release, json)
	int i = 0;
	for (cJSON* c = json->child; c->next != NULL; (++i, c = c->next))
	{
		if (i == 6)
		{
			Evas_Object* notice = elm_label_add(tab_home_content);
			elm_object_text_set(notice, "Older releases are collapsed, click to expand");
			elm_box_pack_end(tab_home_content, notice);
			evas_object_show(notice);
		}
		Evas_Object* release_fr = elm_frame_add(tab_home_content);
		// Size
		evas_object_size_hint_weight_set(release_fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(release_fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
		// END Size
		elm_frame_autocollapse_set(release_fr, EINA_TRUE);
		if (i > 5)
			elm_frame_collapse_set(release_fr, EINA_TRUE);
		elm_box_pack_end(tab_home_content, release_fr);
		cJSON* title = cJSON_GetObjectItemCaseSensitive(c, "name");
		elm_object_text_set(release_fr, title->valuestring);
		evas_object_show(release_fr);
		
		Evas_Object* fr_box = elm_box_add(release_fr);
		elm_object_content_set(release_fr, fr_box);
		evas_object_show(fr_box);
		
		Evas_Object* content = elm_entry_add(fr_box);
		elm_box_pack_end(fr_box, content);
		elm_entry_editable_set(content, EINA_FALSE);
		evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);
		
		/* Get body_html and manipulate it */
		cJSON* body = cJSON_GetObjectItemCaseSensitive(c, "body_html");
		Eina_Strbuf* body_fmt = eina_strbuf_manage_new(body->valuestring);
		eina_strbuf_replace_all(body_fmt, "\n", "<br>");
		eina_strbuf_trim(body_fmt);
		
		
		elm_object_text_set(content, eina_strbuf_string_get(body_fmt));
		evas_object_show(content);
		
		free(eina_strbuf_release(body_fmt));
		
		//elm_box_pack_end(tab_home_box, fr_box);
	}
	return 0;
}

void
_tab_home_make_da_damn_request()
{

	char curl_errbuf[CURL_ERROR_SIZE];
	int res;
	struct memory_chunk data;
	
	data.data = malloc(1);
	data.size = 0;
	
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, home_url);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
	//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Minilauncher4slippi/1.0");
	//application/vnd.github.html+json
	struct curl_slist *hs=NULL;
	hs = curl_slist_append(hs, "Accept: application/vnd.github.html");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
	
	res = curl_easy_perform(curl);
	
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		                curl_easy_strerror(res));
	}
	else {
		printf("%lu bytes retrieved\n", (unsigned long)data.size);
		/* cleanup curl stuff */
	
		releases_result(data);
		free(data.data);
	}
 
	curl_easy_cleanup(curl);
	
	
	/* we are done with libcurl, so clean it up */
	curl_global_cleanup();
	
}


void
tab_home_setup(Evas_Object* parent)
{
	curl_global_init(CURL_GLOBAL_ALL);
	
	tab_home = elm_box_add(parent); // Scroller
	evas_object_size_hint_weight_set(tab_home, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab_home, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(tab_home);
	
	tab_home_scr = elm_scroller_add(tab_home);
	evas_object_size_hint_weight_set(tab_home_scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab_home_scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(tab_home, tab_home_scr);
	evas_object_show(tab_home_scr);
	
	tab_home_content = elm_box_add(tab_home_scr);
	evas_object_size_hint_weight_set(tab_home_content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab_home_content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(tab_home_content);
	
	elm_object_content_set(tab_home_scr, tab_home_content);
	
	
	//elm_object_content_set(tab_home, tab_home_box);
	//evas_object_show(bt);
	//evas_object_show(tab_home_box);
	//evas_object_show(tab_home);
	
	
   	
   	_tab_home_make_da_damn_request();
}
