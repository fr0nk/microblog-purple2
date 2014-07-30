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
#include <glib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <time.h>

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

#ifdef _WIN32
#	include <win32dep.h>
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#include "mb_util.h"
#include "twitter.h"

#define DBGID "tw_util"



void twitter_get_user_host(const MbAccount * ma, char ** user_name, char ** host)
{
	char * at_sign = NULL;

	purple_debug_info(DBGID, "%s\n", __FUNCTION__);
	(*user_name) = g_strdup(purple_account_get_username(ma->account));
	purple_debug_info(DBGID, "username = ##%s##\n", (*user_name));
	if( (at_sign = strrchr(*user_name, '@')) == NULL) {
		if(host != NULL) {
			(*host) = g_strdup(purple_account_get_string(ma->account, mc_name(TC_HOST), mc_def(TC_HOST)));
			purple_debug_info(DBGID, "host (config) = %s\n", (*host));
		}
	} else {
		(*at_sign) = '\0';
		if(host != NULL) {
			(*host) = g_strdup( at_sign + 1 );
			purple_debug_info(DBGID, "host = %s\n", (*host));
		}
	}
}
