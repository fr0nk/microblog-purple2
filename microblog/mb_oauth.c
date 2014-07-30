/*
    Copyright 2008-2010,  Somsak Sriprayoonsakul  <somsaks@gmail.com>

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
 * Implementation of OAuth support
 */

#include <stdlib.h>
#include <glib.h>
#include <math.h>

#include <debug.h>
#include <cipher.h>

#include "twitter.h"
#include "mb_net.h"
#include "mb_http.h"
#include "mb_util.h"

#include "mb_oauth.h"

#define DBGID "mboauth"

static const char fixed_headers[] = "User-Agent:" TW_AGENT "\r\n" \
"Accept: */*\r\n" \
"X-Twitter-Client: " TW_AGENT_SOURCE "\r\n" \
"X-Twitter-Client-Version: 0.1\r\n" \
"X-Twitter-Client-Url: " TW_AGENT_DESC_URL "\r\n" \
"Connection: Close\r\n" \
"Pragma: no-cache\r\n";


static MbConnData * mb_oauth_init_connection(MbAccount * ma, int type, const gchar * path, MbHandlerFunc handler, gchar ** full_url);
static gchar * mb_oauth_gen_nonce(void);
static gchar * mb_oauth_sign_hmac_sha1(const gchar * data, const gchar * key);
static gchar * mb_oauth_gen_sigbase(MbHttpData * data, const gchar * url, int type);
static gint mb_oauth_request_token_handler(MbConnData * conn_data, gpointer data, const char * error);

void mb_oauth_init(struct _MbAccount * ma, const gchar * c_key, const gchar * c_secret) {
	MbOauth * oauth = &ma->oauth;

	oauth->c_key = g_strdup(c_key);
	oauth->c_secret = g_strdup(c_secret);
	oauth->oauth_token = NULL;
	oauth->oauth_secret = NULL;
	oauth->pin = NULL;
	oauth->ma = ma;

	// XXX: Should we put this other places instead?
	srand(time(NULL));
}

void mb_oauth_set_token(struct _MbAccount * ma, const gchar * oauth_token, const gchar * oauth_secret) {
	MbOauth * oauth = &ma->oauth;

	if(oauth->oauth_token) g_free(oauth->oauth_token);
	oauth->oauth_token = g_strdup(oauth_token);

	if(oauth->oauth_secret) g_free(oauth->oauth_secret);
	oauth->oauth_secret = g_strdup(oauth_secret);
}

void mb_oauth_set_pin(struct _MbAccount * ma, const gchar * pin) {

	gchar * tmp, *new;

	if(ma->oauth.pin) g_free(ma->oauth.pin);

	if(!pin) {
		return;
	}

	tmp = g_strdup(pin);
	new = g_strstrip(tmp);
	ma->oauth.pin = g_strdup(new);
	g_free(tmp);
}

void mb_oauth_free(struct _MbAccount * ma)
{
	MbOauth * oauth = &ma->oauth;
	if(oauth->c_key) g_free(oauth->c_key);
	if(oauth->c_secret) g_free(oauth->c_secret);

	if(oauth->oauth_token) g_free(oauth->oauth_token);
	if(oauth->oauth_secret) g_free(oauth->oauth_secret);

	if(oauth->pin) g_free(oauth->pin);

	oauth->c_key = NULL;
	oauth->c_secret = NULL;
	oauth->oauth_token = NULL;
	oauth->oauth_secret = NULL;
}

/**
 * Initialize conn_data structure
 *
 * @param ma MbAccount in action
 * @param type type (HTTP_GET or HTTP_POST)
 * @param path path in URL
 * @param handler callback when connection established
 * @param full_url if not NULL, create the full url in this variable (need to be freed)
 */
static MbConnData * mb_oauth_init_connection(MbAccount * ma, int type, const gchar * path, MbHandlerFunc handler, gchar ** full_url)
{
	MbConnData * conn_data = NULL;
	gboolean use_https = purple_account_get_bool(ma->account, mc_name(TC_USE_HTTPS), mc_def_bool(TC_USE_HTTPS));
	gint retry = purple_account_get_int(ma->account, mc_name(TC_GLOBAL_RETRY), mc_def_int(TC_GLOBAL_RETRY));
	gint port;
	gchar * user = NULL, * host = NULL;

	if(use_https) {
		port = TW_HTTPS_PORT;
	} else {
		port = TW_HTTP_PORT;
	}

	twitter_get_user_host(ma, &user, &host);

	if(full_url) {
		(*full_url) = mb_url_unparse(host, 0, path, NULL, use_https);
	}

	conn_data = mb_conn_data_new(ma, host, port, handler, use_https);
	mb_conn_data_set_retry(conn_data, retry);

	conn_data->request->type = type;
	if(type == HTTP_POST) {
		mb_http_data_set_content_type(conn_data->request, "application/x-www-form-urlencoded");
	}
	conn_data->request->port = port;

	mb_http_data_set_host(conn_data->request, host);
	mb_http_data_set_path(conn_data->request, path);

	// XXX: Use global here -> twitter_fixed_headers
	mb_http_data_set_fixed_headers(conn_data->request, fixed_headers);
	mb_http_data_set_header(conn_data->request, "Host", host);

	if(user) g_free(user);
	if(host) g_free(host);

	return conn_data;

}

