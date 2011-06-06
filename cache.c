/*-
 * Copyright (c) 2010-2011 Roman Bogorodskiy
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


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

void cache_dump(const char *profile, const GSList *contacts)
{
	char *cache_file;
	FILE *cache;

	cache_file = g_strdup_printf("%s/.goocaa.%s.cache", g_get_home_dir(), profile);
	cache = fopen(cache_file, "w");

	g_slist_foreach(contacts, print_item, cache);

	g_free(cache_file);
	fclose(cache);
}

struct cache_t *cache_load(const char *profile)
{
	char *cache_file;
	FILE *cache;
	struct cache_t *cache_data;
	char key[256], value[256];

	cache_data = malloc(sizeof(struct cache_t));

	cache_file = g_strdup_printf("%s/.goocaa.%s.cache", g_get_home_dir(), profile);
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
