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
* HTTP data implementation
*/

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>

#include <purple.h>

#ifdef _WIN32
#	include <win32dep.h>
#	define MAXHOSTNAMELEN 64
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#include "mb_http.h"

// function below might be static instead
static MbHttpParam * mb_http_param_new(void)
{
	MbHttpParam * p = g_new(MbHttpParam, 1);

	p->key = NULL;
	p->value = NULL;
	return p;
}
static void mb_http_param_free(MbHttpParam * param)
{
	if(param->key) g_free(param->key);
	if(param->value) g_free(param->value);
	g_free(param);
}

static guint mb_strnocase_hash(gconstpointer a)
{
	gint len = strlen(a);
	gint i;
	gchar * tmp = g_strdup(a);
	guint retval;

	for(i = 0; i < len; i++) {
		tmp[i] = tolower(tmp[i]);
	}
	retval = g_str_hash(tmp);
	g_free(tmp);
	return retval;
}

static gboolean mb_strnocase_equal(gconstpointer a, gconstpointer b)
{
	if(strcasecmp((const gchar *)a, (const gchar *)b) == 0) {
		return TRUE;
	}
	return FALSE;
}

MbHttpData * mb_http_data_new(void)
{
	MbHttpData * data = g_new(MbHttpData, 1);

	// URL part, default to HTTP with port 80
	data->host = NULL;
	data->path = NULL;
	data->proto = MB_HTTP;
	data->port = 80;

	data->headers = g_hash_table_new_full(mb_strnocase_hash, mb_strnocase_equal, g_free, g_free);
	data->headers_len = 0;
	data->fixed_headers = NULL;
	data->params = NULL;
	data->params_len = 0;

	data->content_type = NULL;
	data->content = NULL;
	data->chunked_content = NULL;
	data->content_len = 0;

	data->status = -1;
	data->type = HTTP_GET; //< default is get
	data->state = MB_HTTP_STATE_INIT;

	data->packet = NULL;
	data->cur_packet = NULL;

	return data;
}
void mb_http_data_free(MbHttpData * data) {
	purple_debug_info(MB_HTTPID, "freeing http data\n");

	if(data->host) {
		purple_debug_info(MB_HTTPID, "freeing host\n");
		g_free(data->host);
	}
	if(data->path) {
		purple_debug_info(MB_HTTPID, "freeing path\n");
		g_free(data->path);
	}

	if(data->headers) {
		purple_debug_info(MB_HTTPID, "freeing header hash table\n");
		g_hash_table_destroy(data->headers);
	}
	if(data->fixed_headers) {
		purple_debug_info(MB_HTTPID, "freeing fixed headers\n");
		g_free(data->fixed_headers);
	}
	data->headers_len = 0;

	if(data->params) {
		purple_debug_info(MB_HTTPID, "freeing each parameter\n");
		GList * it;
		MbHttpParam * p;

		for(it = g_list_first(data->params); it; it = g_list_next(it)) {
			p = it->data;
			purple_debug_info(MB_HTTPID, "freeing parameter, %s=%s\n", p->key, p->value);
			mb_http_param_free(p);
		}
		purple_debug_info(MB_HTTPID, "freeing all params\n");
		g_list_free(data->params);
	}

	if(data->content_type) {
		g_free(data->content_type);
	}
	if(data->content) {
		purple_debug_info(MB_HTTPID, "freeing request\n");
		g_string_free(data->content, TRUE);
	}
	if(data->chunked_content) {
		purple_debug_info(MB_HTTPID, "freeing chunked request\n");
		g_string_free(data->chunked_content, TRUE);
	}

	if(data->packet) {
		purple_debug_info(MB_HTTPID, "freeing packet\n");
		g_free(data->packet);
	}
	purple_debug_info(MB_HTTPID, "freeing self\n");
	g_free(data);
}

/*
	Always remove entry in hash
  */
static gboolean hash_remover_always(gpointer key, gpointer value, gpointer data)
{
	return TRUE;
}