/**
 * generate a random string between 15 and 32 chars length
 * and return a pointer to it. The value needs to be freed by the
 * caller
 *
 * @return zero terminated random string.
 * @note this function was taken from by liboauth
 */
static gchar * mb_oauth_gen_nonce(void) {
	char *nc;
	const char *chars = "abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ" "0123456789_";
	unsigned int max = strlen( chars );
	int i, len;

	len = 15 + floor(rand()*16.0/(double)RAND_MAX);
	nc = g_malloc(len+1);
	for(i=0;i<len; i++) {
		nc[i] = chars[ rand() % max ];
	}
	nc[i]='\0';
	return (nc);
//	return g_strdup("F_2urGzJ0q8Alzgbllio");
}

/**
 * Generate signature from data and key
 *
 * @param data to generate signature from
 * @param key key (secret)
 * @return string represent signature (must be freed by caller)
 */
static gchar * mb_oauth_sign_hmac_sha1(const gchar * data, const gchar * key) {
	PurpleCipherContext * context = NULL;
	size_t out_len;
	guchar digest[128];
	gchar * retval = NULL;

	purple_debug_info(DBGID, "signing data \"%s\"\n with key \"%s\"\n", data, key);
	if( (context = purple_cipher_context_new_by_name("hmac", NULL)) == NULL) {
		purple_debug_info(DBGID, "couldn't find HMAC cipher, upgrade Pidgin?\n");
		return NULL;
	}
	purple_cipher_context_set_option(context, "hash", "sha1");
	purple_cipher_context_set_key(context, (guchar *)key);
	purple_cipher_context_append(context, (guchar *)data, strlen(data));

	if(purple_cipher_context_digest(context, sizeof(digest), digest, &out_len)) {
		//purple_debug_info(DBGID, "got digest = %s, out_len = %d\n", digest, (int)out_len);
		retval = purple_base64_encode(digest, out_len);
		purple_debug_info(DBGID, "got digest = %s, out_len = %d\n", retval, (int)out_len);
	} else {
		purple_debug_info(DBGID, "couldn't digest signature\n");
	}

	purple_cipher_context_destroy(context);

	return retval;
}

/**
 * Generate signature base text
 *
 * @param ma MbAccount in action
 * @param url path part of URL
 * @param type HTTP_GET or HTTP_POST
 * @param params list of PurpleKeyValuePair
 */
static gchar * mb_oauth_gen_sigbase(MbHttpData * data, const gchar * url, int type) {
	gchar * type_str = NULL, * param_str = NULL, * retval = NULL, *encoded_url, *encoded_param;

	// Type
	if(type == HTTP_GET) {
		type_str = "GET";
	} else {
		type_str = "POST";
	}

	// Concatenate all parameter
	param_str = g_malloc(data->params_len + 1);
	mb_http_data_encode_param(data, param_str, data->params_len, TRUE);
    purple_debug_info(DBGID, "final merged param string = %s\n", param_str);

    encoded_url = g_strdup(purple_url_encode(url));
    encoded_param = g_strdup(purple_url_encode(param_str));
    g_free(param_str);

    retval = g_strdup_printf("%s&%s&%s", type_str, encoded_url, encoded_param);
    g_free(encoded_url);
    g_free(encoded_param);

    return retval;
}

static void _do_oauth(struct _MbAccount * ma, const gchar * path, int type, MbOauthResponse func, gpointer data, MbHandlerFunc handler) {
	MbConnData * conn_data = NULL;
	gchar * full_url = NULL;

	conn_data = mb_oauth_init_connection(ma, type, path, handler, &full_url);

	mb_oauth_set_http_data(&ma->oauth, conn_data->request, full_url, type);

	// Set the call back function
	ma->oauth.response_func = func;

	// Initiate connection
	conn_data->handler_data = ma;
	mb_conn_process_request(conn_data);
}

