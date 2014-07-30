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
 * Command handler for twitter-based protocol
 */

#ifndef __TW_CMD__
#define __TW_CMD__

#include <glib.h>
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

#include <sslconn.h>
#include <cmds.h>

#include "twitter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _TwCmdArg;

typedef PurpleCmdRet (* TwCmdFunc)(PurpleConversation *, const gchar *, gchar **, gchar **, struct _TwCmdArg *);

typedef struct _TwCmdArg {
	MbAccount * ma;
	TwCmdFunc func;
	void * data;
} TwCmdArg;

typedef struct {
	char * protocol_id;
	PurpleCmdId * cmd_id;
	TwCmdArg ** cmd_args;
	int cmd_id_num;
} TwCmd;


extern TwCmd * tw_cmd_init(const char * protocol_id);
extern void tw_cmd_finalize(TwCmd * tc);


#ifdef __cplusplus
}
#endif

#endif
