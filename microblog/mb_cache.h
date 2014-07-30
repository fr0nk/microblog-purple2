/*
    Copyright 2008-2010, Somsak Sriprayoonsakul <somsaks@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Some part of the code is copied from facebook-pidgin protocols.
    For the facebook-pidgin projects, please see http://code.google.com/p/pidgin-facebookchat/.

    Courtesy to eionrobb at gmail dot com
*/
/*
 * Cache user information locally and make it available for other Twitgin part
 *
 *  Created on: Apr 18, 2010
 *      Author: somsak
 */

#ifndef __MBPURPLE_CACHE__
#define __MBPURPLE_CACHE__

#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <sys/time.h>

#include <glib.h>

typedef struct {
	gchar * user_name; //< owner of this cache entry
	time_t last_update; //< Last cache data update
	time_t last_use; //< Last use of this data
	int avatar_img_id; //< imgstore id
	gchar * avatar_path; //< path name storing this avatar
	gpointer avatar_data; //< pointer to image buffer, will be freed by imgstore
} MbCacheEntry;

typedef struct {
	// Mapping between cache entry and data inside
	// A mapping between MbAccount and TGCacheGroup
	GHashTable * data;
	gboolean fetcher_is_run;
	int avatar_fetch_max;
} MbCache;

struct _MbAccount;

// Initialize cache system
// Currently, this just create directories, so no need for finalize
extern void mb_cache_init(void);

// Get base dir to cache directory
extern const char * mb_cache_base_dir(void);

// Create new cache
extern MbCache * mb_cache_new(void);

// Destroy cache
extern void mb_cache_free(MbCache * mb_cache);

// Insert data to the cache
extern void mb_cache_insert(struct _MbAccount * ma, MbCacheEntry * entry);

// There will be no cache removal for now, since all cache must available almost all the time
extern const MbCacheEntry * mb_cache_get(const struct _MbAccount * ma, const gchar * user_name);


#ifdef __cplusplus
}
#endif

#endif //< MBPURPLE_CACHE
