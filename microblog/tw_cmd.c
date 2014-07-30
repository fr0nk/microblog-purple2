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
 * Command support for twitter
 */

#include <time.h>
#include <debug.h>
#include "tw_cmd.h"

#define DBGID "tw_cmd"

typedef struct {
	const gchar * cmd;
	const gchar * args;
	PurpleCmdPriority prio;
	PurpleCmdFlag flag;
	TwCmdFunc func;
	void * data;
	const gchar * help;
} TwCmdEnum ;

static PurpleCmdRet tw_cmd_replies(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);
static PurpleCmdRet tw_cmd_refresh(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);
static PurpleCmdRet tw_cmd_refresh_rate(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);
static PurpleCmdRet tw_cmd_caller(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, void * data);
static PurpleCmdRet tw_cmd_tag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);
static PurpleCmdRet tw_cmd_btag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);
static PurpleCmdRet tw_cmd_untag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);
static PurpleCmdRet tw_cmd_set_tag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data, gint position);
static PurpleCmdRet tw_cmd_get_user_tweets(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data);

static TwCmdEnum tw_cmd_enum[] = {
	{"replies", "", PURPLE_CMD_P_PRPL, 0, tw_cmd_replies, NULL,
		"get replies timeline."},
	{"refresh", "", PURPLE_CMD_P_PRPL, 0, tw_cmd_refresh, NULL,
		"refresh tweets now."},
	{"refresh_rate", "w", PURPLE_CMD_P_PRPL, 0, tw_cmd_refresh_rate, NULL,
		"set refresh rate. /refreshrate 120 temporariy change refresh rate to 120 seconds"},
	{"tag", "w", PURPLE_CMD_P_PRPL, 0, tw_cmd_tag, NULL,
		"prepend everything with a specified tag, unset with /untag"},
	{"btag", "w", PURPLE_CMD_P_PRPL, 0, tw_cmd_btag, NULL,
		"append everything with a specified tag, unset with /untag"},
	{"untag", "", PURPLE_CMD_P_PRPL, 0, tw_cmd_untag, NULL,
		"unset already set tag"},
	{"get", "w", PURPLE_CMD_P_PRPL, 0, tw_cmd_get_user_tweets, NULL,
		"get specific user timeline. Use /get <screen_name> to fetch."},
};

PurpleCmdRet tw_cmd_tag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	return tw_cmd_set_tag(conv, cmd, args, error, data, MB_TAG_PREFIX);
}

PurpleCmdRet tw_cmd_btag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	return tw_cmd_set_tag(conv, cmd, args, error, data, MB_TAG_POSTFIX);
}

PurpleCmdRet tw_cmd_set_tag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data, gint position)
{
	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	if(data->ma->tag) {
		g_free(data->ma->tag);
	}
	data->ma->tag = g_strdup(args[0]);
	data->ma->tag_pos = position;

	return PURPLE_CMD_RET_OK;
}

PurpleCmdRet tw_cmd_untag(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	MbAccount * ma = data->ma;

	if(ma->tag) {
		g_free(ma->tag);
		ma->tag = NULL;
		ma->tag_pos = MB_TAG_NONE;
	} else {
		serv_got_im(ma->gc, mc_def(TC_FRIENDS_USER), _("no tag is being set"), PURPLE_MESSAGE_SYSTEM, time(NULL));
	}

	return PURPLE_CMD_RET_OK;
}

PurpleCmdRet tw_cmd_replies(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	const gchar * path;
	TwitterTimeLineReq * tlr;
	int count;
	time_t now;
	MbAccount * ma = data->ma;

	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	path = purple_account_get_string(ma->account, mc_name(TC_REPLIES_TIMELINE), mc_def(TC_REPLIES_TIMELINE));
	count = 20; //< FIXME: Hardcoded number of count
	tlr = twitter_new_tlr(path, mc_def(TC_REPLIES_USER), TL_REPLIES, count, _("end reply messages"));
	tlr->use_since_id = FALSE;
	time(&now);
	serv_got_im(ma->gc, tlr->name, _("getting reply messages"), PURPLE_MESSAGE_SYSTEM, now);
	twitter_fetch_new_messages(ma, tlr);

	return PURPLE_CMD_RET_OK;
}

