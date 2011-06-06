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

#include <libxml/parser.h>
#include <glib.h>
#include <curl/curl.h>

#include "google.h"

char* process_contacts(xmlNode *node, struct contacts_t *contacts_data)
{
	xmlNode *cur_node = NULL;
	char *next_url = NULL;

	for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if (strcmp("entry", (char *)cur_node->name) == 0) {
				process_entry(cur_node, contacts_data);
			} else if (strcmp("link", (char *)cur_node->name) == 0) {
				if (strcmp((char*)xmlGetProp(cur_node, (const xmlChar *)"rel"), 
							"next") == 0) {
					char *next = (char *)xmlGetProp(cur_node, 
							(const xmlChar *)"href");

					next_url = strdup(next);
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

size_t curl_cb(void *buf, size_t size, size_t nmemb, void *userdata) {
	size_t realsize = size * nmemb;
	struct response_data_t *resp_data = userdata;

	resp_data->buf = realloc(resp_data->buf, resp_data->size + realsize + 1);
	memcpy(&(resp_data->buf[resp_data->size]), buf, realsize);
	resp_data->size += realsize;
	resp_data->buf[resp_data->size] = 0;

	return size * nmemb;
}

void google_init()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

char *google_client_login(struct google_account_t *account)
{
	struct response_data_t *resp_data = malloc(sizeof (struct response_data_t));
	char *ret;
	
	char *data;

	CURL *curl;
	CURLcode res;

	resp_data->buf = NULL;
	resp_data->size = 0;

	data = g_strdup_printf("accountType=HOSTED_OR_GOOGLE&Email=%s&Passwd=%s&service=cp&source=novel-goocaa-1",
			account->email, account->passwd);

	curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com/accounts/ClientLogin");
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp_data); 
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
	
	res = curl_easy_perform(curl);	

	curl_easy_cleanup(curl);

	ret = extract_auth_token(resp_data->buf);

	if (resp_data != NULL) {
		if (resp_data->buf != NULL)
			free(resp_data->buf);
		free(resp_data);
	}

	return ret;
}

GSList* google_contacts_full(const char *auth_token)
{
	const char *auth_prefix = "Authorization: GoogleLogin auth=";
	char *auth_header;
	size_t auth_header_length;
	struct contacts_t *contacts_data;
	char *url;
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers = NULL;

	contacts_data = malloc(sizeof(struct contacts_t));
	contacts_data->contacts = NULL;

	/* prepare auth header which is the same for all queries */
	auth_header_length = strlen(auth_prefix) + strlen(auth_token) + 1;
	auth_header = (char *)malloc(auth_header_length * sizeof(char*));
	snprintf(auth_header, auth_header_length, "%s%s", 
			auth_prefix, auth_token);

	headers = curl_slist_append(headers, auth_header);

	/* prepare start url */
	url = strdup("https://www.google.com/m8/feeds/contacts/default/full");

	/* prepare curl */
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_cb);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);

	while (url != NULL) {
		struct response_data_t *resp_data = malloc(sizeof (struct response_data_t));
		char *next_url;

		resp_data->buf = NULL;
		resp_data->size = 0;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp_data); 

		res = curl_easy_perform(curl);

		xmlDoc *doc = NULL;
		xmlNode *root_element = NULL;

		doc = xmlReadMemory(resp_data->buf, resp_data->size, "", NULL, 0);

		root_element = xmlDocGetRootElement(doc);

		next_url = process_contacts(root_element, contacts_data);

		if (url)
			free(url);
		
		if (resp_data) {
			if (resp_data->buf)
				free(resp_data->buf);
			free(resp_data);
		}

		url = next_url;
	}

	/* clean up */
	curl_easy_cleanup(curl);
	
	return contacts_data->contacts;
}

void google_destroy()
{
	curl_global_cleanup();
}