void mb_http_data_truncate(MbHttpData * data)
{
	data->headers_len = 0;
	data->params_len = 0;
	data->content_len = 0;
	data->status = -1;
	data->state = MB_HTTP_STATE_INIT;

	if(data->headers) {
		//g_hash_table_destroy(data->headers);
		//data->headers = g_hash_table_new_full(mb_strnocase_hash, mb_strnocase_equal, g_free, g_free);
		g_hash_table_foreach_remove(data->headers, hash_remover_always, NULL);
	}
	if(data->fixed_headers) {
		g_free(data->fixed_headers);
		data->fixed_headers = NULL;
	}
	if(data->params) {
		GList * it;
		MbHttpParam * p;

		for(it = g_list_first(data->params); it; it = g_list_next(it)) {
			p = it->data;
			mb_http_param_free(p);
		}
		g_list_free(data->params);
		data->params = NULL;
	}

	if(data->content_type) {
		g_free(data->content_type);
		data->content_type = NULL;
	}
	if(data->content) {
		g_string_free(data->content, TRUE);
		data->content = NULL;
	}
	if(data->packet) {
		g_free(data->packet);
		data->packet = NULL;
		data->cur_packet = NULL;
	}
}

void mb_http_data_set_url(MbHttpData * data, const gchar * url)
{
	gchar * tmp_url = g_strdup(url);
	gchar * cur_it = NULL, *tmp_it = NULL, *host_and_port = NULL;

	cur_it = tmp_url;
	for(;;) {
		// looking for ://
		if( (tmp_it = strstr(cur_it, "://")) == NULL) {
			break;
		}
		(*tmp_it) = '\0';
		if(strcmp(cur_it, "http") == 0) {
			data->proto = MB_HTTP;
		} else if(strcmp(cur_it, "https") == 0) {
			data->proto = MB_HTTPS;
		} else {
			data->proto = MB_PROTO_UNKNOWN;
		}
		cur_it = tmp_it + 3;

		//now looking for host part
		if( (tmp_it = strchr(cur_it, '/')) == NULL) {
			break;
		}
		(*tmp_it) = '\0';
		host_and_port = cur_it;
		cur_it = tmp_it; //< save cur_it for later use
		if( (tmp_it = g_strrstr(host_and_port, ":")) == NULL) {
			// host without port
			if(data->host) g_free(data->host);
			data->host = g_strdup(host_and_port);
			switch(data->proto) {
				case MB_HTTP :
					data->port = 80;
					break;
				case MB_HTTPS :
					data->port = 443;
					break;
				default :
					data->port = 80; //< all other protocol assume that it's http, caller should specified port
					break;
			}
		} else {
			(*tmp_it) = '\0';
			if(data->host) g_free(data->host);
			data->host = g_strdup(host_and_port);
			data->port = (gint)strtoul(tmp_it + 1, NULL, 10);
		}

		// now the path
		(*cur_it) = '/';
		if(data->path) g_free(data->path);
		data->path = g_strdup(cur_it);

		break;
	}

	g_free(tmp_url);
}

void mb_http_data_get_url(MbHttpData * data, gchar * url, gint url_len)
{
	gchar proto_str[10];
	if(data->proto == MB_HTTP) {
		strcpy(proto_str, "http");
	} else if(data->proto == MB_HTTPS) {
		strcpy(proto_str, "https");
	} else {
		strcpy(proto_str, "unknown");
	}
	snprintf(url, url_len, "%s://%s:%d%s", proto_str, data->host, data->port, data->path);
}

void mb_http_data_set_path(MbHttpData * data, const gchar * path)
{
	if(data->path) {
		g_free(data->path);
	}
	data->path = g_strdup(path);
}

void mb_http_data_set_host(MbHttpData * data, const gchar * host)
{
	if(data->host) {
		g_free(data->host);
	}
	data->host = g_strdup(host);
}

void mb_http_data_set_content_type(MbHttpData * data, const gchar * type) {
	if(data->content_type) g_free(data->content_type);
	data->content_type = g_strdup(type);
}

void mb_http_data_set_content(MbHttpData * data, const gchar * content, gssize len)
{
	if(data->content) {
		g_string_truncate(data->content, 0);
	} else {
		data->content = g_string_new_len(content, len);
	}
}

void mb_http_data_set_header(MbHttpData* data, const gchar * key, const gchar * value)
{
	gint len = strlen(key) + strlen(value) + 10; //< pad ':' and null terminate string and \r\n

	g_hash_table_insert(data->headers, g_strdup(key), g_strdup(value));
	data->headers_len += len;
}

