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
*/
/*
 * mb_oauth.h
 *
 *  Created on: May 16, 2010
 *      Author: somsak
 */

#ifndef MB_OAUTH_H_
#define MB_OAUTH_H_

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


struct _MbAccount;
struct _MbConnData;
struct _MbHttpData;

typedef gint (* MbOauthResponse)(struct _MbAccount * ma, struct _MbConnData * data, gpointer user_data);

typedef struct _MbOauth {
	gchar * c_key; //< Consumer key
	gchar * c_secret; //< Consumer secret
	gchar * oauth_token; //< request token
	gchar * oauth_secret; //< request secret
	gchar * pin; //< user input PIN
	MbOauthResponse response_func;
	struct _MbAccount * ma;
	gpointer data;
} MbOauth;

void mb_oauth_init(struct _MbAccount * ma, const gchar * c_key, const gchar * c_secret);
void mb_oauth_set_token(struct _MbAccount * ma, const gchar * oauth_token, const gchar * oauth_secret);
void mb_oauth_set_pin(struct _MbAccount * ma, const gchar * pin);
void mb_oauth_request_token(struct _MbAccount * ma, const gchar * path, int type, MbOauthResponse func, gpointer data);
void mb_oauth_request_access(struct _MbAccount * ma, const gchar * path, int type, MbOauthResponse func, gpointer data);
void mb_oauth_free(struct _MbAccount * ma);
//
void mb_oauth_set_http_data(MbOauth * oauth, struct _MbHttpData * http_data, const gchar * full_url, int type);
void mb_oauth_reset_nonce(MbOauth * oauth, struct _MbHttpData * http_data, const gchar * full_url, int type);

#ifdef __cplusplus
}
#endif

#endif /* MB_OAUTH_H_ */
