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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>

#include "cache.h"
#include "google.h"

int main(int argc, char **argv)
{
	char *auth_str, *pattern, *rc, *profile = "default";
	GKeyFile *conf;
	gboolean ret, quiet = FALSE, require_title = FALSE;
	struct google_account_t *account;
	GSList *contacts, *matches;
	int i = 0;
	struct cont_node* contact;
	GError *error = NULL;
	int ch;
	struct cache_t* cache;

	while ((ch = getopt(argc, argv, "p:qt")) != -1) {
		switch (ch) {
			case 'p':
				profile = optarg;
				break;
			case 'q':
				quiet = TRUE;
				break;
			case 't':
				require_title = TRUE;
				break;
			default:
				fprintf(stderr, "getopt error\n");
		}
	}

	argc -= optind;
	argv += optind;

	if (1 != argc) {
		fprintf(stderr, "usage: goocaa [-qt] [-p profile] pattern\n");
		exit(1);
	} else {
		pattern = argv[0];
	}

	rc = g_strdup_printf("%s/.goocaarc", g_get_home_dir());

	conf = g_key_file_new();
	ret = g_key_file_load_from_file(conf, rc, G_KEY_FILE_NONE, &error);

	if (FALSE == ret) {
		fprintf(stderr, "Failed to load config file: %s - %s\n", rc, error->message);
		g_error_free(error);
		exit(1);
	}

	if (FALSE == g_key_file_has_group(conf, profile)) {
		fprintf(stderr, "Profile '%s' is not defined in the config file\n", profile);
		exit(1);
	}

	account = malloc(sizeof(struct google_account_t));
	account->email = g_key_file_get_string(conf,
			profile, "email", &error);
	account->passwd = g_key_file_get_string(conf,
			profile, "passwd", &error);

	if ((account->email == NULL) || (account->passwd == NULL)) {
		fprintf(stderr, "Error reading info from config file, profile: %s\n", profile);
		
		g_error_free(error);
		free(account);
		exit(1);
	}

	cache = cache_load(profile);

	if ((cache->contacts == NULL) || (time(NULL) - cache->modtime > 60*60*24)) {
		/* cache is outdated or doesn't exist, fetch info */	
		google_init();

		auth_str = google_client_login(account);

		if (auth_str == NULL) {
			fprintf(stderr, "Authentification error\n");
			exit(1);
		}

		contacts = google_contacts_full(auth_str);
	
		google_destroy();

		cache_dump(profile, contacts);
	} else
		contacts = cache->contacts;

	matches = NULL;

	while ((contact = g_slist_nth_data(contacts, i++)) != NULL) {
		gboolean match = FALSE;

		if ((contact->email != NULL) &&
				(strcasestr(contact->email,
					      pattern)))
			match = TRUE;

		if ((contact->title != NULL) &&
		       (strcasestr(contact->title, pattern)))
				match = TRUE;

		if (match == TRUE)
			matches = g_slist_append(matches, contact);
	}
	
	if (!quiet)
		printf("%d match(es) for '%s'\n", g_slist_length(matches), pattern);

	i = 0;
	while ((contact = g_slist_nth_data(matches, i++)) != NULL) {
			char *email;
			char *title;

			if (require_title && (contact->title == NULL || strlen(contact->title) == 0))
				continue;

			email = contact->email;
			
			if ((contact->title == NULL) || strlen(contact->title) == 0)
				title = email;
			else
				title = contact->title;


			if (quiet)
				printf("\"%s\" <%s>\n", title, email);
			else
				printf("%s\t%s\n", email, title);
	}

	g_key_file_free(conf);

	return 0;
}