gchar * mb_http_data_get_header(MbHttpData * data, const gchar * key)
{
	return (gchar *)g_hash_table_lookup(data->headers, key);
}

void mb_http_data_set_fixed_headers(MbHttpData * data, const gchar * headers)
{
	if(data->fixed_headers) {
		g_free(data->fixed_headers);
	}
	data->fixed_headers = g_strdup(headers);
	data->headers_len += strlen(data->fixed_headers);
}

static gint mb_http_data_param_key_pred(gconstpointer a, gconstpointer key)
{
	const MbHttpParam * p = (const MbHttpParam *)a;

	if(strcmp(p->key, (gchar *)key) == 0) {
		return 0;
	}
	return -1;
}

void mb_http_data_add_param(MbHttpData * data, const gchar * key, const gchar * value)
{
	MbHttpParam * p = mb_http_param_new();

	purple_debug_info(MB_HTTPID, "adding parameter %s = %s\n", key, value);
	//p->key = g_strdup(purple_url_encode(key));
	p->key = g_strdup(key);
	//p->value = g_strdup(purple_url_encode(value));
	p->value = g_strdup(value);
	data->params = g_list_append(data->params, p);
	data->params_len += (5*strlen(p->key) + 5*strlen(p->value) + 5); //< length of key + value + "& or ?", x3 in case all character must be url_encode
}

void mb_http_data_add_param_int(MbHttpData * data, const gchar * key, gint value)
{
	char tmp[100];

	snprintf(tmp, sizeof(tmp), "%d", value);
	mb_http_data_add_param(data, key, tmp);
}

void mb_http_data_add_param_ull(MbHttpData * data, const gchar * key, unsigned long long value)
{
	char tmp[200];

	// Use g_snprintf for maximum compatibility
	g_snprintf(tmp, sizeof(tmp), "%llu", value);
	mb_http_data_add_param(data, key, tmp);
}

const gchar * mb_http_data_find_param(MbHttpData * data, const gchar * key)
{
	GList * retval;
	MbHttpParam * p;

	retval = g_list_find_custom(data->params, key, mb_http_data_param_key_pred);

	if(retval) {
		p = retval->data;
		return p->value;
	} else {
		return NULL;
	}
}

gboolean mb_http_data_rm_param(MbHttpData * data, const gchar * key)
{
	MbHttpParam * p;
	GList *it = NULL;
	gboolean retval = FALSE;

	purple_debug_info(MB_HTTPID, "%s called, key = %s\n", __FUNCTION__, key);
	it = g_list_first(data->params);
	while(it) {
		p = it->data;
		if(strcmp(p->key, key) == 0) {
			p = it->data;
			data->params_len -= (5*strlen(p->key) + 5*strlen(p->value) - 5);
			mb_http_param_free(p);
			data->params = g_list_delete_link(data->params, it);
			it = g_list_first(data->params);
			retval = TRUE;
		} else {
			it = g_list_next(it);
		}
	}
	return retval;
}

/*
// generic utility
extern gbool mb_http_data_ok(MbHttpData * data);
*/

static void mb_http_data_header_assemble(gpointer key, gpointer value, gpointer udata)
{
	MbHttpData * data = udata;
	gint len;

	len = sprintf(data->cur_packet, "%s: %s\r\n", (char *)key, (char *)value);
	data->cur_packet += len;
}

static int _string_compare_key(gconstpointer a, gconstpointer b) {
	const MbHttpParam * param_a = (MbHttpParam *)a;
	const MbHttpParam * param_b = (MbHttpParam *)b;

	return strcmp(param_a->key, param_b->key);
}

void mb_http_data_sort_param(MbHttpData * data) {
	data->params = g_list_sort(data->params, _string_compare_key);
}

