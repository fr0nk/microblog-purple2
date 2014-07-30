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
//#include <gtkconv.h>

#include "mb_net.h"
#include "mb_util.h"
#include "tw_cmd.h"

#ifdef _WIN32
#	include <win32dep.h>
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif

#include "twitter.h"
#include "mb_cache.h"

MbConfig * _mb_conf = NULL;

static TwCmd * tw_cmd = NULL;

PurplePlugin * prpl_plugin = NULL;

void twitterim_remove_oauth(PurplePluginAction * action);

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
	mb_cache_init();
}

static void plugin_destroy(PurplePlugin * plugin)
{
	purple_debug_info("twitterim", "plugin_destroy\n");
	purple_signal_unregister(plugin, "twitter-message");
}

gboolean plugin_load(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
	PurplePluginInfo *info = plugin->info;
	PurplePluginProtocolInfo *prpl_info = info->extra_info;
	GList * auth_type_list = NULL;
	PurpleKeyValuePair * kv;
	
	purple_debug_info("twitterim", "plugin_load\n");

	_mb_conf = (MbConfig *)g_malloc0(TC_MAX * sizeof(MbConfig));

	// This is just the place to pass pointer to plug-in itself
	_mb_conf[TC_PLUGIN].conf = NULL;
	_mb_conf[TC_PLUGIN].def_str = (gchar *)plugin;

	// Authentication types
	_mb_conf[TC_AUTH_TYPE].conf = g_strdup("twitter_auth_type");
	_mb_conf[TC_AUTH_TYPE].def_str = g_strdup(mb_auth_types_str[MB_OAUTH]);

	kv = g_new(PurpleKeyValuePair, 1);
	kv->key = g_strdup("OAuth");
	kv->value = g_strdup((char *)mb_auth_types_str[MB_OAUTH]);
	auth_type_list = g_list_append(auth_type_list, kv);

	/*
	kv = g_new(PurpleKeyValuePair, 1);
	kv->key = g_strdup("XAuth");
	kv->value = g_strdup((char *)mb_auth_types_str[MB_XAUTH]);
	auth_type_list = g_list_append(auth_type_list, kv);
	*/

	kv = g_new(PurpleKeyValuePair, 1);
	kv->key = g_strdup("HTTP Basic Authentication (old method)");
	kv->value = g_strdup((char *)mb_auth_types_str[MB_HTTP_BASICAUTH]);
	auth_type_list = g_list_append(auth_type_list, kv);

	option = purple_account_option_list_new(_("Authentication Method"), _mb_conf[TC_AUTH_TYPE].conf, auth_type_list);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	// Hide myself
	_mb_conf[TC_HIDE_SELF].conf = g_strdup("twitter_hide_myself");
	_mb_conf[TC_HIDE_SELF].def_bool = TRUE;
	option = purple_account_option_bool_new(_("Hide myself in conversation"), _mb_conf[TC_HIDE_SELF].conf, _mb_conf[TC_HIDE_SELF].def_bool);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_PRIVACY].conf = g_strdup("twitter_privacy");
	_mb_conf[TC_PRIVACY].def_bool = FALSE;
	option = purple_account_option_bool_new(_("Not receive messages while unavailable"), _mb_conf[TC_PRIVACY].conf, _mb_conf[TC_PRIVACY].def_bool);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_MSG_REFRESH_RATE].conf = g_strdup("twitter_msg_refresh_rate");
	_mb_conf[TC_MSG_REFRESH_RATE].def_int = 60;
	option = purple_account_option_int_new(_("Message refresh rate (seconds)"), _mb_conf[TC_MSG_REFRESH_RATE].conf, _mb_conf[TC_MSG_REFRESH_RATE].def_int);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_INITIAL_TWEET].conf = g_strdup("twitter_init_tweet");
	_mb_conf[TC_INITIAL_TWEET].def_int = 15;
	option = purple_account_option_int_new(_("Number of initial tweets"), _mb_conf[TC_INITIAL_TWEET].conf, _mb_conf[TC_INITIAL_TWEET].def_int);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_GLOBAL_RETRY].conf = g_strdup("twitter_global_retry");
	_mb_conf[TC_GLOBAL_RETRY].def_int = 3 ;
	option = purple_account_option_int_new(_("Maximum number of retry"), _mb_conf[TC_GLOBAL_RETRY].conf, _mb_conf[TC_GLOBAL_RETRY].def_int);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_HOST].conf = g_strdup("twitter_hostname");
	_mb_conf[TC_HOST].def_str = g_strdup("api.twitter.com");
	option = purple_account_option_string_new(_("Hostname"), _mb_conf[TC_HOST].conf, _mb_conf[TC_HOST].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_USE_HTTPS].conf = g_strdup("twitter_use_https");
	_mb_conf[TC_USE_HTTPS].def_bool = TRUE;
	option = purple_account_option_bool_new(_("Use HTTPS"), _mb_conf[TC_USE_HTTPS].conf, _mb_conf[TC_USE_HTTPS].def_bool);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_STATUS_UPDATE].conf = g_strdup("twitter_status_update");
	_mb_conf[TC_STATUS_UPDATE].def_str = g_strdup("/1.1/statuses/update.json");
	option = purple_account_option_string_new(_("Status update path"), _mb_conf[TC_STATUS_UPDATE].conf, _mb_conf[TC_STATUS_UPDATE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_VERIFY_PATH].conf = g_strdup("twitter_verify");
	_mb_conf[TC_VERIFY_PATH].def_str = g_strdup("/1.1/account/verify_credentials.json");
	option = purple_account_option_string_new(_("Account verification path"), _mb_conf[TC_VERIFY_PATH].conf, _mb_conf[TC_VERIFY_PATH].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_FRIENDS_TIMELINE].conf = g_strdup("twitter_friends_timeline");
	_mb_conf[TC_FRIENDS_TIMELINE].def_str = g_strdup("/1.1/statuses/home_timeline.json");
	option = purple_account_option_string_new(_("Home timeline path"), _mb_conf[TC_FRIENDS_TIMELINE].conf, _mb_conf[TC_FRIENDS_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_USER_TIMELINE].conf = g_strdup("twitter_user_timeline");
	_mb_conf[TC_USER_TIMELINE].def_str = g_strdup("/1.1/statuses/user_timeline.json");
	option = purple_account_option_string_new(_("User timeline path"), _mb_conf[TC_USER_TIMELINE].conf, _mb_conf[TC_USER_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
	_mb_conf[TC_PUBLIC_TIMELINE].conf = g_strdup("twitter_public_timeline");
	_mb_conf[TC_PUBLIC_TIMELINE].def_str = g_strdup("/1.1/statuses/public_timeline.json");
	option = purple_account_option_string_new(_("Public timeline path"), _mb_conf[TC_PUBLIC_TIMELINE].conf, _mb_conf[TC_PUBLIC_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_REPLIES_TIMELINE].conf = g_strdup("twitter_replies_timeline");
	_mb_conf[TC_REPLIES_TIMELINE].def_str = g_strdup("/1.1/statuses/mentions.json");
	option = purple_account_option_string_new(_("Mentions timeline path"), _mb_conf[TC_REPLIES_TIMELINE].conf, _mb_conf[TC_REPLIES_TIMELINE].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	// critical options with no setting page
	_mb_conf[TC_OAUTH_TOKEN].conf = g_strdup("twitter_oauth_token");
	_mb_conf[TC_OAUTH_TOKEN].def_str = NULL;
	_mb_conf[TC_OAUTH_SECRET].conf = g_strdup("twitter_oauth_secret");
	_mb_conf[TC_OAUTH_SECRET].def_str = NULL;

	// and now for non-option global
	_mb_conf[TC_FRIENDS_USER].def_str = g_strdup("twitter.com");
	_mb_conf[TC_REPLIES_USER].def_str = g_strdup("twitter.com");
	_mb_conf[TC_PUBLIC_USER].def_str = g_strdup("twpublic");
	_mb_conf[TC_USER_USER].def_str = g_strdup("twuser");
	_mb_conf[TC_USER_GROUP].def_str = g_strdup("Twitter");

	// OAuth stuff
	
	_mb_conf[TC_CONSUMER_KEY].def_str = g_strdup(purple_prefs_get_string("TC_CONSUMER_KEY") ? purple_prefs_get_string("TC_CONSUMER_KEY") : "PCWAdQpyyR12ezp2fVwEhw");
	_mb_conf[TC_CONSUMER_SECRET].def_str = g_strdup(purple_prefs_get_string("TC_CONSUMER_KEY") ? purple_prefs_get_string("TC_CONSUMER_KEY") : "EveLmCXJIg2R7BTCpm6OWV8YyX49nI0pxnYXh7JMvDg");

	_mb_conf[TC_REQUEST_TOKEN_URL].conf = g_strdup("twitter_oauth_request_token_url");
	_mb_conf[TC_REQUEST_TOKEN_URL].def_str = g_strdup("/oauth/request_token");
	option = purple_account_option_string_new(_("OAuth request token path"), _mb_conf[TC_REQUEST_TOKEN_URL].conf, _mb_conf[TC_REQUEST_TOKEN_URL].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_ACCESS_TOKEN_URL].conf = g_strdup("twitter_oauth_access_token_url");
	_mb_conf[TC_ACCESS_TOKEN_URL].def_str = g_strdup("/oauth/access_token");
	option = purple_account_option_string_new(_("OAuth access token path"), _mb_conf[TC_ACCESS_TOKEN_URL].conf, _mb_conf[TC_ACCESS_TOKEN_URL].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	_mb_conf[TC_AUTHORIZE_URL].conf = g_strdup("twitter_oauth_authorize_token_url");
	_mb_conf[TC_AUTHORIZE_URL].def_str = g_strdup("/oauth/authorize");
	option = purple_account_option_string_new(_("OAuth authorize path"), _mb_conf[TC_AUTHORIZE_URL].conf, _mb_conf[TC_AUTHORIZE_URL].def_str);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	// command support
	tw_cmd = tw_cmd_init(info->id);

	return TRUE;
}

gboolean plugin_unload(PurplePlugin *plugin)
{
	gint i;

	purple_debug_info("twitterim", "plugin_unload\n");

	tw_cmd_finalize(tw_cmd);
	tw_cmd = NULL;

	g_free(_mb_conf[TC_CONSUMER_KEY].def_str);
	g_free(_mb_conf[TC_CONSUMER_SECRET].def_str);
	g_free(_mb_conf[TC_REQUEST_TOKEN_URL].def_str);
	g_free(_mb_conf[TC_ACCESS_TOKEN_URL].def_str);
	g_free(_mb_conf[TC_AUTHORIZE_URL].def_str);

	g_free(_mb_conf[TC_HOST].def_str);
	g_free(_mb_conf[TC_STATUS_UPDATE].def_str);
	g_free(_mb_conf[TC_VERIFY_PATH].def_str);
	g_free(_mb_conf[TC_FRIENDS_TIMELINE].def_str);
	g_free(_mb_conf[TC_USER_TIMELINE].def_str);
	g_free(_mb_conf[TC_PUBLIC_TIMELINE].def_str);
	g_free(_mb_conf[TC_FRIENDS_USER].def_str);
	g_free(_mb_conf[TC_PUBLIC_USER].def_str);
	g_free(_mb_conf[TC_USER_USER].def_str);
	g_free(_mb_conf[TC_USER_GROUP].def_str);

	g_free(_mb_conf[TC_AUTH_TYPE].def_str);

	for(i = 0; i < TC_MAX; i++) {
		if(_mb_conf[i].conf) {
			g_free(_mb_conf[i].conf);
		}
	}

	g_free(_mb_conf);

	return TRUE;
}

const char * twitterim_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
	return "twitter";
}

GList * twitterim_actions(PurplePlugin *plugin, gpointer context)
{
	GList *m = NULL;
	PurplePluginAction *act;

	act = purple_plugin_action_new(_("Remove access credential (Oauth Token)"), twitterim_remove_oauth);
	m = g_list_append(m, act);

	return m;
}

void twitterim_remove_oauth(PurplePluginAction * action) {
	PurpleConnection * gc = action->context;

	purple_account_remove_setting(gc->account, _mb_conf[TC_OAUTH_TOKEN].conf);
	purple_account_remove_setting(gc->account, _mb_conf[TC_OAUTH_SECRET].conf);
	purple_notify_formatted(gc->account, _("Access credential removed"),
			_("Your access credential was removed"), NULL,
			_("Access credential will be requested again at the next log-in time"), NULL, NULL);
}

PurplePluginProtocolInfo twitter_prpl_info = {
	/* options */
	OPT_PROTO_UNIQUE_CHATNAME | OPT_PROTO_PASSWORD_OPTIONAL | OPT_PROTO_REGISTER_NOSCREENNAME,
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
	twitterim_list_icon,   /* list_icon */
	NULL,                   /* list_emblems */
	twitter_status_text, /* status_text */
//	twitterim_tooltip_text,/* tooltip_text */
	NULL,
	twitter_statuses,    /* status_types */
	NULL,                   /* blist_node_menu */
	NULL,                   /* chat_info */
	NULL,                   /* chat_info_defaults */
	twitter_login,       /* login */
	twitter_close,       /* close */
	twitter_send_im,     /* send_im */
	NULL,                   /* set_info */
//	twitterim_send_typing, /* send_typing */
	NULL,
//	twitterim_get_info,    /* get_info */
	NULL,
	twitter_set_status,/* set_status */
	NULL,                   /* set_idle */
	NULL,                   /* change_passwd */
//	twitterim_add_buddy,   /* add_buddy */
	NULL,
	NULL,                   /* add_buddies */
//	twitterim_remove_buddy,/* remove_buddy */
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
	"prpl-mbpurple-twitter", /* id */
	"TwitterIM", /* name */
	MBPURPLE_VERSION, /* version */
	"Twitter data feeder", /* summary */
	"Twitter data feeder", /* description */
	"Somsak Sriprayoonsakul <somsaks@gmail.com>", /* author */
	"http://microblog-purple.googlecode.com/", /* homepage */
	plugin_load, /* load */
	plugin_unload, /* unload */
	plugin_destroy, /* destroy */
	NULL, /* ui_info */
	&twitter_prpl_info, /* extra_info */
	NULL, /* prefs_info */
	twitterim_actions, /* actions */
	NULL, /* padding */
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(twitterim, plugin_init, info);
