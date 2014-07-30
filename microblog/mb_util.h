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
/**
 *  Utility for microblog plug-in
 */
#ifndef __MB_UTIL__
#define __MB_UTIL__

#ifdef __cplusplus
extern "C" {
#endif

#include "account.h"

extern const char * mb_get_uri_txt(PurpleAccount * pa);
extern time_t mb_mktime(char * time_str);
extern void mb_account_set_ull(PurpleAccount * account, const char * name, unsigned long long value);
extern unsigned long long mb_account_get_ull(PurpleAccount * account, const char * name, unsigned long long default_value);
extern void mb_account_set_idhash(PurpleAccount * account, const char * name, GHashTable * id_hash);
extern void mb_account_get_idhash(PurpleAccount * account, const char * name, GHashTable * id_hash);
extern gchar * mb_url_unparse(const char * host, int port, const char * path, const char * params, gboolean use_https);

#ifdef __cplusplus
}
#endif

#endif
