#if !defined(__GOOGLE_H)
#define __GOOGLE_H

#include <ne_basic.h>
#include <libxml/parser.h>
#include <glib.h>

struct contacts_t {
	GSList *contacts;
};

struct response_data_t {
	char *buf;
	size_t size;
};

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

void process_entry(xmlNode *node, struct contacts_t*);

#endif /* !defined(__GOOGLE_H) */
