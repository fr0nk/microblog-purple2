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
 
#ifndef __MB_NET__
#define __MB_NET__

#include <util.h>

#include "mb_http.h"
#include "twitter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MB_NET "mb_net"

//typedef TwitterAccount MbAccount; //< for the sake of simplicity for now

enum mb_error_action {
	MB_ERROR_NOACTION = 0,
	MB_ERROR_RAISE_ERROR = 1,
};

// if handler return
// 0 - Everything's ok
// -1 - Requeue the whole process again
struct _MbConnData;

typedef gint (*MbHandlerFunc)(struct _MbConnData * , gpointer , const char * error);
typedef void (*MbHandlerDataFreeFunc)(gpointer);

typedef struct _MbConnData {
	gchar * host;
	gint port;
	MbAccount * ma;
	gchar * error_message;
	MbHttpData * request;
	MbHttpData * response;
	gint retry;
	gint max_retry;

	// Packet preparer, created for OAuth
	// We will not set prepare_handler back to NULL, so it will be called again when connection is retried
	// The 3rd argument pass will always be NULL (since no connection has been made yet
	MbHandlerFunc prepare_handler;
	gpointer prepare_handler_data;

	// Connection handler
	MbHandlerFunc handler;
	gpointer handler_data;

	gboolean is_ssl;
	PurpleUtilFetchUrlData * fetch_url_data;
} MbConnData;

/*
	Create new connection data
	
	@param ta MbAccount instance
	@param handler handler function for this request
	@param is_ssl whether this is SSL or not
	@return new MbConnData
*/
extern MbConnData * mb_conn_data_new(MbAccount * ta, const gchar * host, gint port, MbHandlerFunc handler, gboolean is_ssl);

/*
	Free an instance of MbConnData
	
	@param conn_data conn_data to free
	@note connection will be closed if it's still open
*/
extern void mb_conn_data_free(MbConnData * conn_data);

/**
 * Set maximum number of retry
 *
 * @param data MbConnData in action
 * @param retry number of desired retry
 */
extern void mb_conn_data_set_retry(MbConnData * data, gint retry);

/**
 * Process the initialize MbConnData
 *
 * @param data MbConnData to process
 */
extern void mb_conn_process_request(MbConnData * data);

/**
 * Call purple_connection_error_reason if this connection was retried more than data->max_retry already
 *
 * @param data MbConnData in action
 * @param error error reason
 * @param description text description
 */
extern void mb_conn_error(MbConnData * data, PurpleConnectionError error, const char * description);

/**
 * Create full URL from MbConnData
 *
 * @param data MbConnData in action
 * @return full URL, need to be freed after use
 */
extern gchar * mb_conn_url_unparse(MbConnData * data);

/**
 * Test if the maximu retry is already reached
 *
 * @param data MbConnData in action
 * @return TRUE if max retry is reached, otherwise false
 */
extern gboolean mb_conn_max_retry_reach(MbConnData * data);

#ifdef __cplusplus
}
#endif

#endif
