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

#include <glib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib/gi18n.h>
#include <sys/types.h>
//#include <gtkconv.h>
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

#include "mb_net.h"
#include "mb_util.h"

#ifdef _WIN32
#	include <win32dep.h>
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#include "twitter.h"

#ifndef STATUSNET

#define LOG_ID "idcim"
#define _USER_GROUP "Identi.ca"
#define _FRIENDS_USER "identi.ca"
#define _PUBLIC_USER "idcpublic"
#define _USER_USER "idcuser"

#else

#define LOG_ID "status.net"
#define _FRIENDS_USER "status.net"
#define _PUBLIC_USER "Public"
#define _USER_USER "Personal"
#define _USER_GROUP "Status.net"


#endif

MbConfig * _mb_conf = NULL;

static void plugin_init(PurplePlugin *plugin)
{
	purple_debug_info("twitterim", "plugin_init\n");
	purple_debug_info("twitterim", "plugin = %p\n", plugin);

	purple_signal_register(plugin, "twitter-message",
                                                 purple_marshal_VOID__POINTER_POINTER_POINTER,
                                                 NULL, 3,
                                                 purple_value_new(PURPLE_TYPE_POINTER), // MbAccount ta
                                                 purple_value_new(PURPLE_TYPE_STRING), // gchar * name
                                                 purple_value_new(PURPLE_TYPE_POINTER) // TwitterMsg cur_msg
                                                 );
}

static void plugin_destroy(PurplePlugin * plugin)
{
	purple_debug_info("twitterim", "plugin_destroy\n");
	purple_signal_unregister(plugin, "twitter-message");
}

gboolean plugin_load(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
#ifdef STATUSNET
	PurpleAccountUserSplit * split;
#endif
	PurplePluginInfo *info = plugin->info;
	PurplePluginProtocolInfo *prpl_info = info->extra_info;
	
	purple_debug_info(LOG_ID, "plugin_load\n");
	_mb_conf = (MbConfig *)g_malloc0(TC_MAX * sizeof(MbConfig));

	// This is just the place to pass pointer to plug-in itself
	_mb_conf[TC_PLUGIN].conf = NULL;
	_mb_conf[TC_PLUGIN].def_str = (gchar *)plugin;

	_mb_conf[TC_HIDE_SELF].conf = g_strdup("hide_myself");
	_mb_conf[TC_HIDE_SELF].def_bool = TRUE;
	option = purple_account_option_bool_new(_("Hide myself in conversation"), _mb_conf[TC_HIDE_SELF].conf, _mb_conf[TC_HIDE_SELF].def_bool);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_MSG_REFRESH_RATE].conf = g_strdup("msg_refresh_rate");
	_mb_conf[TC_MSG_REFRESH_RATE].def_int = 60;
	option = purple_account_option_int_new(_("Message refresh rate (seconds)"), _mb_conf[TC_MSG_REFRESH_RATE].conf, _mb_conf[TC_MSG_REFRESH_RATE].def_int);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_INITIAL_TWEET].conf = g_strdup("init_tweet");
	_mb_conf[TC_INITIAL_TWEET].def_int = 15;
	option = purple_account_option_int_new(_("Number of initial tweets"), _mb_conf[TC_INITIAL_TWEET].conf, _mb_conf[TC_INITIAL_TWEET].def_int);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_GLOBAL_RETRY].conf = g_strdup("global_retry");
	_mb_conf[TC_GLOBAL_RETRY].def_int = 3 ;
	option = purple_account_option_int_new(_("Maximum number of retry"), _mb_conf[TC_GLOBAL_RETRY].conf, _mb_conf[TC_GLOBAL_RETRY].def_int);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

