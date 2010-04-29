#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "google.h"

int main(int argc, char **argv)
{
	char *auth_str, *pattern, *rc;
	GKeyFile *conf;
	gboolean ret;
	struct google_account_t *account;
	GSList *contacts, *matches;
	int i = 0;
	struct cont_node* contact;

	if (2 != argc) {
		fprintf(stderr, "usage: %s pattern\n", argv[0]);
		exit(1);
	} else {
		pattern = argv[1];
	}

	rc = g_strdup_printf("%s/.goocaarc", g_get_home_dir());

	conf = g_key_file_new();
	ret = g_key_file_load_from_file(conf, rc, G_KEY_FILE_NONE, NULL);

	if (ret == FALSE) {
		fprintf(stderr, "Failed to load config file: %s - %s\n", rc, g_strerror(errno));
		exit(1);
	}

	account = malloc(sizeof(struct google_account_t));
	account->email = g_key_file_get_string(conf, 
			"default", "email", NULL);
	account->passwd = g_key_file_get_string(conf,
			"default", "passwd", NULL);

	google_init();

	auth_str = google_client_login(account);

	if (auth_str == NULL) {
		fprintf(stderr, "Authentification error\n");
		exit(1);
	}

	matches = NULL;
	contacts = google_contacts_full(auth_str);

	while ((contact = g_slist_nth_data(contacts, i++)) != NULL) {
		gboolean match = FALSE;
	
		if ((contact->email != NULL) &&
				(strstr(contact->email, 
					      pattern) != NULL))
			match = TRUE;

		if ((contact->title != NULL) &&
				(strstr(contact->email, pattern) != NULL))
			match = TRUE;

		if (match == TRUE)
			matches = g_slist_append(matches, contact);
	}
	
	printf("%d match(es)\n", g_slist_length(matches));

	i = 0;
	while ((contact = g_slist_nth_data(matches, i++)) != NULL) {
			char *email;
			char *title;

			email = contact->email;
			
			if ((contact->title == NULL) || strlen(contact->title) == 0)
				title = email;
			else
				title = contact->title;


			printf("%s\t%s\n", email, title);
	}

	google_destroy();

	return 0;
}