int mb_http_data_encode_param(MbHttpData *data, char * buf, int len, gboolean url_encode)
{
	GList * it;
	MbHttpParam * p;
	int cur_len = 0, ret_len = 0;
	char * cur_buf = buf;

	purple_debug_info(MB_HTTPID, "%s called, len = %d\n", __FUNCTION__, len);
	if(data->params) {
		gchar * encoded_val = NULL;
		for(it = g_list_first(data->params); it; it = g_list_next(it)) {
			p = it->data;
			purple_debug_info(MB_HTTPID, "%s: key = %s, value = %s\n", __FUNCTION__, p->key, p->value);
			// Only encode value here, so _ in key will not be translated
			if(url_encode) {
				encoded_val = g_strdup(purple_url_encode(p->value));
			} else {
				encoded_val = g_strdup(p->value);
			}
			ret_len = snprintf(cur_buf, len - cur_len, "%s=%s&", p->key, encoded_val);
			g_free(encoded_val);
			purple_debug_info(MB_HTTPID, "len = %d, cur_len = %d, cur_buf = ##%s##\n", len, cur_len, cur_buf);
			cur_len += ret_len;
			if(cur_len >= len) {
				purple_debug_info(MB_HTTPID, "len is too small, len = %d, cur_len = %d\n", len, cur_len);
				return cur_len;
			}
			cur_buf += ret_len;
		}
		cur_buf--;
		(*cur_buf) = '\0';
	}
	purple_debug_info(MB_HTTPID, "final param is %s\n", buf);
	return (cur_len - 1);
}

void mb_http_data_decode_param_from_content(MbHttpData *data) {
	GString * content = NULL;
	gchar * cur = NULL, * start = NULL, * amp = NULL, * equal = NULL;
	gchar * key, * val;

	if(data->content_len > 0) {
		content = data->content;
		start = cur = content->str;
		while( (cur - content->str) < data->content_len) {
			// Look for &
			if( (*cur) == '&') {
				amp = cur;
				(*amp) = '\0';
				if(equal) {
					(*equal) = '\0';
					key = start;
					val = (equal + 1);
					// treat every parameter as string
					mb_http_data_add_param(data, key, val);
					(*equal) = '=';
				}
				(*amp) = '&';
				start = amp + 1;
			} else if ( (*cur) == '=') {
				equal = cur;
			}
			cur++;
		}
	}

}

void mb_http_data_prepare_write(MbHttpData * data)
{
	gchar * cur_packet, * param_content = NULL, *url = NULL;
	gint packet_len, url_len, len;

	if(data->path == NULL) return;

	// assemble all headers
	// I don't sure how hash table will behave, so assemple everything should be better
	packet_len = data->headers_len + data->params_len + MAXHOSTNAMELEN + 10 + strlen(data->path) + 100; //< for \r\n\r\n and GET|POST and other stuff
	if(data->content) {
		packet_len += data->content->len;
	}
	if(data->packet) g_free(data->packet);
	data->packet = g_malloc0(packet_len + 1);
	cur_packet = data->packet;

        // GET and POST must use full URL when using proxies
        // length calculated as length of "unknown" + 3 digits for "://" + 5
        // digits for port + length of data->path + a few bytes extra
        url_len = sizeof(gchar) * (MAXHOSTNAMELEN + 20 + strlen(data->path));
        url = (gchar *) g_malloc0(url_len + 1);
        mb_http_data_get_url(data, url, url_len);

	// GET|POST and parameter part
	if(data->type == HTTP_GET) {
		len = sprintf(cur_packet, "GET %s", url);
	} else {
		len = sprintf(cur_packet, "POST %s", url);
	}
        g_free((gpointer) url);

	//printf("cur_packet = %s\n", cur_packet);
	cur_packet += len;

	// parameter
	if(data->params) {
		if(data->content_type && data->type == HTTP_POST && (strcmp(data->content_type, "application/x-www-form-urlencoded") == 0)) {
			// Special case
			// put all parameters in content in this case
			param_content = g_malloc0(data->params_len + 1);
			data->content_len = mb_http_data_encode_param(data, param_content, data->params_len, TRUE);
			// XXX: in this case, abandon what content was
			g_string_free(data->content, TRUE);
			data->content = g_string_new(param_content);
			g_free(param_content);
		} else {
			// put all parameters after path
			(*cur_packet) = '?';
			cur_packet++;
			len = mb_http_data_encode_param(data, cur_packet,  packet_len - (cur_packet - data->packet), TRUE);
			cur_packet += len;
		}
	}

	// Trailing "HTTP/1.1\r\n"
	(*cur_packet) = ' ';
	len = sprintf(cur_packet, " HTTP/1.1\r\n");
	cur_packet += len;

	// headers part
	data->cur_packet = cur_packet;
	g_hash_table_foreach(data->headers, mb_http_data_header_assemble, data);

	// Content type (which is actually another header)
	if(data->content_type) {
		len = sprintf(data->cur_packet, "Content-Type: %s\r\n", (char *)data->content_type);
		data->cur_packet += len;
	}

	// Fixed headers
	cur_packet = data->cur_packet;
	if(data->fixed_headers) {
		strcpy(cur_packet, data->fixed_headers);
		cur_packet += strlen(data->fixed_headers);
	}

	// content-length, if needed
	if(data->content) {
		len = sprintf(cur_packet, "Content-Length: %d\r\n", (int)data->content->len);
		cur_packet += len;
	}

	// end header part
	len = sprintf(cur_packet, "\r\n");
	cur_packet += len;

	// Content part
	if(data->content) {
	/*
		len = sprintf(cur_packet, "%s", data->content->str);
		cur_packet += len;
	*/
		memcpy(cur_packet, data->content->str, data->content->len);
		cur_packet += data->content->len;
	}

	data->packet_len = cur_packet - data->packet;

	// reset back to head of packet, ready to transfer
	data->cur_packet = data->packet;

	purple_debug_info(MB_HTTPID, "prepared packet = %s\n", data->packet);
}

