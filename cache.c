#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"
#include "google.h"

void print_item(gpointer data, gpointer userdata)
{
	struct cont_node* contact = (struct cont_node*)data;
	FILE *cache = (FILE *)userdata;

	if (contact->email != NULL)
		fprintf(cache, "%s\n%s\n", contact->email, contact->title);
}

void cache_dump(const GSList *contacts)
{
	char *cache_file;
	FILE *cache;

	cache_file = g_strdup_printf("%s/.goocaa.cache", g_get_home_dir());
	cache = fopen(cache_file, "w");

	printf("cache_dump\n");
	g_slist_foreach(contacts, print_item, cache);

	g_free(cache_file);
	fclose(cache);
}

struct cache_t *cache_load()
{
	char *cache_file;
	FILE *cache;
	struct cache_t *cache_data;
	char key[256], value[256];

	cache_data = malloc(sizeof(struct cache_t));

	cache_file = g_strdup_printf("%s/.goocaa.cache", g_get_home_dir());
	cache = fopen(cache_file, "r");

	if (NULL == cache) {
		cache_data->contacts = NULL;
		cache_data->modtime = (time_t)0;

		return cache_data;
	} else {
		struct stat *cache_stat = malloc(sizeof(struct stat));
		fstat(fileno(cache), cache_stat);

		cache_data->modtime = cache_stat->st_mtime;
	}

	while (0 == feof(cache)) {
		struct cont_node *contact;
		
		fgets(key, 255, cache);
		fgets(value, 255, cache);
		
		contact = malloc(sizeof(struct cont_node));
		contact->email = strndup(key, strlen(key)-1);
		contact->title = strndup(value, strlen(value)-1);

		cache_data->contacts = g_slist_append(cache_data->contacts,
				contact);
	}

	g_free(cache_file);

	return cache_data;
}
