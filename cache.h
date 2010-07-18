#if !defined(__CACHE_H)
#define __CACHE_H

#include <glib.h>

struct cache_t {
	GSList *contacts;
	time_t modtime;
};

void cache_dump(const char *profile, const GSList *contacts);

struct cache_t *cache_load(const char *profile);

#endif /* !defined(__CACHE_H) */