void mb_http_data_post_read(MbHttpData * data, const gchar * buf, gint buf_len)
{
	gint packet_len = (buf_len > MB_MAXBUFF) ? buf_len : MB_MAXBUFF;
	gint cur_pos_len, whole_len;
	gchar * delim, * cur_pos, *content_start = NULL;
	gchar * key, *value, *key_value_sep;
	gboolean continue_to_next_state = FALSE;

	if(buf_len <= 0) return;
	switch(data->state) {
		case MB_HTTP_STATE_INIT :
			if(data->packet) {
				g_free(data->packet);
			}
			data->packet = g_malloc0(packet_len);
			data->packet_len = packet_len;
			data->cur_packet = data->packet;
			data->state = MB_HTTP_STATE_HEADER;
			//break; //< purposely move to next step
		case MB_HTTP_STATE_HEADER :
			//printf("processing header\n");
			// at this state, no data at all, so this should be the very first chunk of data
			// reallocate buffer if we don't have enough
			cur_pos_len = data->cur_packet - data->packet;
			if( (data->packet_len - cur_pos_len) < buf_len) {
				//printf("reallocate buffer\n");
				data->packet_len += (buf_len * 2);
				data->packet = g_realloc(data->packet, data->packet_len);
				data->cur_packet = data->packet + cur_pos_len;
			}
			memcpy(data->cur_packet, buf, buf_len);
			whole_len = (data->cur_packet - data->packet) + buf_len;

			// decipher header
			cur_pos = data->packet;
			//printf("initial_cur_pos = #%s#\n", cur_pos);
			while( (delim = strstr(cur_pos, "\r\n")) != NULL) {
				if( strncmp(delim, "\r\n\r\n", 4) == 0 ) {
					// we reach the content now
					content_start = delim + 4;
					//printf("found content = %s\n", content_start);
				}
				(*delim) = '\0';
				//printf("cur_pos = %s\n", cur_pos);
				if(strncmp(cur_pos, "HTTP/1.0", 7) == 0) {
					// first line
					data->status = (gint)strtoul(&cur_pos[9], NULL, 10);
					//printf("data->status = %d\n", data->status);
				} else {
					//Header line
					if( (key_value_sep = strchr(cur_pos, ':')) != NULL) {
						//printf("header line\n");
						(*key_value_sep) = '\0';
						key = cur_pos;
						value = key_value_sep + 1;
						key = g_strstrip(key);
						value = g_strstrip(value);

						if(strcasecmp(key, "Content-Length") == 0) {
							data->content_len = (gint)strtoul(value, NULL, 10);
						} else if (strcasecmp(key, "Transfer-Encoding") == 0) {
							// Actually I should check for the value
							// AFAIK, Transfer-Encoding only valid value is chunked
							// Anyways, this is for identi.ca
							purple_debug_info(MB_HTTPID, "chunked data transfer\n");
							if(data->chunked_content) {
								g_string_free(data->chunked_content, TRUE);
							}
							data->chunked_content = g_string_new(NULL);
							// Need to goes into CONTENT state to decode the first chunk
						}
						mb_http_data_set_header(data, key, value);
					} else {
						// invalid header?
						// do nothing for now
						purple_debug_info(MB_HTTPID, "an invalid line? line = #%s#", cur_pos);
					}
				}
				if(content_start) {
					break;
				}
				cur_pos = delim + 2;
			}
			if(content_start) {
				// copy the rest of data as content and free everything
				if(data->content != NULL) {
					g_string_free(data->content, TRUE);
				}
				if(data->chunked_content) {
					data->chunked_content = g_string_new_len(content_start, whole_len - (content_start - data->packet));
					data->content = g_string_new(NULL);
					continue_to_next_state = TRUE;
					purple_debug_info(MB_HTTPID, "we'll continue to next state (STATE_CONTENT)\n");
				} else {
					data->content = g_string_new_len(content_start, whole_len - (content_start - data->packet));
				}
				g_free(data->packet);
				data->cur_packet = data->packet = NULL;
				data->packet_len = 0;
				data->state = MB_HTTP_STATE_CONTENT;
			} else {
				// copy the rest of string to the beginning
				if( (cur_pos - data->packet) < whole_len) {
					gint tmp_len = whole_len - (cur_pos - data->packet);
					gchar * tmp = g_malloc(tmp_len);

					memcpy(tmp, cur_pos, tmp_len);
					memcpy(data->packet, tmp, tmp_len);
					g_free(tmp);

					data->cur_packet = data->packet + tmp_len;
				}
			}
			if(!continue_to_next_state) {
				break;
			}
		case MB_HTTP_STATE_CONTENT :
			if(data->chunked_content) {
				if(!continue_to_next_state) { //< already have buffer from previous state
					// Buffer is not already here
					g_string_append_len(data->chunked_content, buf, buf_len);
				}
				// decode the chunked content and put it in content
				for(;;) {
					purple_debug_info(MB_HTTPID, "current data in chunked_content = #%s#\n", data->chunked_content->str);
					cur_pos = strstr(data->chunked_content->str, "\r\n");
					if(!cur_pos) {
						// Content will only be what can be decoded
						purple_debug_info(MB_HTTPID, "can't find any CRLF\n");
						break;
					}
					if(cur_pos == data->chunked_content->str) {
						g_string_erase(data->chunked_content, 0, 2);
						continue;
					}
					(*cur_pos) = '\0';
					cur_pos_len = strtoul(data->chunked_content->str, NULL, 16);
					purple_debug_info(MB_HTTPID, "chunk length = %d, %x\n", cur_pos_len, cur_pos_len);
					(*cur_pos) = '\r';
					if(cur_pos_len == 0) {
						// we got everything
						purple_debug_info(MB_HTTPID, "got 0 size chunk, end of message\n");
						data->state = MB_HTTP_STATE_FINISHED;
						data->content_len = data->content->len;
						break;
					}
					if( (data->chunked_content->len - (cur_pos - data->chunked_content->str)) >= cur_pos_len ) {
						// copy the string to content, then shift the rest of chunked_content
						purple_debug_info(MB_HTTPID, "appending chunk\n");
						g_string_append_len(data->content, cur_pos + 2, cur_pos_len);
						purple_debug_info(MB_HTTPID, "current content = #%s#\n", data->content->str);
						g_string_erase(data->chunked_content, 0, (cur_pos + 2 + cur_pos_len) - data->chunked_content->str);
					} else {
						// we need more data
						purple_debug_info(MB_HTTPID, "data is not enough, need more\n");
						break;
					}
				}
			} else {
				g_string_append_len(data->content, buf, buf_len);
				if(data->content->len >= data->content_len) {
					data->state = MB_HTTP_STATE_FINISHED;
				}
			}
			break;
		case MB_HTTP_STATE_FINISHED :
			break;
	}
}

