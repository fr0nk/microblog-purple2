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
#include <notify.h>
#include <version.h>

#ifdef _WIN32
#	include <win32dep.h>
#else
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif



static void plugin_init(PurplePlugin *plugin)
{
}

gboolean plugin_load(PurplePlugin *plugin)
{
	return TRUE;
}

gboolean plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static const char * dummy_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
	return "twitter";
}

static void dummy_login(PurpleAccount *acct)
{
	purple_debug_info("dummytw", "dummy_login\n");
	purple_notify_info(acct->gc, "Attention please", "Please re-add Twitter protocol",
			"Twitter plug-in from Microblog-Purple has been undergo important upgrades,which makes it incompatible with the old plug-in\n"
			"\n"
			"To upgrade, please REMOVE \"OldTwitterIM\", then RE-ADD \"TwitterIM\" back\n"
			"\n"
			"Sorry for any inconvenience, and enjoy your twitter-er experience"
	);
}

static void dummy_close(PurpleConnection *gc)
{
}


static void dummy_buddy_free(PurpleBuddy * buddy)
{
}

static gchar * dummy_status_text(PurpleBuddy *buddy)
{
	return NULL;
}

static void dummy_set_status(PurpleAccount *acct, PurpleStatus *status)
{
}


static GList * dummy_statuses(PurpleAccount *acct)
{
	GList *types = NULL;
	PurpleStatusType *status;
	
	//Online people have a status message and also a date when it was set	
	status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, _("Online"), TRUE, TRUE, FALSE);
	types = g_list_append(types, status);
	
	//Offline people dont have messages
	status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, NULL, _("Offline"), TRUE, TRUE, FALSE);
	types = g_list_append(types, status);
	
	return types;

}

static PurplePluginProtocolInfo dummy_prpl_info = {
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
	dummy_list_icon,   /* list_icon */
	NULL,                   /* list_emblems */
	dummy_status_text, /* status_text */
	NULL,
	dummy_statuses,    /* status_types */
	NULL,                   /* blist_node_menu */
	NULL,                   /* chat_info */
	NULL,                   /* chat_info_defaults */
	dummy_login,       		/* login */
	dummy_close,       		/* close */
	NULL,     		/* send_im */
	NULL,                   /* set_info */
	NULL,
	NULL,
	dummy_set_status,/* set_status */
	NULL,                   /* set_idle */
	NULL,                   /* change_passwd */
	NULL,
	NULL,                   /* add_buddies */
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
	dummy_buddy_free,			/* buddy_free */
	NULL,                   /* convo_closed */
	purple_normalize_nocase,			/* normalize */
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

static GList * dummy_actions(PurplePlugin *plugin, gpointer context)
{
	return NULL;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL, /* type */
	NULL, /* ui_requirement */
	0, /* flags */
	NULL, /* dependencies */
	PURPLE_PRIORITY_DEFAULT, /* priority */
	"prpl-somsaks-twitter", /* id */
	"OldTwitterIM", /* name */
	"0.1.3", /* version */
	"Twitter data feeder", /* summary */
	"Twitter data feeder", /* description */
	"Somsak Sriprayoonsakul <somsaks@gmail.com>", /* author */
	"http://microblog-purple.googlecode.com/", /* homepage */
	plugin_load, /* load */
	plugin_unload, /* unload */
	NULL, /* destroy */
	NULL, /* ui_info */
	&dummy_prpl_info, /* extra_info */
	NULL, /* prefs_info */
	dummy_actions, /* actions */
	NULL, /* padding */
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(dummy, plugin_init, info);
