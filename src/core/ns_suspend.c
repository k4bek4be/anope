/* NickServ core functions
 *
 * (C) 2003-2008 Anope Team
 * Contact us at info@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 * $Id$
 *
 */
/*************************************************************************/

#include "module.h"

int do_suspend(User * u);
int do_unsuspend(User * u);
void myNickServHelp(User * u);

class NSSuspend : public Module
{
 public:
	NSSuspend(const std::string &modname, const std::string &creator) : Module(modname, creator)
	{
		Command *c;

		this->SetAuthor("Anope");
		this->SetVersion("$Id$");
		this->SetType(CORE);

		c = createCommand("SUSPEND", do_suspend, is_services_oper, -1, -1, -1, NICK_SERVADMIN_HELP_SUSPEND, NICK_SERVADMIN_HELP_SUSPEND);
		moduleAddCommand(NICKSERV, c, MOD_UNIQUE);
		c = createCommand("UNSUSPEND", do_unsuspend, is_services_oper, -1, -1, -1, NICK_SERVADMIN_HELP_UNSUSPEND, NICK_SERVADMIN_HELP_UNSUSPEND);
		moduleAddCommand(NICKSERV, c, MOD_UNIQUE);

		moduleSetNickHelp(myNickServHelp);
	}
};

/**
 * Add the help response to anopes /ns help output.
 * @param u The user who is requesting help
 **/
void myNickServHelp(User * u)
{
    if (is_services_oper(u)) {
        notice_lang(s_NickServ, u, NICK_HELP_CMD_SUSPEND);
        notice_lang(s_NickServ, u, NICK_HELP_CMD_UNSUSPEND);
    }
}

/**
 * The /ns suspend command.
 * @param u The user who issued the command
 * @param MOD_CONT to continue processing other modules, MOD_STOP to stop processing.
 **/
int do_suspend(User * u)
{
    NickAlias *na, *na2;
    char *nick = strtok(NULL, " ");
    char *reason = strtok(NULL, "");
    int i;

    if (!nick || !reason) {
        syntax_error(s_NickServ, u, "SUSPEND", NICK_SUSPEND_SYNTAX);
        return MOD_CONT;
    }

    if (readonly) {
        notice_lang(s_NickServ, u, READ_ONLY_MODE);
        return MOD_CONT;
    }

    if ((na = findnick(nick)) == NULL) {
        notice_lang(s_NickServ, u, NICK_X_NOT_REGISTERED, nick);
        return MOD_CONT;
    }

    if (na->status & NS_VERBOTEN) {
        notice_lang(s_NickServ, u, NICK_X_FORBIDDEN, na->nick);
        return MOD_CONT;
    }

    if (NSSecureAdmins && nick_is_services_admin(na->nc)
        && !is_services_root(u)) {
        notice_lang(s_NickServ, u, PERMISSION_DENIED);
        return MOD_CONT;
    }

    if (na) {
        na->nc->flags |= NI_SUSPENDED;
        na->nc->flags |= NI_SECURE;
        na->nc->flags &= ~(NI_KILLPROTECT | NI_KILL_QUICK | NI_KILL_IMMED);

        for (i = 0; i < na->nc->aliases.count; i++) {
            na2 = (NickAlias *)na->nc->aliases.list[i];
            if (na2->nc == na->nc) {
                na2->status &= ~(NS_IDENTIFIED | NS_RECOGNIZED);
                na2->last_quit = sstrdup(reason);
            }
        }

        if (WallForbid)
            ircdproto->SendGlobops(s_NickServ, "\2%s\2 used SUSPEND on \2%s\2",
                             u->nick, nick);

        alog("%s: %s set SUSPEND for nick %s", s_NickServ, u->nick, nick);
        notice_lang(s_NickServ, u, NICK_SUSPEND_SUCCEEDED, nick);
        send_event(EVENT_NICK_SUSPENDED, 1, nick);

    } else {

        alog("%s: Valid SUSPEND for %s by %s failed", s_NickServ, nick,
             u->nick);
        notice_lang(s_NickServ, u, NICK_SUSPEND_FAILED, nick);

    }
    return MOD_CONT;
}

/*************************************************************************/

int do_unsuspend(User * u)
{
    NickAlias *na;
    char *nick = strtok(NULL, " ");

    if (!nick) {
        syntax_error(s_NickServ, u, "UNSUSPEND", NICK_UNSUSPEND_SYNTAX);
        return MOD_CONT;
    }

    if (readonly) {
        notice_lang(s_NickServ, u, READ_ONLY_MODE);
        return MOD_CONT;
    }

    if ((na = findnick(nick)) == NULL) {
        notice_lang(s_NickServ, u, NICK_X_NOT_REGISTERED, nick);
        return MOD_CONT;
    }

    if (na->status & NS_VERBOTEN) {
        notice_lang(s_NickServ, u, NICK_X_FORBIDDEN, na->nick);
        return MOD_CONT;
    }
    if (NSSecureAdmins && nick_is_services_admin(na->nc)
        && !is_services_root(u)) {
        notice_lang(s_NickServ, u, PERMISSION_DENIED);
        return MOD_CONT;
    }

    if (na) {
        na->nc->flags &= ~NI_SUSPENDED;

        if (WallForbid)
            ircdproto->SendGlobops(s_NickServ, "\2%s\2 used UNSUSPEND on \2%s\2",
                             u->nick, nick);

        alog("%s: %s set UNSUSPEND for nick %s", s_NickServ, u->nick,
             nick);
        notice_lang(s_NickServ, u, NICK_UNSUSPEND_SUCCEEDED, nick);
        send_event(EVENT_NICK_UNSUSPEND, 1, nick);

    } else {

        alog("%s: Valid UNSUSPEND for %s by %s failed", s_NickServ, nick,
             u->nick);
        notice_lang(s_NickServ, u, NICK_UNSUSPEND_FAILED, nick);

    }

    return MOD_CONT;

}

MODULE_INIT("ns_suspend", NSSuspend)