void mb_http_data_set_basicauth(MbHttpData * data, const gchar * user, const gchar * passwd)
{
	gchar * merged_tmp, *encoded_tmp, *value_tmp;
	gsize authen_len;

	if(passwd == NULL) {
		purple_debug_info(MB_HTTPID, "Password not set! Shouldn't we ask for it?\n");
		// TODO: This prevents crashing, however ...
		// we should either ask for the users password or disable basic http altogether.
		passwd = "";
	}

	authen_len = strlen(user) + strlen(passwd) + 1;
	merged_tmp = g_strdup_printf("%s:%s", user, passwd);
	encoded_tmp = purple_base64_encode((const guchar *)merged_tmp, authen_len);
	//g_strlcpy(output, encoded_temp, len);
	g_free(merged_tmp);
	value_tmp = g_strdup_printf("Basic %s", encoded_tmp);
	g_free(encoded_tmp);
	mb_http_data_set_header(data, "Authorization", value_tmp);
	g_free(value_tmp);
}

static gint _do_read(gint fd, PurpleSslConnection * ssl, MbHttpData * data)
{
	gint retval;
	gchar * buffer;

	purple_debug_info(MB_HTTPID, "_do_read called\n");
	buffer = g_malloc0(MB_MAXBUFF + 1);
	if(ssl) {
		retval = purple_ssl_read(ssl, buffer, MB_MAXBUFF);
	} else {
		retval = read(fd, buffer, MB_MAXBUFF);
	}
	purple_debug_info(MB_HTTPID, "retval = %d\n", retval);
	purple_debug_info(MB_HTTPID, "buffer = %s\n", buffer);
	if(retval > 0) {
		mb_http_data_post_read(data, buffer, retval);
	} else if(retval == 0) {
		data->state = MB_HTTP_STATE_FINISHED;
		if(data->packet) {
			g_free(data->packet);
		}
	}
	g_free(buffer);
	purple_debug_info(MB_HTTPID, "before return in _do_read\n");

	return retval;
}

