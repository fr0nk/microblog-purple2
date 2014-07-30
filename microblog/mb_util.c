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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#include <proxy.h>
#include <sslconn.h>
#include <prpl.h>
#include <debug.h>
#include <connection.h>
#include <request.h>
#include <dnsquery.h>
#include <accountopt.h>
#include <xmlnode.h>
#include <version.h>
#include <util.h>

#ifdef _WIN32
#	include <win32dep.h>
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#define DBGID "mb_util"

static const char * month_abb_names[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

static const char * wday_abb_names[] = {
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
	"Sun",
};

time_t mb_mktime(char * time_str)
{
	struct tm msg_time;
	char * cur, * next, *tmp_cur, *tmp_next, oldval;
	int counter = 0,  tmp_counter = 0, i;
	int cur_timezone = 0, sign = 1;
	time_t retval;

	msg_time.tm_isdst = 0;
	cur = time_str;
	next = strchr(cur, ' ');
	while(next) {

		oldval = (*next);
		(*next) = '\0';
		switch(counter) {
			case 0 :
				// day of week
				for(i = 0; i < 7; i++) {
					if(strncasecmp(cur, wday_abb_names[i], 3) == 0) {
						msg_time.tm_wday = i +1;
						break;
					}
				}
				break;
			case 1 :
				//month name
				for(i = 0; i < 12; i++) {
					if(strncasecmp(cur, month_abb_names[i], 3) == 0) {
						msg_time.tm_mon = i;
						break;
					}
				}
				break;
			case 2 :
				// day of month
				msg_time.tm_mday = strtoul(cur, NULL, 10);
				break;
			case 3 :
				// HH:MM:SS
				tmp_cur = cur;
				tmp_next = strchr(cur, ':');
				tmp_counter = 0;
				while(tmp_next) {
					switch(tmp_counter) {
						case 0 :
							msg_time.tm_hour = strtoul(tmp_cur, NULL, 10);
							break;
						case 1 :
							msg_time.tm_min = strtoul(tmp_cur, NULL, 10);
							break;

					}
					tmp_cur = tmp_next + 1;
					tmp_next =strchr(tmp_cur, ':');
					tmp_counter++;
				}
				msg_time.tm_sec = strtoul(tmp_cur, NULL, 10);
				break;
			case 4 :
				// timezone
				if( (*cur) == '+') {
					cur++;
				} else if ( (*cur) == '-') {
					sign = -1;
					cur++;
				}
				cur_timezone = (int)strtol(cur, NULL, 10);
				cur_timezone = sign * (cur_timezone / 100) * 60 * 60 + (cur_timezone % 100) * 60;
				break;
		}
		(*next) = oldval;
		cur = next + 1;
		next = strchr(cur, ' ');
		counter++;
	}
	// what's left is year
	msg_time.tm_year = strtoul(cur, NULL, 10) - 1900;

//#ifdef UTEST
	purple_debug_info(DBGID, "msg_time.tm_wday = %d\n", msg_time.tm_wday);
	purple_debug_info(DBGID, "msg_time.tm_mday = %d\n", msg_time.tm_mday);
	purple_debug_info(DBGID, "msg_time.tm_mon = %d\n", msg_time.tm_mon);
	purple_debug_info(DBGID, "msg_time.tm_year = %d\n", msg_time.tm_year);
	purple_debug_info(DBGID, "msg_time.tm_hour = %d\n", msg_time.tm_hour);
	purple_debug_info(DBGID, "msg_time.tm_min = %d\n", msg_time.tm_min);
	purple_debug_info(DBGID, "msg_time.tm_sec = %d\n", msg_time.tm_sec);
	purple_debug_info(DBGID, "cur_timezone = %d\n", cur_timezone);
	purple_debug_info(DBGID, "msg_time.tm_isdst = %d\n", msg_time.tm_isdst);
	purple_debug_info(DBGID, "finished\n");
//#endif

#ifndef __WIN32
	// Always return GMT time (not sure subtracting is right, but that's ultimately
	// irrelevant for twitter at least, twitter.com always returns +0000)
	retval = timegm(&msg_time) - cur_timezone;
#else
	// Seems that on Windows mktime always return the correct date/time according to timezone
	// Convert back to GMT first then convert to local time
//	retval = purple_time_build(msg_time.tm_year + 1900, msg_time.tm_mon, msg_time.tm_mday, msg_time.tm_hour, msg_time.tm_min, msg_time.tm_sec);
	retval = (mktime(&msg_time) - cur_timezone) + wpurple_get_tz_offset();
#endif
	purple_debug_info(DBGID, "final msg_time = %ld\n", retval);
	return retval;
}

const char * mb_get_uri_txt(PurpleAccount * pa)
{
	if (strcmp(pa->protocol_id, "prpl-mbpurple-twitter") == 0) {
		return "tw";
	} else if(strcmp(pa->protocol_id, "prpl-mbpurple-identica") == 0) {
		return "idc";
	}
	// no support for laconica for now
	return NULL;
}

void mb_account_set_ull(PurpleAccount * account, const char * name, unsigned long long value)
{
	gchar * tmp_str;

	tmp_str = g_strdup_printf("%llu", value);
	purple_account_set_string(account, name, tmp_str);
	g_free(tmp_str);
}

unsigned long long mb_account_get_ull(PurpleAccount * account, const char * name, unsigned long long default_value)
{
	const char * tmp_str;

	tmp_str = purple_account_get_string(account, name, NULL);
	if(tmp_str) {
		return strtoull(tmp_str, NULL, 10);
	} else {
		return default_value;
	}
}

static void mb_account_foreach_idhash(gpointer key, gpointer val, gpointer userdata)
{
	GString * output = userdata;
	gchar * str_key = key;

	if(output->len > 0) {
		g_string_append_printf(output, ",%s", str_key);
		purple_debug_info(DBGID, "appending idhash %s\n", str_key);
	} else {
		g_string_append(output, str_key);
		purple_debug_info(DBGID, "setting idhash %s\n", str_key);
	}
}

// work around to save a idhash to account
// Use , separater
void mb_account_set_idhash(PurpleAccount * account, const char * name, GHashTable * id_hash)
{
	GString * output = g_string_new("");

	g_hash_table_foreach(id_hash, mb_account_foreach_idhash, output);

	purple_debug_info(DBGID, "set_idhash output value = %s\n", output->str);
	// empty string will be set if sent_id_hash is empty
	purple_account_set_string(account, name, output->str);
	g_string_free(output, TRUE);
}

// work around to load a idhash from account
// Use , separater
void mb_account_get_idhash(PurpleAccount * account, const char * name, GHashTable * id_hash)
{
	const gchar * id_list;
	gchar * hash_val;
	gchar ** id_list_str, **tmp;

	id_list = purple_account_get_string(account, name, NULL);

	if(id_list && (strlen(id_list) > 0)) {
		purple_debug_info(DBGID, "got idlist = %s\n", id_list);
		id_list_str = g_strsplit(id_list, ",", 0);
		tmp = id_list_str;
		while( (*tmp) != NULL) {
			hash_val = g_strdup(*tmp);
			purple_debug_info(DBGID, "inserting value = %s\n", hash_val);
			g_hash_table_insert(id_hash, hash_val, hash_val);
			tmp++;
		}
		g_strfreev(id_list_str);
	}
}

gchar * mb_url_unparse(const char * host, int port, const char * path, const char * params, gboolean use_https) {
	gchar * proto = "http://";
	if(use_https) {
		proto = "https://";
	}
	if(port == 0) {
		return g_strdup_printf("%s%s%s%s%s", proto, host, path, params ? "?" : "", params ? params : "");
	} else {
		return g_strdup_printf("%s%s:%d%s%s%s", proto, host, port, path, params ? "?" : "", params ? params : "");
	}
}

#ifdef UTEST
int main(int argc, char * argv[])
{
	/*
	time_t msg_time;
	char * twitter_time = strdup("Wed Jul 23 10:59:53 +0000 2008");
	char * cur, * next, *tmp_cur, *tmp_next, oldval;
	int counter = 0,  tmp_counter = 0, i;
	
	printf("test time = %s\n", twitter_time);
	printf("current timezone offset = %ld\n", timezone);
	printf("current dst offset = %ld\n", daylight);
	msg_time = mb_mktime(twitter_time);
	printf("time = %ld\n", msg_time);
	free(twitter_time);
	
	//printf("Converted time = %s\n", asctime(&msg_time));

	 */
	printf("%s", mb_url_unparse("twitter.com", 0, "/oauth/authorize", "a=b&c=d", TRUE));
}

#endif
