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
	Microblog network processing (mostly for HTTP data)
 */

#include <glib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#ifdef _WIN32
#	include <win32dep.h>
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#include <util.h>
#include <debug.h>
#include "mb_net.h"

// Caller of request retry function
static gboolean mb_conn_retry_request(gpointer data);
// Fetch URL callback
static void mb_conn_fetch_url_cb(PurpleUtilFetchUrlData * url_data, gpointer user_data, const gchar * url_text, gsize len, const gchar * error_message);
 
MbConnData * mb_conn_data_new(MbAccount * ma, const gchar * host, gint port, MbHandlerFunc handler, gboolean is_ssl)
{
	MbConnData * conn_data = NULL;
	
	conn_data = g_new(MbConnData, 1);
	
	conn_data->host = g_strdup(host);
	conn_data->port = port;
	conn_data->ma = ma;
	conn_data->prepare_handler = NULL;
	conn_data->prepare_handler_data = NULL;
	conn_data->handler = handler;
	conn_data->handler_data = NULL;
	conn_data->retry = 0;
	conn_data->max_retry = 0;
	//conn_data->conn_data = NULL;
	conn_data->is_ssl = is_ssl;
	conn_data->request = mb_http_data_new();
	conn_data->response = mb_http_data_new();
	if(conn_data->is_ssl) {
		conn_data->request->proto = MB_HTTPS;
	} else {
		conn_data->request->proto = MB_HTTP;
	}

	conn_data->fetch_url_data = NULL;
	
	purple_debug_info(MB_NET, "new: create conn_data = %p\n", conn_data);
	ma->conn_data_list = g_slist_prepend(ma->conn_data_list, conn_data);
	purple_debug_info(MB_NET, "registered new connection data with MbAccount\n");
	return conn_data;
}

void mb_conn_data_free(MbConnData * conn_data)
{
	purple_debug_info(MB_NET, "%s: conn_data = %p\n", __FUNCTION__, conn_data);

	if(conn_data->fetch_url_data) {
		purple_util_fetch_url_cancel(conn_data->fetch_url_data);
	}

	if(conn_data->host) {
		purple_debug_info(MB_NET, "freeing host name\n");
		g_free(conn_data->host);
	}

	purple_debug_info(MB_NET, "freeing HTTP data->response\n");
	if(conn_data->response)	mb_http_data_free(conn_data->response);

	purple_debug_info(MB_NET, "freeing HTTP data->request\n");
	if(conn_data->request)	mb_http_data_free(conn_data->request);

	purple_debug_info(MB_NET, "unregistering conn_data from MbAccount\n");
	if(conn_data->ma->conn_data_list) {
		GSList * list = g_slist_find(conn_data->ma->conn_data_list, conn_data);
		if(list) {
			conn_data->ma->conn_data_list = g_slist_delete_link(conn_data->ma->conn_data_list, list);
		}
	}
	purple_debug_info(MB_NET, "freeing self at %p\n", conn_data);
	g_free(conn_data);
}

void mb_conn_data_set_retry(MbConnData * data, gint retry)
{
	data->max_retry = retry;
}

gchar * mb_conn_url_unparse(MbConnData * data)
{
	gchar port_str[20];

	if( ((data->port == 80) && !(data->is_ssl)) ||
		((data->port == 443) && (data->is_ssl)))
	{
		port_str[0] = '\0';
	} else {
		snprintf(port_str, 19, ":%hd", data->port);
	}

	// parameter is ignored here since we handle header ourself
	return g_strdup_printf("%s%s%s%s%s",
			data->is_ssl ? "https://" : "http://",
			data->host,
			port_str,
			(data->request->path[0] == '/') ? "" : "/",
			data->request->path
		);
}


void mb_conn_fetch_url_cb(PurpleUtilFetchUrlData * url_data, gpointer user_data, const gchar * url_text, gsize len, const gchar * error_message)
{
	MbConnData * conn_data = (MbConnData *)user_data;
	MbAccount * ma = conn_data->ma;
	gint retval;

	purple_debug_info(MB_NET, "%s: url_data = %p\n", __FUNCTION__, url_data);
	// in whatever situation, url_data should be handled only by libpurple
	conn_data->fetch_url_data = NULL;

	if(error_message != NULL) {
		if(conn_data->handler) {
			retval = conn_data->handler(conn_data, conn_data->handler_data, error_message);
		}
		if(ma->gc != NULL) {
			purple_connection_error_reason(ma->gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, error_message);
		}
        mb_conn_data_free(conn_data);
	} else {
		mb_http_data_post_read(conn_data->response, url_text, len);
		if(conn_data->handler) {

			purple_debug_info(MB_NET, "going to call handler\n");
			retval = conn_data->handler(conn_data, conn_data->handler_data, NULL);
			purple_debug_info(MB_NET, "handler returned, retval = %d\n", retval);

			if(retval == 0) {
				// Everything's good. Free data structure and go-on with usual works
				purple_debug_info(MB_NET, "everything's ok, freeing data\n");
				mb_conn_data_free(conn_data);
			} else if(retval == -1) {
				// Something's wrong. Requeue the whole process
				conn_data->retry++;
				if(conn_data->retry <= conn_data->max_retry) {
					purple_debug_info(MB_NET, "handler return -1, conn_data %p, retry %d, max_retry = %d\n", conn_data, conn_data->retry, conn_data->max_retry);
					mb_http_data_truncate(conn_data->response);
					// retry again in 1 second
					purple_timeout_add_seconds(1, mb_conn_retry_request, conn_data);
				} else {
					purple_debug_info(MB_NET, "retry exceed %d > %d\n", conn_data->retry, conn_data->max_retry);
					mb_conn_data_free(conn_data);
				}
			} 
		}
	}
}

static gboolean mb_conn_retry_request(gpointer data)
{
	MbConnData * conn_data = (MbConnData *)data;

	mb_conn_process_request(conn_data);
	return FALSE;
}

void mb_conn_process_request(MbConnData * data)
{
	gchar * url;

	purple_debug_info(MB_NET, "NEW mb_conn_process_request, conn_data = %p\n", data);

	purple_debug_info(MB_NET, "connecting to %s on port %hd\n", data->host, data->port);

	if(data->prepare_handler) {
		data->prepare_handler(data, data->prepare_handler_data, NULL);
	}
	url = mb_conn_url_unparse(data);

	// we manage user_agent by ourself so ignore this completely
	mb_http_data_prepare_write(data->request);
	data->fetch_url_data = purple_util_fetch_url_request(url, TRUE, "", TRUE, data->request->packet, TRUE, mb_conn_fetch_url_cb, (gpointer)data);
	g_free(url);
}

void mb_conn_error(MbConnData * data, PurpleConnectionError error, const char * description)
{
	if(data->retry >= data->max_retry) {
		data->ma->state = PURPLE_DISCONNECTED;
		purple_connection_error_reason(data->ma->gc, error, description);
	}
}

gboolean mb_conn_max_retry_reach(MbConnData * data) {
	return (gboolean)(data->retry >= data->max_retry);
}