gint mb_http_data_read(gint fd, MbHttpData * data)
{
	return _do_read(fd, NULL, data);
}

gint mb_http_data_ssl_read(PurpleSslConnection * ssl, MbHttpData * data)
{
	return _do_read(0, ssl, data);
}

static gint _do_write(gint fd, PurpleSslConnection * ssl, MbHttpData * data)
{
	gint retval, cur_packet_len;

	purple_debug_info(MB_HTTPID, "preparing HTTP data chunk\n");
	if(data->packet == NULL) {
		mb_http_data_prepare_write(data);
	}
	// Do SSL-write, then update cur_packet to proper position. Exit if already exceeding the length
	purple_debug_info(MB_HTTPID, "writing data %s\n", data->cur_packet);
	cur_packet_len = data->packet_len - (data->cur_packet - data->packet);
	if(ssl) {
		retval = purple_ssl_write(ssl, data->cur_packet, cur_packet_len);
	} else {
		retval = write(fd, data->cur_packet, cur_packet_len);
	}
	if(retval >= cur_packet_len)  {
		// everything is written
		purple_debug_info(MB_HTTPID, "we sent all data\n");
		data->state = MB_HTTP_STATE_FINISHED;
		g_free(data->packet);
		data->cur_packet = data->packet = NULL;
		data->packet_len = 0;
		//return retval;
	} else if( (retval > 0) && (retval < cur_packet_len)) {
		purple_debug_info(MB_HTTPID, "more data must be sent\n");
		data->cur_packet = data->cur_packet + retval;
	}
	return retval;
}

gint mb_http_data_write(gint fd, MbHttpData * data)
{
	return _do_write(fd, NULL, data);
}

gint mb_http_data_ssl_write(PurpleSslConnection * ssl, MbHttpData * data)
{
	return _do_write(0, ssl, data);
}

#ifdef UTEST

static void print_hash_value(gpointer key, gpointer value, gpointer udata)
{
	printf("key = %s, value = %s\n", (char *)key, (char *)value);
}