PurpleCmdRet tw_cmd_refresh_rate(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	char * end_ptr = NULL;
	int new_rate = -1;
	MbAccount * ma = data->ma;

	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);
	new_rate = (int)strtol(args[0], &end_ptr, 10);
	if( (*end_ptr) == '\0' ) {
		if(new_rate > 10) {
			purple_account_set_int(ma->account, mc_name(TC_MSG_REFRESH_RATE), new_rate);
			return PURPLE_CMD_RET_OK;
		} else {
			serv_got_im(ma->gc, mc_def(TC_FRIENDS_USER), _("new rate is too low, must be > 10 seconds"), PURPLE_MESSAGE_SYSTEM, time(NULL));
			return PURPLE_CMD_RET_FAILED;
		}
	}
	return PURPLE_CMD_RET_FAILED;
}

PurpleCmdRet tw_cmd_refresh(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	twitter_fetch_all_new_messages(data->ma);
	return PURPLE_CMD_RET_OK;
}

PurpleCmdRet tw_cmd_get_user_tweets(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, TwCmdArg * data)
{
	const gchar * path;
	TwitterTimeLineReq * tlr;
	int count;
	time_t now;
	MbAccount * ma = data->ma;

	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);

	path = purple_account_get_string(ma->account, mc_name(TC_USER_TIMELINE), mc_def(TC_USER_TIMELINE));
	count = 20; //< FIXME: Hardcoded number of count

	// This will spawn another timeline
	//tlr = twitter_new_tlr(path, mc_def(TC_USER_USER), TL_USER, count, _("end user messages"));

	// This will fetch user tweets into the same timeline
	tlr = twitter_new_tlr(path, mc_def(TC_REPLIES_USER), TL_REPLIES, count, _("end user messages"));
	tlr->use_since_id = FALSE;
	tlr->screen_name = args[0];
	time(&now);
	serv_got_im(ma->gc, tlr->name, _("getting user messages"), PURPLE_MESSAGE_SYSTEM, now);
	twitter_fetch_new_messages(ma, tlr);

	return PURPLE_CMD_RET_OK;
}

/*
 * Convenient proxy for calling real function
 */
PurpleCmdRet tw_cmd_caller(PurpleConversation * conv, const gchar * cmd, gchar ** args, gchar ** error, void * data)
{
	TwCmdArg * tca = data;

	purple_debug_info(DBGID, "%s called for cmd = %s\n", __FUNCTION__, cmd);
	tca->ma = conv->account->gc->proto_data;
	return tca->func(conv, cmd, args, error, tca);
}

TwCmd * tw_cmd_init(const char * protocol_id)
{
	int i, len = sizeof(tw_cmd_enum)/sizeof(TwCmdEnum);
	TwCmd * tw_cmd;
	
	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);
	tw_cmd = g_new(TwCmd, 1);
	tw_cmd->protocol_id = g_strdup(protocol_id);
	tw_cmd->cmd_id_num = len;
	tw_cmd->cmd_args = g_malloc0(sizeof(TwCmdArg *) * tw_cmd->cmd_id_num);
	tw_cmd->cmd_id = g_malloc(sizeof(PurpleCmdId) * tw_cmd->cmd_id_num);
	for(i = 0; i < len; i++) {
		tw_cmd->cmd_args[i] = g_new0(TwCmdArg, 1);
		tw_cmd->cmd_args[i]->func = tw_cmd_enum[i].func;
		tw_cmd->cmd_args[i]->data = tw_cmd_enum[i].data;
		tw_cmd->cmd_id[i] = purple_cmd_register(tw_cmd_enum[i].cmd,
				tw_cmd_enum[i].args,
				tw_cmd_enum[i].prio,
				tw_cmd_enum[i].flag | PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
				protocol_id,
				tw_cmd_caller,
				tw_cmd_enum[i].help,
				tw_cmd->cmd_args[i]
		);
		purple_debug_info(DBGID, "command %s registered\n", tw_cmd_enum[i].cmd);
	}
	return tw_cmd;
}

void tw_cmd_finalize(TwCmd * tc)
{
	int i;

	purple_debug_info(DBGID, "%s called\n", __FUNCTION__);
	for(i = 0; i < tc->cmd_id_num; i++) {
		purple_cmd_unregister(tc->cmd_id[i]);
		g_free(tc->cmd_args[i]);
	}
	g_free(tc->protocol_id);
	g_free(tc);
}