static gint mb_oauth_request_token_handler(MbConnData * conn_data, gpointer data, const char * error) {
	MbAccount * ma = (MbAccount *)data;
	GList * it;
	MbHttpParam * param = NULL;
	gint retval = 0;

	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	if(error) {
		purple_debug_info(DBGID, "got error: %s\n", error);
		return -1;
	}

	purple_debug_info(DBGID, "got response %s\n", conn_data->response->content->str);

	if(conn_data->response->status == HTTP_OK) {
		purple_debug_info(DBGID, "going to decode the received message\n");
		// Decode the parameter first
		mb_http_data_decode_param_from_content(conn_data->response);
		purple_debug_info(DBGID, "message decoded\n");

		// fill-in all necessary parameter
		if(ma->oauth.oauth_token != NULL) {
			g_free(ma->oauth.oauth_token);
		}
		if(ma->oauth.oauth_secret != NULL) {
			g_free(ma->oauth.oauth_secret);
		}
		ma->oauth.oauth_token = ma->oauth.oauth_secret = NULL;
		for(it = g_list_first(conn_data->response->params); it; it = g_list_next(it)) {
			param = (MbHttpParam *)it->data;
			if(strcmp(param->key, "oauth_token") == 0) {
				ma->oauth.oauth_token = g_strdup(param->value);
			} else if(strcmp(param->key, "oauth_token_secret") == 0) {
				ma->oauth.oauth_secret = g_strdup(param->value);
			}
			if(ma->oauth.oauth_token && ma->oauth.oauth_secret) {
				break;
			}
		}
	} // no else, leave the error detection to callback function below

	// now call the user-defined function to get PIN or whatever we need to request for access token
	// Let the func do the job, since the caller should know the URL
	if(ma && ma->oauth.response_func) {
		retval = ma->oauth.response_func(ma, conn_data, data);
	}
	purple_debug_info(DBGID, "return value = %d\n", retval);
	return retval;
}

void mb_oauth_request_token(struct _MbAccount * ma, const gchar * path, int type, MbOauthResponse func, gpointer data) {
	if(ma->oauth.oauth_token) g_free(ma->oauth.oauth_token);
	if(ma->oauth.oauth_secret) g_free(ma->oauth.oauth_secret);
	ma->oauth.oauth_token = ma->oauth.oauth_secret = NULL;
	_do_oauth(ma, path, type, func, data, mb_oauth_request_token_handler);
}

void mb_oauth_request_access(struct _MbAccount * ma, const gchar * path, int type, MbOauthResponse func, gpointer data) {
	_do_oauth(ma, path, type, func, data, mb_oauth_request_token_handler);
}
//
void mb_oauth_set_http_data(MbOauth * oauth, struct _MbHttpData * http_data, const gchar * full_url, int type) {
	gchar * nonce = NULL, * sig_base = NULL, * secret = NULL, * signature = NULL;

	// Attach OAuth data
	mb_http_data_add_param(http_data, "oauth_consumer_key", oauth->c_key);

	nonce = mb_oauth_gen_nonce();
	mb_http_data_add_param(http_data, "oauth_nonce", nonce);
	g_free(nonce);

	mb_http_data_add_param(http_data, "oauth_signature_method", "HMAC-SHA1");
	mb_http_data_add_param_ull(http_data, "oauth_timestamp", time(NULL));
	mb_http_data_add_param(http_data, "oauth_version", "1.0");

	if(oauth->oauth_token && oauth->oauth_secret) {
		mb_http_data_add_param(http_data, "oauth_token", oauth->oauth_token);
	}

	if(oauth->pin) {
		mb_http_data_add_param(http_data, "oauth_verifier", oauth->pin);
	}

	mb_http_data_sort_param(http_data);

	// Create signature
	sig_base = mb_oauth_gen_sigbase(http_data, full_url, type);
	purple_debug_info(DBGID, "got signature base = %s\n", sig_base);

	secret = g_strdup_printf("%s&%s", oauth->c_secret, oauth->oauth_secret ? oauth->oauth_secret : "");

	signature = mb_oauth_sign_hmac_sha1(sig_base, secret);
	g_free(secret);
	g_free(sig_base);
	purple_debug_info(DBGID, "signed signature = %s\n", signature);

	// Attach to parameter
	mb_http_data_add_param(http_data, "oauth_signature", signature);
	g_free(signature);

}

void mb_oauth_reset_nonce(MbOauth * oauth, struct _MbHttpData * http_data, const gchar * full_url, int type) {
	gchar * nonce, * secret, * sig_base, * signature;

	mb_http_data_rm_param(http_data, "oauth_nonce");
	mb_http_data_rm_param(http_data, "oauth_signature");

	nonce = mb_oauth_gen_nonce();
	mb_http_data_add_param(http_data, "oauth_nonce", nonce);
	g_free(nonce);

	// Re-Create signature
	sig_base = mb_oauth_gen_sigbase(http_data, full_url, type);
	purple_debug_info(DBGID, "got signature base = %s\n", sig_base);

	secret = g_strdup_printf("%s&%s", oauth->c_secret, oauth->oauth_secret ? oauth->oauth_secret : "");

	signature = mb_oauth_sign_hmac_sha1(sig_base, secret);
	g_free(secret);
	g_free(sig_base);
	purple_debug_info(DBGID, "signed signature = %s\n", signature);

	// Attach to parameter
	mb_http_data_add_param(http_data, "oauth_signature", signature);
	g_free(signature);
}