#ifndef STATUSNET
	_mb_conf[TC_HOST].conf = g_strdup("hostname");
	_mb_conf[TC_HOST].def_str = g_strdup("identi.ca");
	option = purple_account_option_string_new(_("Identi.ca hostname"), _mb_conf[TC_HOST].conf, _mb_conf[TC_HOST].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
#else
	split = purple_account_user_split_new(_("Server"), "status.net", '@');
	prpl_info->user_splits = g_list_append(prpl_info->user_splits, split);
	
#endif
	
	/*
	 * No HTTPS for Identi.ca for now
	 */
	_mb_conf[TC_USE_HTTPS].conf = g_strdup("use_https");
	_mb_conf[TC_USE_HTTPS].def_bool = FALSE;
	
	_mb_conf[TC_STATUS_UPDATE].conf = g_strdup("status_update");
	_mb_conf[TC_STATUS_UPDATE].def_str = g_strdup("/api/statuses/update.xml");
	option = purple_account_option_string_new(_("Status update path"), _mb_conf[TC_STATUS_UPDATE].conf, _mb_conf[TC_STATUS_UPDATE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_VERIFY_PATH].conf = g_strdup("verify");
	_mb_conf[TC_VERIFY_PATH].def_str = g_strdup("/api/account/verify_credentials.xml");
	option = purple_account_option_string_new(_("Account verification path"), _mb_conf[TC_VERIFY_PATH].conf, _mb_conf[TC_VERIFY_PATH].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_FRIENDS_TIMELINE].conf = g_strdup("friends_timeline");
	_mb_conf[TC_FRIENDS_TIMELINE].def_str = g_strdup("/api/statuses/friends_timeline.xml");
	option = purple_account_option_string_new(_("Friends timeline path"), _mb_conf[TC_FRIENDS_TIMELINE].conf, _mb_conf[TC_FRIENDS_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_USER_TIMELINE].conf = g_strdup("user_timeline");
	_mb_conf[TC_USER_TIMELINE].def_str = g_strdup("/api/statuses/user_timeline.xml");
	option = purple_account_option_string_new(_("User timeline path"), _mb_conf[TC_USER_TIMELINE].conf, _mb_conf[TC_USER_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_PUBLIC_TIMELINE].conf = g_strdup("public_timeline");
	_mb_conf[TC_PUBLIC_TIMELINE].def_str = g_strdup("/api/statuses/public_timeline.xml");
	option = purple_account_option_string_new(_("Public timeline path"), _mb_conf[TC_PUBLIC_TIMELINE].conf, _mb_conf[TC_PUBLIC_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	// and now for non-option global
	_mb_conf[TC_FRIENDS_USER].def_str = g_strdup(_FRIENDS_USER);
	_mb_conf[TC_PUBLIC_USER].def_str = g_strdup(_PUBLIC_USER);
	_mb_conf[TC_USER_USER].def_str = g_strdup(_USER_USER);
	_mb_conf[TC_USER_GROUP].def_str = g_strdup(_USER_GROUP);

	return TRUE;
}

gboolean plugin_unload(PurplePlugin *plugin)
{
	gint i;

	purple_debug_info(LOG_ID, "plugin_unload\n");

	g_free(_mb_conf[TC_HOST].def_str);
	g_free(_mb_conf[TC_STATUS_UPDATE].def_str);
	g_free(_mb_conf[TC_VERIFY_PATH].def_str);
	g_free(_mb_conf[TC_FRIENDS_TIMELINE].def_str);
	g_free(_mb_conf[TC_USER_TIMELINE].def_str);
	g_free(_mb_conf[TC_PUBLIC_TIMELINE].def_str);
	g_free(_mb_conf[TC_FRIENDS_USER].def_str);
	g_free(_mb_conf[TC_PUBLIC_USER].def_str);
	g_free(_mb_conf[TC_USER_USER].def_str);
	for(i = 0; i < TC_MAX; i++) {
		if(_mb_conf[i].conf) g_free(_mb_conf[i].conf);
	}

	g_free(_mb_conf);
	return TRUE;
}

const char * idcim_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
#ifndef STATUSNET
	return "identica";
#else
	return "statusnet";
#endif
}

GList * idcim_actions(PurplePlugin *plugin, gpointer context)
{
	GList *m = NULL;
	/*
	PurplePluginAction *act;

	act = purple_plugin_action_new(_("Set Facebook status..."), idcim_set_status_cb);
	m = g_list_append(m, act);
	
	act = purple_plugin_action_new(_("Search for buddies..."), idcim_search_users);
	m = g_list_append(m, act);
	*/
	
	return m;
}



PurplePluginProtocolInfo prpl_info = {
	/* options */
	OPT_PROTO_UNIQUE_CHATNAME,
	NULL,                   /* user_splits */
	NULL,                   /* protocol_options */
	//NO_BUDDY_ICONS          /* icon_spec */
	{   /* icon_spec, a PurpleBuddyIconSpec */
		"png,jpg,gif",                   /* format */
		0,                               /* min_width */
		0,                               /* min_height */
		50,                             /* max_width */
		50,                             /* max_height */
		10000,                           /* max_filesize */
		PURPLE_ICON_SCALE_DISPLAY,       /* scale_rules */
	},
	idcim_list_icon,   /* list_icon */
	NULL,                   /* list_emblems */
	twitter_status_text, /* status_text */
//	idcim_tooltip_text,/* tooltip_text */
	NULL,
	twitter_statuses,    /* status_types */
	NULL,                   /* blist_node_menu */
	NULL,                   /* chat_info */
	NULL,                   /* chat_info_defaults */
	twitter_login,       /* login */
	twitter_close,       /* close */
	twitter_send_im,     /* send_im */
	NULL,                   /* set_info */
//	idcim_send_typing, /* send_typing */
	NULL,
//	idcim_get_info,    /* get_info */
	NULL,
	twitter_set_status,/* set_status */
	NULL,                   /* set_idle */
	NULL,                   /* change_passwd */
//	idcim_add_buddy,   /* add_buddy */
	NULL,
	NULL,                   /* add_buddies */
//	idcim_remove_buddy,/* remove_buddy */
	NULL,
	NULL,                   /* remove_buddies */
	NULL,                   /* add_permit */
	NULL,                   /* add_deny */
	NULL,                   /* rem_permit */
	NULL,                   /* rem_deny */
	NULL,                   /* set_permit_deny */
	NULL,                   /* join_chat */
	NULL,                   /* reject chat invite */
	NULL,                   /* get_chat_name */
	NULL,                   /* chat_invite */
	NULL,                   /* chat_leave */
	NULL,                   /* chat_whisper */
	NULL,                   /* chat_send */
	NULL,                   /* keepalive */
	NULL,                   /* register_user */
	NULL,                   /* get_cb_info */
	NULL,                   /* get_cb_away */
	NULL,                   /* alias_buddy */
	NULL,                   /* group_buddy */
	NULL,                   /* rename_group */
	twitter_buddy_free,  /* buddy_free */
	NULL,                   /* convo_closed */
	purple_normalize_nocase,/* normalize */
	NULL,                   /* set_buddy_icon */
	NULL,                   /* remove_group */
	NULL,                   /* get_cb_real_name */
	NULL,                   /* set_chat_topic */
	NULL,                   /* find_blist_chat */
	NULL,                   /* roomlist_get_list */
	NULL,                   /* roomlist_cancel */
	NULL,                   /* roomlist_expand_category */
	NULL,                   /* can_receive_file */
	NULL,                   /* send_file */
	NULL,                   /* new_xfer */
	NULL,                   /* offline_message */
	NULL,                   /* whiteboard_prpl_ops */
	NULL,                   /* send_raw */
	NULL,                   /* roomlist_room_serialize */
	NULL,                   /* unregister_user */
	NULL,                   /* send_attention */
	NULL,                   /* attention_types */
	sizeof(PurplePluginProtocolInfo) /* struct_size */
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL, /* type */
	NULL, /* ui_requirement */
	0, /* flags */
	NULL, /* dependencies */
	PURPLE_PRIORITY_DEFAULT, /* priority */
#ifndef STATUSNET
	"prpl-mbpurple-identica", /* id */
	"Identi.ca", /* name */
#else
	"prpl-mbpurple-laconica", /* id */ //< Keep the old name for compatibility sake
	"Status.net", /* name */
#endif
	MBPURPLE_VERSION, /* version */
#ifndef STATUSNET
	"Identi.ca data feeder", /* summary */
	"Identi.ca data feeder", /* description */
#else
	"Status.net data feeder", /* summary */
	"Status.net data feeder", /* description */
#endif
	"Somsak Sriprayoonsakul <somsaks@gmail.com>", /* author */
	"http://microblog-purple.googlecode.com/", /* homepage */
	plugin_load, /* load */
	plugin_unload, /* unload */
	plugin_destroy, /* destroy */
	NULL, /* ui_info */
	&prpl_info, /* extra_info */
	NULL, /* prefs_info */
	idcim_actions, /* actions */
	NULL, /* padding */
	NULL,
	NULL,
	NULL
};

#ifndef STATUSNET
PURPLE_INIT_PLUGIN(idcim, plugin_init, info);
#else
PURPLE_INIT_PLUGIN(lcim, plugin_init, info);
#endif
