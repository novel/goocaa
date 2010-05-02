#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "google.h"

int main(int argc, char **argv)
{
	char *auth_str, *pattern, *rc, *profile = "default";
	GKeyFile *conf;
	gboolean ret;
	struct google_account_t *account;
	GSList *contacts, *matches;
	int i = 0;
	struct cont_node* contact;
	GError *error = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "p:")) != -1) {
		switch (ch) {
			case 'p':
				profile = optarg;
				break;
			default:
				fprintf(stderr, "getopt error\n");
		}
	}

	argc -= optind;
	argv += optind;

	if (1 != argc) {
		fprintf(stderr, "usage: goocaa pattern\n");
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
		exit(1);
	}

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
		       (strstr(contact->title, pattern) != NULL))
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
