#if !defined(__GOOGLE_H)
#define __GOOGLE_H

#include <libxml/parser.h>
#include <glib.h>

struct google_account_t {
	char *email;
	char *passwd;
};

void google_init();

char* google_client_login(struct google_account_t *);

GSList* google_contacts_full(const char*);

void google_destroy();

struct cont_node {
	char *title;
	char *email;
};

#endif /* !defined(__GOOGLE_H) */