int main(int argc, char * argv[])
{
	MbHttpData * hdata = NULL;
	GList * it;
	char buf[512];
	FILE * fp;
	size_t retval;

	g_mem_set_vtable(glib_mem_profiler_table);
	// URL

	hdata = mb_http_data_new();
	mb_http_data_set_url(hdata, "https://twitter.com/statuses/friends_timeline.xml");
	printf("URL to set = %s\n", "https://twitter.com/statuses/friends_timeline.xml");
	printf("host = %s\n", hdata->host);
	printf("port = %d\n", hdata->port);
	printf("proto = %d\n", hdata->proto);
	printf("path = %s\n", hdata->path);


	mb_http_data_set_url(hdata, "http://twitter.com/statuses/update.atom");
	printf("URL to set = %s\n", "http://twitter.com/statuses/update.atom");
	printf("host = %s\n", hdata->host);
	printf("port = %d\n", hdata->port);
	printf("proto = %d\n", hdata->proto);
	printf("path = %s\n", hdata->path);

	// header
	mb_http_data_set_header(hdata, "User-Agent", "CURL");
	mb_http_data_set_header(hdata, "X-Twitter-Client", "1024");
	mb_http_data_set_fixed_headers(hdata, "X-Twitter-Host: 12345678\r\n\
X-Twitter-ABC: 5sadlfjas;dfasdfasdf\r\n");
	mb_http_data_set_basicauth(hdata, "user1", "passwd1");
	printf("Header %s = %s\n", "User-Agent", mb_http_data_get_header(hdata, "User-Agent"));
	printf("Header %s = %s\n", "Content-Length", mb_http_data_get_header(hdata, "X-Twitter-Client"));
	printf("Header %s = %s\n", "XXX", mb_http_data_get_header(hdata, "XXX"));


	// param
	mb_http_data_add_param(hdata, "key1", "valuevalue1");
	mb_http_data_add_param(hdata, "key2", "valuevalue1 bcadf");
	mb_http_data_add_param(hdata, "key1", "valuevalue1");
	mb_http_data_add_param_int(hdata, "keyint1", 1000000);

	printf("Param %s = %s\n", "key1", mb_http_data_find_param(hdata, "key1"));
	printf("Param %s = %s\n", "key2", mb_http_data_find_param(hdata, "key2"));

	for(it = g_list_first(hdata->params); it; it = g_list_next(it)) {
		MbHttpParam * p = it->data;

		printf("Key = %s, Value = %s\n", p->key, p->value);
	}

	printf("Before remove\n");
	mb_http_data_rm_param(hdata, "key1");

	printf("After remove\n");
	for(it = g_list_first(hdata->params); it; it = g_list_next(it)) {
		MbHttpParam * p = it->data;

		printf("Key = %s, Value = %s\n", p->key, p->value);
	}
	printf("data->path = %s\n", hdata->path);
	mb_http_data_prepare_write(hdata);

	printf("data prepared to write\n");
	printf("%s\n", hdata->packet);
	printf("packet_len = %d, strlen = %d\n", hdata->packet_len, strlen(hdata->packet));
//	printf("%s\n", buf);
	mb_http_data_free(hdata);
	hdata =mb_http_data_new();


	fp = fopen("test_input.xml", "r");
	while(!feof(fp)) {
		retval = fread(buf, sizeof(char), sizeof(buf), fp);
		mb_http_data_post_read(hdata, buf, retval);
	}
	fclose(fp);
	printf("http status = %d\n", hdata->status);
	g_hash_table_foreach(hdata->headers, print_hash_value, NULL);
	printf("http content length = %d\n", hdata->content_len);
	if(hdata->content_len > 0)
		printf("http content = %s\n", hdata->content->str);

	// test again, after truncated
	mb_http_data_truncate(hdata);
	fp = fopen("test_input.xml", "r");
	while(!feof(fp)) {
		retval = fread(buf, sizeof(char), sizeof(buf), fp);
		mb_http_data_post_read(hdata, buf, retval);
	}
	fclose(fp);

	printf("http status = %d\n", hdata->status);
	g_hash_table_foreach(hdata->headers, print_hash_value, NULL);
	printf("http content length = %d\n", hdata->content_len);
	if(hdata->content_len > 0)
		printf("http content = %s\n", hdata->content->str);

	mb_http_data_free(hdata);
	g_mem_profile();

	return 0;
}
#endif
