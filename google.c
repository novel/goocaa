#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ne_basic.h>
#include <libxml/parser.h>
#include <glib.h>

#include "google.h"

struct contacts_t {
	GSList *contacts;
};

struct response_data_t {
	char *data;
	size_t len;
	ne_buffer *buf;
};

void process_entry(xmlNode *node, struct contacts_t*);

char* process_contacts(xmlNode *node, struct contacts_t *contacts_data)
{
	xmlNode *cur_node = NULL;
	char *next_url = NULL;

	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			//printf("node type: Element, name: %s\n", cur_node->name);
			if (strcmp("entry", (char *)cur_node->name) == 0) {
				process_entry(cur_node, contacts_data);
			} else if (strcmp("link", (char *)cur_node->name) == 0) {
				//printf("got link!\n");
				if (strcmp((char*)xmlGetProp(cur_node, (const xmlChar *)"rel"), 
							"next") == 0) {
					int j = 0;
					char *next = (char *)xmlGetProp(cur_node, 
							(const xmlChar *)"href");

					while (j != 3)
						if (*(next++) == '/')
							j++;

					next_url = strdup(next - 1);
				}
			}
		}
	}

	return next_url;
}

void process_entry(xmlNode *node, struct contacts_t *contacts_data)
{
	xmlNode *cur_node = NULL;

	struct cont_node *contact;

	contact = malloc(sizeof(struct cont_node));
	contact->title = NULL;
	contact->email = NULL;

	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (strcmp("email", (char *)cur_node->name) == 0) {
				contact->email = (char *)xmlGetProp(cur_node, 
						(const xmlChar *)"address");
			} else if (strcmp("title", (char *)cur_node->name) == 0) {
				contact->title = (char *)xmlNodeGetContent(cur_node);
			}
		}
	}

	contacts_data->contacts = g_slist_append(contacts_data->contacts, contact);
}

static char *extract_auth_token(const char *response)
{
	char *auth_str;
	char *ret;

	auth_str = strstr(response, "Auth=");
	if (auth_str == NULL)
		return NULL;

	ret = strndup(auth_str + 5, strlen(auth_str) - 6);

	return ret;
}

int cb(void *userdata, const char *buf, size_t len) {
	struct response_data_t *resp_data = userdata;

	if (len != 0)
		ne_buffer_append(resp_data->buf, buf, len);

	return 0;
}

void google_init()
{
	ne_sock_init();
}

char *google_client_login(struct google_account_t *account)
{
	ne_session *sess;
	ne_request *req;
	struct response_data_t *resp_data;
	char *ret;
	char *data;

	data = g_strdup_printf("accountType=HOSTED_OR_GOOGLE&Email=%s&Passwd=%s&service=cp&source=novel-goocaa-1", account->email, account->passwd);

	sess = ne_session_create("https", "www.google.com", 443);
	ne_ssl_trust_default_ca(sess);

	req = ne_request_create(sess, "POST", "/accounts/ClientLogin");
	ne_set_request_flag(req, NE_REQFLAG_IDEMPOTENT, 0);
	ne_add_request_header(req, "Content-Type", "application/x-www-form-urlencoded");
	ne_set_request_body_buffer(req, data, strlen(data));

	resp_data = malloc(sizeof(struct response_data_t));
	resp_data->buf = ne_buffer_create();

	ne_add_response_body_reader(req, ne_accept_always, cb, resp_data);

	if (ne_request_dispatch(req)) {
		   printf("Request failed: %s\n", ne_get_error(sess));
		   exit(1);
	}

	ret = extract_auth_token(resp_data->buf->data);

	ne_request_destroy(req);

	return ret;
}

#if 0
void google_contacts_full(const char *auth_token)
{
//	printf("hello world\n");
	struct contacts_t *contacts_data;

	contacts_data = malloc(sizeof(struct contacts_t));
	contacts_data->contacts = NULL;

	google_contacts_full_(auth_token);
}
#endif

GSList* google_contacts_full(const char *auth_token)
{
	ne_session *sess;
	ne_request *req;
	const char *auth_prefix = "GoogleLogin auth=";
	char *auth_header;
	size_t auth_header_length;
	struct response_data_t *resp_data;
	struct contacts_t *contacts_data;
	char *url;

	contacts_data = malloc(sizeof(struct contacts_t));
	contacts_data->contacts = NULL;

	sess = ne_session_create("https", "www.google.com", 443);
	ne_ssl_trust_default_ca(sess);

	url = strdup("/m8/feeds/contacts/default/full");

	while (url != NULL) {
		char *next_url;

		req = ne_request_create(sess, "GET", url);

		auth_header_length = strlen(auth_prefix) + strlen(auth_token) + 1;
		auth_header = (char *)malloc(auth_header_length * sizeof(char*));
		snprintf(auth_header, auth_header_length, "%s%s", 
				auth_prefix, auth_token);

		ne_add_request_header(req, "Authorization", auth_header);

		resp_data = malloc(sizeof(struct response_data_t));
		resp_data->buf = ne_buffer_create();

		ne_add_response_body_reader(req, ne_accept_always, cb, resp_data);

		if (ne_request_dispatch(req) != 0) {
			   printf("Request failed: %s\n", ne_get_error(sess));
		}

		xmlDoc *doc = NULL;
		xmlNode *root_element = NULL;

		doc = xmlReadMemory(resp_data->buf->data, resp_data->buf->length, "", NULL, 0);
		//printf("====\n%s\n======\n", resp_data->buf->data);

		root_element = xmlDocGetRootElement(doc);

		next_url = process_contacts(root_element, contacts_data);
		//printf("next_url = %s\n", next_url);

		free(url);
		//url = NULL;
		url = next_url;
		
		ne_request_destroy(req);
	}
	
	return contacts_data->contacts;
}

void google_destroy()
{
	ne_sock_exit();
}
