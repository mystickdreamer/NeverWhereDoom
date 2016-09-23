/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/interpreter.c,v 1.13 2005/01/05 16:27:27 fnord Exp $");

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "tedit.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "shop.h"
#include "guild.h"

/* local global variables */
DISABLED_DATA *disabled_first = NULL;

/* external variables */
extern char *motd;
extern char *imotd;
extern char *background;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int selfdelete_fastwipe;
extern int xap_objs;
extern char *GREETANSI;
extern char *GREETINGS;
extern char *ANSIQUESTION;
extern const char compress_offer[4];

/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
int parse_class(struct char_data *ch, int arg);
int parse_race(struct char_data *ch, int arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);
void read_saved_vars(struct char_data *ch);
void remove_player(int pfilepos);
void assemblies_parse(struct descriptor_data *d, char *arg);
extern void assedit_parse(struct descriptor_data *d, char *arg);
int class_ok_race[NUM_RACES][NUM_CLASSES];
int race_ok_gender[NUM_SEX][NUM_RACES];
void gedit_disp_menu(struct descriptor_data *d);
void gedit_parse(struct descriptor_data *d, char *arg);
void cedit_creation(struct char_data *ch);

/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
int reserved_word(char *argument);
int _parse_name(char *arg, char *name);
int check_disabled(const struct command_info *command);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_advance);
ACMD(do_aedit);
ACMD(do_alias);
ACMD(do_assemble);
ACMD(do_assedit);
ACMD(do_assist);
ACMD(do_astat);
ACMD(do_at);
ACMD(do_autoexit);
ACMD(do_ban);
ACMD(do_boom);
ACMD(do_break);
ACMD(do_cast);
ACMD(do_checkloadstatus);
ACMD(do_chown);
ACMD(do_color);
ACMD(do_compare);
ACMD(do_copyover);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_disable);
ACMD(do_dig);
ACMD(do_disarm);
ACMD(do_dismount);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drive);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_feats);
ACMD(do_file);
ACMD(do_finddoor);
ACMD(do_findkey);
ACMD(do_fix);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_heal);
ACMD(do_help);
ACMD(do_hindex);
ACMD(do_history);
ACMD(do_helpcheck);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_iedit);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kill);
ACMD(do_languages);
ACMD(do_lay_hands);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
ACMD(do_map);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_mount);
ACMD(do_not_here);
ACMD(do_memorize);
ACMD(do_oasis_copy);
ACMD(do_oasis);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pagelength);
ACMD(do_peace);
ACMD(do_players);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_raise);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_respond);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_resurrect);
ACMD(do_return);
ACMD(do_room_copy);
ACMD(do_sac);
ACMD(do_save);
ACMD(do_saveall);
ACMD(do_say);
ACMD(do_score);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_show_save_list);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_spells);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_stunning);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tame);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_turn);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);
ACMD(do_wield);
ACMD(do_value);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizupdate);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zcheck);
ACMD(do_zreset);
ACMD(do_zpurge);

/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mjunk);
ACMD(do_mdoor);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mdamage);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mhunt);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mzoneecho);
ACMD(do_vdelete);
ACMD(do_mfollow);
ACMD(do_dig);

struct command_info *complete_cmd_info;

/* This is the Master Command List(tm).
 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority. */

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", "", 0, 0, 0, ADMLVL_NONE	, 0 },     /* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NORTH },
  { "east"     , "e"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_EAST },
  { "south"    , "s"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SOUTH },
  { "west"     , "w"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_WEST },
  { "up"       , "u"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_UP },
  { "down"     , "d"       , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_DOWN },
  { "northwest", "northw"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "nw"       , "nw"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "northeast", "northe"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "ne"       , "ne"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "southeast", "southe"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "se"       , "se"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "southwest", "southw"  , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },
  { "sw"       , "sw"      , POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },

  /* now, the main list */
  { "art"      , "a"		, POS_RESTING , do_cast     , 1, ADMLVL_NONE	, SCMD_ART },
  { "at"       , "at"		, POS_DEAD    , do_at       , 0, ADMLVL_GOD	, 0 },
  { "advance"  , "adv"		, POS_DEAD    , do_advance  , 0, ADMLVL_IMPL	, 0 },
  { "aedit"    , "aed"	 	, POS_DEAD    , do_oasis    , 0, ADMLVL_GOD	, SCMD_OASIS_AEDIT },
  { "alias"    , "ali"		, POS_DEAD    , do_alias    , 0, ADMLVL_NONE	, 0 },
  { "afk"      , "afk"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AFK },
  { "assist"   , "ass"		, POS_FIGHTING, do_assist   , 1, ADMLVL_NONE	, 0 },
  { "assemble" , "asse"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_ASSEMBLE },
  { "assedit"  , "assed"	, POS_STANDING, do_assedit  , 0, ADMLVL_IMPL	, 0},
  { "astat"    , "ast"		, POS_DEAD    , do_astat    , 0, ADMLVL_GOD	, 0 },
  { "ask"      , "ask"		, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_ASK },
  { "auction"  , "auc"		, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_AUCTION },
  { "autoassist", "autoass"     , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOASSIST },
  { "autoexit" , "autoex"	, POS_DEAD    , do_autoexit , 0, ADMLVL_NONE	, 0 },
  { "autogold" , "autogo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOGOLD },
  { "autoloot" , "autolo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOLOOT },
  { "automap"  , "automa"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_AUTOMAP },
  { "automem"  , "autome"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_AUTOMEM },
  { "autosac"  , "autosa"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOSAC },
  { "autosplit", "autosp"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOSPLIT },

  { "backstab" , "ba"		, POS_FIGHTING, do_kill	    , 0, ADMLVL_NONE	, 0 }, /* part of normal fighting */
  { "bake"     , "bak"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_BAKE },
  { "ban"      , "ban"		, POS_DEAD    , do_ban      , 0, ADMLVL_GRGOD	, 0 },
  { "balance"  , "bal"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "buildwalk", "buildwalk"	, POS_STANDING, do_gen_tog, 0, ADMLVL_BUILDER	, SCMD_BUILDWALK },
  { "boom"     , "bo"	        , POS_STANDING, do_boom     , 0, ADMLVL_IMPL	, 0 },
  { "break"    , "break"	, POS_STANDING, do_break    , 0, ADMLVL_IMMORT	, 0 },
  { "brew"     , "brew"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_BREW },
  { "brief"    , "br"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_BRIEF },
  { "buy"      , "bu"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "bug"      , "bug"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_BUG },

  { "cast"     , "c"		, POS_SITTING , do_cast     , 1, ADMLVL_NONE	, SCMD_CAST },
  { "cedit"    , "cedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMPL	, SCMD_OASIS_CEDIT },
  { "check"    , "ch"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "checkload", "checkl"	, POS_DEAD    , do_checkloadstatus, 0, ADMLVL_GOD, 0 },
  { "chown"    , "cho"		, POS_DEAD    , do_chown    , 1, ADMLVL_GRGOD	, 0 },
  { "clear"    , "cle"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "close"    , "cl"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_CLOSE },
  { "cls"      , "cls"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "clsolc"   , "clsolc"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_BUILDER	, SCMD_CLS },
  { "consider" , "con"		, POS_RESTING , do_consider , 0, ADMLVL_NONE	, 0 },
  { "color"    , "col"		, POS_DEAD    , do_color    , 0, ADMLVL_NONE	, 0 },
  { "compare"  , "comp"		, POS_RESTING , do_compare  , 0, ADMLVL_NONE	, 0 },
  { "commands" , "com"		, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_COMMANDS },
  { "compact"  , "compact"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_COMPACT },
  { "copyover" , "copyover"	, POS_DEAD    , do_copyover , 0, ADMLVL_GRGOD	, 0 },
  { "craft"    , "craft"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_CRAFT },
  { "credits"  , "cred"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CREDITS },

  { "date"     , "da"		, POS_DEAD    , do_date     , 0, ADMLVL_IMMORT	, SCMD_DATE },
  { "dc"       , "dc"		, POS_DEAD    , do_dc       , 0, ADMLVL_GOD	, 0 },
  { "deposit"  , "depo"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "diagnose" , "diag"		, POS_RESTING , do_diagnose , 0, ADMLVL_NONE	, 0 },
  { "disable"  , "disa"		, POS_DEAD    , do_disable  , 0, ADMLVL_GRGOD	, 0 },
  { "dig"      , "dig"		, POS_DEAD    , do_dig      , 0, ADMLVL_BUILDER	, 0 },
  { "dismount" , "dismo"        , POS_STANDING, do_dismount , 0, ADMLVL_NONE    , 0 },
  { "display"  , "disp"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "donate"   , "don"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DONATE },
  { "drink"    , "dri"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_DRINK },
  { "drive"    , "drive"	, POS_STANDING, do_drive    , 0, ADMLVL_NONE	, 0 },
  { "drop"     , "dro"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DROP },

  { "eat"      , "ea"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_EAT },
  { "echo"     , "ec"		, POS_SLEEPING, do_echo     , 0, ADMLVL_GOD	, SCMD_ECHO },
  { "emote"    , "em"		, POS_RESTING , do_echo     , 1, ADMLVL_NONE	, SCMD_EMOTE },
  { ":"        , ":"		, POS_RESTING, do_echo      , 1, ADMLVL_NONE	, SCMD_EMOTE },
  { "enter"    , "ent"		, POS_STANDING, do_enter    , 0, ADMLVL_NONE	, 0 },
  { "equipment", "eq"		, POS_SLEEPING, do_equipment, 0, ADMLVL_NONE	, 0 },
  { "exits"    , "ex"		, POS_RESTING , do_exits    , 0, ADMLVL_NONE	, 0 },
  { "examine"  , "exa"		, POS_SITTING , do_examine  , 0, ADMLVL_NONE	, 0 },

  { "feats"    , "fea"		, POS_RESTING, do_feats     , 0, ADMLVL_NONE	, 0 },
  { "force"    , "force"	, POS_SLEEPING, do_force    , 0, ADMLVL_GOD	, 0 },
  { "forget"   , "forget"	, POS_RESTING, do_memorize  , 1, ADMLVL_NONE	, SCMD_FORGET },
  { "fill"     , "fil"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_FILL },
  { "file"     , "fi"		, POS_SLEEPING, do_file     , 0, ADMLVL_GRGOD	, 0 },
  { "finddoor" , "findd"	, POS_SLEEPING, do_finddoor , 0, ADMLVL_IMMORT, 0 },
  { "findkey"  , "findk"	, POS_SLEEPING, do_findkey  , 0, ADMLVL_IMMORT, 0 },
  { "fix"      , "fix"		, POS_STANDING, do_fix      , 0, ADMLVL_IMMORT	, 0 },
  { "flee"     , "fl"		, POS_FIGHTING, do_flee     , 1, ADMLVL_NONE	, 0 },
  { "fletch"   , "fletch"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_FLETCH },
  { "follow"   , "fol"		, POS_RESTING , do_follow   , 0, ADMLVL_NONE	, 0 },
  { "forge"    , "for"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_FORGE },
  { "freeze"   , "freeze"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_FREEZE	, SCMD_FREEZE },

  { "gain"     , "ga"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "get"      , "get"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "gecho"    , "gecho"	, POS_DEAD    , do_gecho    , 0, ADMLVL_GOD	, 0 },
  { "gedit"    , "gedit"      	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_GEDIT },
  { "gemote"   , "gem"	 	, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GEMOTE },
  { "glist"    , "glist"	, POS_SLEEPING, do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_GLIST },
  { "give"     , "giv"		, POS_RESTING , do_give     , 0, ADMLVL_NONE	, 0 },
  { "goto"     , "go"		, POS_SLEEPING, do_goto     , 0, ADMLVL_IMMORT	, 0 },
  { "gold"     , "gol"		, POS_RESTING , do_gold     , 0, ADMLVL_NONE	, 0 },
  { "gossip"   , "gos"		, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GOSSIP },
  { "group"    , "gro"		, POS_RESTING , do_group    , 1, ADMLVL_NONE	, 0 },
  { "grab"     , "grab"		, POS_RESTING , do_grab     , 0, ADMLVL_NONE	, 0 },
  { "grats"    , "grat"		, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GRATZ },
  { "gsay"     , "gsay"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },
  { "gtell"    , "gt"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },

  { "heal"     , "he"           , POS_STANDING, do_heal     , 0, ADMLVL_NONE    , 0 },
  { "help"     , "h"            , POS_DEAD    , do_help     , 0, ADMLVL_NONE    , 0 },
  { "hedit"    , "hedit"        , POS_DEAD    , do_oasis    , 0, ADMLVL_GOD     , SCMD_OASIS_HEDIT },
  { "hindex"   , "hind"         , POS_DEAD    , do_hindex   , 0, ADMLVL_NONE    , 0 },
  { "helpcheck", "helpch"       , POS_DEAD    , do_helpcheck, 0, ADMLVL_NONE    , 0 },
  { "handbook" , "handb"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_IMMORT	, SCMD_HANDBOOK },
  { "hcontrol" , "hcontrol"	, POS_DEAD    , do_hcontrol , 0, ADMLVL_GRGOD	, 0 },
  { "hide"     , "hide"		, POS_RESTING , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_HIDE },
  { "hit"      , "hit"		, POS_FIGHTING, do_hit      , 0, ADMLVL_NONE	, SCMD_HIT },
  { "history"  , "hist"         , POS_DEAD    , do_history  , 0, ADMLVL_NONE    , 0 },
  { "hold"     , "hold"		, POS_RESTING , do_grab     , 1, ADMLVL_NONE	, 0 },
  { "holler"   , "holler"	, POS_RESTING , do_gen_comm , 1, ADMLVL_NONE	, SCMD_HOLLER },
  { "holylight", "holy"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_HOLYLIGHT },
  { "house"    , "house"	, POS_RESTING , do_house    , 0, ADMLVL_NONE	, 0 },

  { "i"        , "i"		, POS_DEAD    , do_inventory, 0, ADMLVL_NONE	, 0 },
  { "idea"     , "id"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_IDEA },
  { "imotd"    , "imotd"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_IMMORT	, SCMD_IMOTD },
  { "immlist"  , "imm"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_IMMLIST },
  { "inside"   , "in"		, POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_IN },
  { "info"     , "info"		, POS_SLEEPING, do_gen_ps   , 0, ADMLVL_NONE	, SCMD_INFO },
  { "insult"   , "insult"	, POS_RESTING , do_insult   , 0, ADMLVL_NONE	, 0 },
  { "inventory", "inv"		, POS_DEAD    , do_inventory, 0, ADMLVL_NONE	, 0 },
  { "iedit"    , "ie"   	, POS_DEAD    , do_iedit    , 0, ADMLVL_GRGOD	, 0 },
  { "invis"    , "invi"		, POS_DEAD    , do_invis    , 0, ADMLVL_IMMORT	, 0 },

  { "junk"     , "junk"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_JUNK },

  { "kill"     , "k"		, POS_FIGHTING, do_kill     , 0, ADMLVL_NONE	, 0 },
  { "knit"     , "knit"		, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_KNIT },

  { "look"     , "l"		, POS_RESTING , do_look     , 0, ADMLVL_NONE	, SCMD_LOOK },
  { "languages", "lang"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "last"     , "last"		, POS_DEAD    , do_last     , 0, ADMLVL_GOD	, 0 },
  { "lay"      , "lay"		, POS_FIGHTING, do_lay_hands, 1, ADMLVL_NONE	, 0 },
  { "learn"    , "lear"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "leave"    , "lea"		, POS_STANDING, do_leave    , 0, ADMLVL_NONE	, 0 },
  { "levels"   , "lev"		, POS_DEAD    , do_levels   , 0, ADMLVL_NONE	, 0 },
  { "list"     , "lis"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "links"    , "lin"		, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_LINKS },
  { "lock"     , "loc"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_LOCK },
  { "load"     , "load"		, POS_DEAD    , do_load     , 0, ADMLVL_GOD	, 0 },

  { "motd"     , "motd"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_MOTD },
  { "mail"     , "mail"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "make"     , "make"		, POS_STANDING, do_assemble , 0, ADMLVL_NONE	, SCMD_MAKE },
  { "map"      , "map"		, POS_STANDING, do_map      , 0, ADMLVL_NONE	, 0 },
  { "mix"      , "mix"		, POS_STANDING, do_assemble , 0, ADMLVL_NONE	, SCMD_MIX },
  { "mcopy"    , "mcopy"	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_MEDIT },
  { "medit"    , "medit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_MEDIT },
  { "memorize" , "mem"		, POS_RESTING , do_memorize , 1, ADMLVL_NONE	, SCMD_MEMORIZE },
  { "memwhen"  , "memwhe"    	, POS_RESTING , do_memorize , 0, ADMLVL_IMMORT	, SCMD_WHEN_SLOT },
  { "mlist"    , "mlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_MLIST },
  { "mount"    , "mount"	, POS_STANDING, do_mount    , 0, ADMLVL_NONE, 0 },
  { "mute"     , "mute"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_SQUELCH },
  { "murder"   , "murder"	, POS_FIGHTING, do_hit      , 0, ADMLVL_NONE	, SCMD_MURDER },

  { "news"     , "news"		, POS_SLEEPING, do_gen_ps   , 0, ADMLVL_NONE	, SCMD_NEWS },
  { "noauction", "noauction"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOAUCTION },
  { "nocompress","nocompress"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOCOMPRESS },
  { "nogossip" , "nogossip"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGOSSIP },
  { "nograts"  , "nograts"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGRATZ },
  { "nohassle" , "nohassle"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_NOHASSLE },
  { "norepeat" , "norepeat"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOREPEAT },
  { "noshout"  , "noshout"	, POS_SLEEPING, do_gen_tog  , 1, ADMLVL_NONE	, SCMD_DEAF },
  { "nosummon" , "nosummon"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_NOSUMMON },
  { "notell"   , "notell"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_NOTELL },
  { "notitle"  , "notitle"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_NOTITLE },
  { "nowiz"    , "nowiz"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_NOWIZ },

  { "ocopy"    , "ocopy"	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_OEDIT },
  { "order"    , "ord"		, POS_RESTING , do_order    , 1, ADMLVL_NONE	, 0 },
  { "offer"    , "off"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "open"     , "ope"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_OPEN },
  { "olc"      , "olc"		, POS_DEAD    , do_show_save_list, 0, ADMLVL_BUILDER, 0 },
  { "olist"    , "olist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_OLIST },
  { "oedit"    , "oedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_OEDIT },
  { "outside"  , "out"     	, POS_STANDING, do_move     , 0, ADMLVL_NONE	, SCMD_OUT },

  { "put"      , "put"		, POS_RESTING , do_put      , 0, ADMLVL_NONE	, 0 },
  { "page"     , "pag"		, POS_DEAD    , do_page     , 0, ADMLVL_GOD	, 0 },
  { "pardon"   , "pardon"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_PARDON },
  { "pagelength", "pagel"	, POS_DEAD    , do_pagelength, 0, 0, 0 },
  { "peace"    , "pea"		, POS_DEAD    , do_peace    , 0, ADMLVL_GRGOD	, 0 },
  { "pick"     , "pi"		, POS_STANDING, do_gen_door , 1, ADMLVL_NONE	, SCMD_PICK },
  { "pilot"    , "pilot"	, POS_STANDING, do_drive    , 0, ADMLVL_NONE	, 0 },
  { "players"  , "play"		, POS_DEAD    , do_players  , 0, ADMLVL_GOD	, 0 },
  { "policy"   , "pol"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_POLICIES },
  { "poofin"   , "poofi"	, POS_DEAD    , do_poofset  , 0, ADMLVL_IMMORT	, SCMD_POOFIN },
  { "poofout"  , "poofo"	, POS_DEAD    , do_poofset  , 0, ADMLVL_IMMORT	, SCMD_POOFOUT },
  { "pour"     , "pour"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_POUR },
  { "powerattack", "pow"	, POS_DEAD    , do_value    , 0, ADMLVL_NONE	, SCMD_POWERATT },
  { "prompt"   , "pro"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "practice" , "pra"		, POS_RESTING , do_practice , 1, ADMLVL_NONE	, 0 },
  { "purge"    , "purge"	, POS_DEAD    , do_purge    , 0, ADMLVL_GOD	, 0 },

  { "quaff"    , "qua"		, POS_RESTING , do_use      , 0, ADMLVL_NONE	, SCMD_QUAFF },
  { "qecho"    , "que"		, POS_DEAD    , do_qcomm    , 0, ADMLVL_GOD	, SCMD_QECHO },
  { "quest"    , "que"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_QUEST },
  { "qui"      , "qui"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, 0 },
  { "quit"     , "quit"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, SCMD_QUIT },
  { "qsay"     , "qsay"		, POS_RESTING , do_qcomm    , 0, ADMLVL_NONE	, SCMD_QSAY },

  { "raise"    , "rai"		, POS_STANDING, do_raise    , 0, ADMLVL_GOD	, 0 },
  { "reply"    , "rep"		, POS_SLEEPING, do_reply    , 0, ADMLVL_NONE	, 0 },
  { "rest"     , "re"		, POS_RESTING , do_rest     , 0, ADMLVL_NONE	, 0 },
  { "read"     , "rea"		, POS_RESTING , do_look     , 0, ADMLVL_NONE	, SCMD_READ },
  { "reload"   , "reload"	, POS_DEAD    , do_reboot   , 0, ADMLVL_IMPL	, 0 },
  { "recite"   , "reci"		, POS_RESTING , do_use      , 0, ADMLVL_NONE	, SCMD_RECITE },
  { "receive"  , "rece"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "remove"   , "rem"		, POS_RESTING , do_remove   , 0, ADMLVL_NONE	, 0 },
  { "rent"     , "rent"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "report"   , "repo"		, POS_RESTING , do_report   , 0, ADMLVL_NONE	, 0 },
  { "reroll"   , "rero"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_GRGOD	, SCMD_REROLL },
  { "respond"  , "resp" 	, POS_RESTING,  do_respond  , 1, ADMLVL_NONE	, 0 },
  { "restore"  , "resto"	, POS_DEAD    , do_restore  , 0, ADMLVL_GOD	, 0 },
  { "resurrect", "resu"		, POS_DEAD    , do_resurrect, 0, ADMLVL_NONE	, 0 },
  { "return"   , "retu"		, POS_DEAD    , do_return   , 0, ADMLVL_NONE	, 0 },
  { "redit"    , "redit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_REDIT },
  { "rlist"    , "rlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_RLIST },
  { "roomflags", "roomf"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_ROOMFLAGS },
  { "rcopy"    , "rcopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_REDIT },

  { "sacrifice", "sac"		, POS_SLEEPING, do_sac      , 0, ADMLVL_NONE	, 0 },
  { "say"      , "say"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "'"        , "'"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "save"     , "sav"		, POS_SLEEPING, do_save     , 0, ADMLVL_NONE	, 0 },
  { "saveall"  , "saveall"	, POS_DEAD    , do_saveall  , 0, ADMLVL_BUILDER	, 0},
  { "score"    , "sc"		, POS_DEAD    , do_score    , 0, ADMLVL_NONE	, 0 },
  { "scopy"    , "scopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_SEDIT },
  { "scribe"   , "scr"		, POS_RESTING , do_scribe   , 0, ADMLVL_NONE	, 0 },
  { "search"   , "sea"		, POS_STANDING, do_look     , 0, ADMLVL_NONE	, SCMD_SEARCH },
  { "sell"     , "sell"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "sedit"    , "sedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_SEDIT },
  { "send"     , "send"		, POS_SLEEPING, do_send     , 0, ADMLVL_GOD	, 0 },
  { "set"      , "set"		, POS_DEAD    , do_set      , 0, ADMLVL_GOD	, 0 },
  { "shout"    , "sho"		, POS_RESTING , do_gen_comm , 0, ADMLVL_NONE	, SCMD_SHOUT },
  { "show"     , "show"		, POS_DEAD    , do_show     , 0, ADMLVL_IMMORT	, 0 },
  { "shutdow"  , "shutdow"	, POS_DEAD    , do_shutdown , 0, ADMLVL_IMPL	, 0 },
  { "shutdown" , "shutdown"	, POS_DEAD    , do_shutdown , 0, ADMLVL_IMPL	, SCMD_SHUTDOWN },
  { "sip"      , "sip"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_SIP },
  { "sit"      , "sit"		, POS_RESTING , do_sit      , 0, ADMLVL_NONE	, 0 },
  { "skillset" , "skillset"	, POS_SLEEPING, do_skillset , 0, ADMLVL_GRGOD	, 0 },
  { "sleep"    , "sl"		, POS_SLEEPING, do_sleep    , 0, ADMLVL_NONE	, 0 },
  { "slist"    , "slist"	, POS_SLEEPING, do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_SLIST },
  { "slowns"   , "slowns"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMPL	, SCMD_SLOWNS },
  { "sneak"    , "sneak"	, POS_STANDING, do_gen_tog  , 1, ADMLVL_NONE	, SCMD_SNEAK },
  { "snoop"    , "snoop"	, POS_DEAD    , do_snoop    , 0, ADMLVL_GOD	, 0 },
  { "socials"  , "socials"	, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_SOCIALS },
  { "split"    , "split"	, POS_SITTING , do_split    , 1, ADMLVL_NONE	, 0 },
  { "speak"    , "spe"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "spells"   , "spel"		, POS_RESTING , do_spells   , 0, ADMLVL_IMMORT	, 0 },
  { "stand"    , "st"		, POS_RESTING , do_stand    , 0, ADMLVL_NONE	, 0 },
  { "stat"     , "stat"		, POS_DEAD    , do_stat     , 0, ADMLVL_IMMORT	, 0 },
  { "steal"    , "ste"		, POS_STANDING, do_steal    , 1, ADMLVL_NONE	, 0 },
  { "stop"     , "stop"		, POS_RESTING , do_memorize , 1, ADMLVL_NONE	, SCMD_STOP },
  { "stunning" , "stun"		, POS_FIGHTING, do_stunning , 1, ADMLVL_NONE	, 0 },
  { "switch"   , "switch"	, POS_DEAD    , do_switch   , 0, ADMLVL_GRGOD	, 0 },
  { "syslog"   , "syslog"	, POS_DEAD    , do_syslog   , 0, ADMLVL_IMMORT	, 0 },

  { "tame"     , "tame"         , POS_STANDING, do_tame     , 0, ADMLVL_NONE    , 0 },
  { "tcopy"    , "tcopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_TEDIT },
  { "tell"     , "t"		, POS_DEAD    , do_tell     , 0, ADMLVL_NONE	, 0 },
  { "take"     , "ta"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "taste"    , "tas"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_TASTE },
  { "teleport" , "tele"		, POS_DEAD    , do_teleport , 0, ADMLVL_GOD	, 0 },
  { "tedit"    , "tedit"	, POS_DEAD    , do_tedit    , 0, ADMLVL_GRGOD	, 0 },  
  { "thatch"   , "thatch"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_THATCH },
  { "thaw"     , "thaw"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_FREEZE	, SCMD_THAW },
  { "title"    , "title"	, POS_DEAD    , do_title    , 0, ADMLVL_NONE	, 0 },
  { "time"     , "time"		, POS_DEAD    , do_time     , 0, ADMLVL_NONE	, 0 },
  { "toggle"   , "toggle"	, POS_DEAD    , do_toggle   , 0, ADMLVL_NONE	, 0 },
  { "track"    , "track"	, POS_STANDING, do_track    , 0, ADMLVL_NONE	, 0 },
  { "trackthru", "trackthru"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMPL	, SCMD_TRACK },
  { "train"    , "train"	, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "transfer" , "transfer"	, POS_SLEEPING, do_trans    , 0, ADMLVL_GOD	, 0 },
  { "trigedit" , "trigedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_TRIGEDIT},
  { "turn"     , "turn"		, POS_STANDING, do_turn     , 0, ADMLVL_NONE	, 0},
  { "typo"     , "typo"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_TYPO },

  { "unlock"   , "unlock"	, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_UNLOCK },
  { "ungroup"  , "ungroup"	, POS_DEAD    , do_ungroup  , 0, ADMLVL_NONE	, 0 },
  { "unban"    , "unban"	, POS_DEAD    , do_unban    , 0, ADMLVL_GRGOD	, 0 },
  { "unaffect" , "unaffect"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_UNAFFECT },
  { "uptime"   , "uptime"	, POS_DEAD    , do_date     , 0, ADMLVL_IMMORT	, SCMD_UPTIME },
  { "use"      , "use"		, POS_SITTING , do_use      , 1, ADMLVL_NONE	, SCMD_USE },
  { "users"    , "users"	, POS_DEAD    , do_users    , 0, ADMLVL_IMMORT	, 0 },

  { "value"    , "val"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "version"  , "ver"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_VERSION },
  { "vieworder", "view" 	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_VIEWORDER },
  { "visible"  , "vis"		, POS_RESTING , do_visible  , 1, ADMLVL_NONE	, 0 },
  { "vnum"     , "vnum"		, POS_DEAD    , do_vnum     , 0, ADMLVL_IMMORT	, 0 },
  { "vstat"    , "vstat"	, POS_DEAD    , do_vstat    , 0, ADMLVL_IMMORT	, 0 },

  { "wake"     , "wa"		, POS_SLEEPING, do_wake     , 0, ADMLVL_NONE	, 0 },
  { "wear"     , "wea"		, POS_RESTING , do_wear     , 0, ADMLVL_NONE	, 0 },
  { "weather"  , "weather"	, POS_RESTING , do_weather  , 0, ADMLVL_NONE	, 0 },
  { "weave"    , "weave"	, POS_SITTING , do_assemble , 0, ADMLVL_NONE	, SCMD_WEAVE },
  { "who"      , "who"		, POS_DEAD    , do_who      , 0, ADMLVL_NONE	, 0 },
  { "whoami"   , "whoami"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WHOAMI },
  { "whois"    , "whois"	, POS_DEAD    , do_whois    , 0, ADMLVL_NONE	, 0 },
  { "where"    , "where"	, POS_RESTING , do_where    , 1, ADMLVL_NONE	, 0 },
  { "whisper"  , "whisper"	, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_WHISPER },
  { "wield"    , "wie"		, POS_RESTING , do_wield    , 0, ADMLVL_NONE	, 0 },
  { "wimpy"    , "wimpy"	, POS_DEAD    , do_value    , 0, ADMLVL_NONE	, SCMD_WIMPY },
  { "withdraw" , "withdraw"	, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "wiznet"   , "wiz"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_IMMORT	, 0 },
  { ";"        , ";"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_IMMORT	, 0 },
  { "wizhelp"  , "wizhelp"	, POS_SLEEPING, do_commands , 0, ADMLVL_IMMORT	, SCMD_WIZHELP },
  { "wizlist"  , "wizlist"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WIZLIST },
  { "wizlock"  , "wizlock"	, POS_DEAD    , do_wizlock  , 0, ADMLVL_IMPL	, 0 },
  { "wizupdate", "wizupdate"    , POS_DEAD    , do_wizupdate, 0, ADMLVL_GRGOD	, 0 },
  { "write"    , "write"	, POS_STANDING, do_write    , 1, ADMLVL_NONE	, 0 },

  { "zcheck"   , "zcheck"       , POS_DEAD    , do_zcheck   , 0, ADMLVL_GOD, 0 },
  { "zreset"   , "zreset"	, POS_DEAD    , do_zreset   , 0, ADMLVL_BUILDER	, 0 },
  { "zedit"    , "zedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_ZEDIT },
  { "zlist"    , "zlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_ZLIST },
  { "zpurge"   , "zpurge"       , POS_DEAD    , do_zpurge   , 0, ADMLVL_GRGOD, 0 },

  /* DG trigger commands */
  { "attach"   , "attach"	, POS_DEAD    , do_attach   , 0, ADMLVL_BUILDER	, 0 },
  { "detach"   , "detach"	, POS_DEAD    , do_detach   , 0, ADMLVL_BUILDER	, 0 },
  { "tlist"    , "tlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_TLIST },
  { "tstat"    , "tstat"	, POS_DEAD    , do_tstat    , 0, ADMLVL_BUILDER	, 0 },
  { "masound"  , "masound"	, POS_DEAD    , do_masound  , -1, ADMLVL_NONE	, 0 },
  { "mkill"    , "mkill"	, POS_STANDING, do_mkill    , -1, ADMLVL_NONE	, 0 },
  { "mjunk"    , "mjunk"	, POS_SITTING , do_mjunk    , -1, ADMLVL_NONE	, 0 },
  { "mdamage"  , "mdamage"	, POS_DEAD    , do_mdamage  , -1, ADMLVL_NONE	, 0 },
  { "mdoor"    , "mdoor"	, POS_DEAD    , do_mdoor    , -1, ADMLVL_NONE	, 0 },
  { "mecho"    , "mecho"	, POS_DEAD    , do_mecho    , -1, ADMLVL_NONE	, 0 },
  { "mechoaround", "mechoaround", POS_DEAD    , do_mechoaround, -1, ADMLVL_NONE	, 0 },
  { "msend"    , "msend"	, POS_DEAD    , do_msend    , -1, ADMLVL_NONE	, 0 },
  { "mload"    , "mload"	, POS_DEAD    , do_mload    , -1, ADMLVL_NONE	, 0 },
  { "mpurge"   , "mpurge"	, POS_DEAD    , do_mpurge   , -1, ADMLVL_NONE	, 0 },
  { "mgoto"    , "mgoto"	, POS_DEAD    , do_mgoto    , -1, ADMLVL_NONE	, 0 },
  { "mat"      , "mat"		, POS_DEAD    , do_mat      , -1, ADMLVL_NONE	, 0 },
  { "mteleport", "mteleport"	, POS_DEAD    , do_mteleport, -1, ADMLVL_NONE	, 0 },
  { "mforce"   , "mforce"	, POS_DEAD    , do_mforce   , -1, ADMLVL_NONE	, 0 },
  { "mhunt"    , "mhunt"	, POS_DEAD    , do_mhunt    , -1, ADMLVL_NONE	, 0 },
  { "mremember", "mremember"	, POS_DEAD    , do_mremember, -1, ADMLVL_NONE	, 0 },
  { "mforget"  , "mforget"	, POS_DEAD    , do_mforget  , -1, ADMLVL_NONE	, 0 },
  { "mtransform", "mtransform"	, POS_DEAD    , do_mtransform, -1, ADMLVL_NONE	, 0 },
  { "mzoneecho", "mzoneecho"	, POS_DEAD    , do_mzoneecho, -1, ADMLVL_NONE	, 0 },
  { "vdelete"  , "vdelete"	, POS_DEAD    , do_vdelete  , 0, ADMLVL_BUILDER	, 0 },
  { "mfollow"  , "mfollow"	, POS_DEAD    , do_mfollow  , -1, ADMLVL_NONE	, 0 },

  { "\n", "zzzzzzz", 0, 0, 0, ADMLVL_NONE	, 0 } };	/* this must be last */

const char *fill[] =
{
  "in",
  "inside",
  "into",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/* This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function. */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  /* REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE); */

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /* special case to handle one-character, non-alphanumeric commands; requested 
   * by many people so "'hi" or ";godnet test" is possible. Patch sent by Eric 
   * Green and Stefan Wasilewski.  */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. otherwise, find the command */
  {
  int cont;                                            /* continue the command checks */
  cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
  if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
  if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
  if (cont) return;                                    /* yes, command trigger took over */
  }
  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++) {
    if (!strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level &&
          GET_ADMLEVEL(ch) >= complete_cmd_info[cmd].minimum_admlevel)
	break;
  }
  if (*complete_cmd_info[cmd].command == '\n')
    send_to_char(ch, "Huh?!?\r\n");
  else if (check_disabled(&complete_cmd_info[cmd]))    /* is it disabled? */
      send_to_char(ch, "This command has been temporarily disabled.\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_admlevel >= ADMLVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
  } else if (no_specials || !special(ch, cmd, line))
    ((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                            *
 **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH];
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next, temp);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char(ch, "You can't alias 'alias'.\r\n");
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char(ch, "Alias added.\r\n");
    }
  }
}

/* Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands. */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH];	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/* Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue. */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/* searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching. */
int search_block(char *arg, const char **list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae. */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/* Function to skip over the leading spaces of a string. */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


/* Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place. */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/* copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string. */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/* one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word.
 *
 * No longer ignores fill words.  -dak, 6 Jan 2003. */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}


/* Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

void display_races(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YRace SELECTION menu:\r\n@G---------------------------------------\r\n@n");
  for (x = 0; x < NUM_RACES; x++)
    if (race_ok_gender[(int)GET_SEX(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%-15s@n%s", x+1, pc_race_types[x], !(++i %2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BR@W) @CRandom Race Selection!\r\n@n");
      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WRace: @n");
}

void display_classes(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YClass SELECTION menu:\r\n@G--------------------------------------\r\n@n");
  for (x = 0; x < NUM_BASIC_CLASSES; x++)
    if (class_ok_race[(int)GET_RACE(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%s@n%s", x+1, pc_class_types[x], !(++i%2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BR@W) @CRandom Class Selection!\r\n@n");
      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WClass: @n");
}

void display_races_help(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YRace HELP menu:\r\n@G--------------------------------------------\r\n@n");
  for (x = 0; x < NUM_RACES; x++)
    if (race_ok_gender[(int)GET_SEX(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%-15s@n%s", x+1, pc_race_types[x], !(++i%2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WHelp on Race #: @n");
}

void display_classes_help(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YClass HELP menu:\r\n@G-------------------------------------------\r\n@n");
  for (x = 0; x < NUM_BASIC_CLASSES; x++)
    if (class_ok_race[(int)GET_RACE(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%s@n%s", x+1, pc_class_types[x], !(++i%2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WHelp on Class #: @n");
}

/* determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/* Return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string) */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  if (arg2 != temp)
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}



/* *************************************************************************
 *  Stuff for controlling the non-playing sockets (get name, pwd etc       *
 ************************************************************************* */


/* This function needs to die. */
int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /* Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && GET_IDNUM(k->character) == id) {
      /* Character taking over their own body. */

      if (!target && STATE(k) == CON_PLAYING) {
	write_to_output(k, "\r\nThis body has been usurped!\r\n");
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

 /* now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist). */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target)
    return (0);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->timer = 0;
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
  REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_NONE, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    if (has_mail(GET_IDNUM(d->character)))
      write_to_output(d, "You have mail waiting.\r\n");
#ifdef HAVE_ZLIB_H
    if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
    }
#endif /* HAVE_ZLIB_H */
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
#ifdef HAVE_ZLIB_H
    if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
    }
#endif /* HAVE_ZLIB_H */
    break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{
    int load_result;
    IDXTYPE load_room;
    struct char_data *check;
    
      reset_char(d->character);
      read_aliases(d->character);

      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

      /* We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE. */

      if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
	load_room = real_room(load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
          if (GET_RACE(d->character) == 23)
              load_room = real_room(CONFIG_CREATE_START);
          else if (GET_ADMLEVEL(d->character))
	  load_room = real_room(CONFIG_IMMORTAL_START);
	else
	  load_room = real_room(CONFIG_MORTAL_START);
      }

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = real_room(CONFIG_FROZEN_START);

      d->character->next = character_list;
      character_list = d->character;
      char_to_room(d->character, load_room);
      load_result = Crash_load(d->character);
      if (d->character->player_specials->host) {
        free(d->character->player_specials->host);
        d->character->player_specials->host = NULL;
      }
      d->character->player_specials->host = strdup(d->host);
      GET_ID(d->character) = GET_IDNUM(d->character);
      /* find_char helper */
      add_to_lookup_table(GET_ID(d->character), (void *)d->character);
      read_saved_vars(d->character);
      load_char_pets(d->character);
      for (check = character_list; check; check = check->next)
        if (!check->master && IS_NPC(check) && check->master_id == GET_IDNUM(d->character) &&
            AFF_FLAGGED(check, AFF_CHARM) && !circle_follow(check, d->character))
          add_follower(check, d->character);
      if (GET_LOADROOM(d->character) && !PLR_FLAGGED(d->character, PLR_LOADROOM))
        GET_LOADROOM(d->character) = NOWHERE;
      save_char(d->character);

    return load_result;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  int load_result=-1;	/* Overloaded variable */
  int player_i, total, rr;

  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char*);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_IEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_CEDIT, cedit_parse },
    { CON_AEDIT, aedit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { CON_ASSEDIT, assedit_parse },
    { CON_GEDIT, gedit_parse },
    { CON_LEVELUP, levelup_parse },
    { CON_HEDIT, hedit_parse },
    { -1, NULL }
  };

  skip_spaces(&arg);

  if (d->character == NULL) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
  }

  /* Quick check for the OLC states. */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      /* send context-sensitive help if need be */
      if (context_help(d, arg)) return;
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d)) {
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	write_to_output(d, "Invalid name, please try another.\r\nName: ");
	return;
      }
      if ((player_i = load_char(tmp_name, d->character)) > -1) {
	GET_PFILEPOS(d->character) = player_i;

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {

	  /* make sure old files are removed so the new player doesn't get
	   * the deleted player's equipment (this should probably be a
	   * stock behavior) */
	  if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
	    remove_player(player_i);

	  /* We get a false positive from the original deleted character. */
	  free_char(d->character);
	  /* Check for multiple creations... */
	  if (!Valid_Name(tmp_name)) {
	    write_to_output(d, "@YInvalid name@n, please try @Canother.@n\r\nName: ");
	    return;
	  }
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->name, CAP(tmp_name));	/* strcpy: OK (size checked above) */
	  GET_PFILEPOS(d->character) = player_i;
	  write_to_output(d, "Did I get that right, %s (Y/N)? ", tmp_name);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
	  REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
          d->character->time.logon = time(0);
	  write_to_output(d, "Password: ");
	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	/* Check for multiple creations of a character. */
	if (!Valid_Name(tmp_name)) {
	  write_to_output(d, "Invalid name, please try another.\r\nName: ");
	  return;
	}
	CREATE(d->character->name, char, strlen(tmp_name) + 1);
	strcpy(d->character->name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

	write_to_output(d, "Did I get that right, %s (Y/N)? ", tmp_name);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;

  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlog(NRM, ADMLVL_GOD, TRUE, "Request for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
	write_to_output(d, "Sorry, new characters are not allowed from your site!\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	write_to_output(d, "Sorry, new players can't be created at the moment.\r\n");
	mudlog(NRM, ADMLVL_GOD, TRUE, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      write_to_output(d, "@MNew character.@n\r\nGive me a @gpassword@n for @C%s@n: ", GET_PC_NAME(d->character));
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "Okay, what IS it, then? ");
      free(d->character->name);
      d->character->name = NULL;
      STATE(d) = CON_GET_NAME;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;

  case CON_PASSWORD:		/* get pwd for known player      */
    /* To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96 */

    echo_on(d);    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    write_to_output(d, "\r\n");

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	mudlog(BRF, ADMLVL_GOD, TRUE, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	GET_BAD_PWS(d->character)++;
	save_char(d->character);
	if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) {	/* 3 strikes and you're out. */
	  write_to_output(d, "Wrong password... disconnecting.\r\n");
	  STATE(d) = CON_CLOSE;
	} else {
	  write_to_output(d, "Wrong password.\r\nPassword: ");
	  echo_off(d);
	}
	return;
      }

      /* Password was correct. */
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n");
	STATE(d) = CON_CLOSE;
	mudlog(NRM, ADMLVL_GOD, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
	return;
      }
      if (GET_LEVEL(d->character) < circle_restrict) {
	write_to_output(d, "The game is temporarily restricted.. try again later.\r\n");
	STATE(d) = CON_CLOSE;
	mudlog(NRM, ADMLVL_GOD, TRUE, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
	return;
      }
      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	return;

      if (GET_ADMLEVEL(d->character))
	write_to_output(d, "%s", imotd);
      else
	write_to_output(d, "%s", motd);

      if (GET_INVIS_LEV(d->character)) 
        mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, 
        "%s [%s] has connected. (invis %d)", GET_NAME(d->character), d->host, 
        GET_INVIS_LEV(d->character));
      else
        mudlog(BRF, ADMLVL_IMMORT, TRUE, 
               "%s [%s] has connected.", GET_NAME(d->character), d->host);

      if (load_result) {
        write_to_output(d, "\r\n\r\n\007\007\007"
		"@r%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.@n\r\n",
		load_result, (load_result > 1) ? "S" : "");
	GET_BAD_PWS(d->character) = 0;
      }
      write_to_output(d, "\r\n*** PRESS RETURN: ");
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_PC_NAME(d->character))) {
      write_to_output(d, "\r\nIllegal password.\r\nPassword: ");
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH+1) */
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    write_to_output(d, "\r\nPlease retype @gpassword@n: ");
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;
    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nPasswords don't match... start over.\r\nPassword: ");
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
//      write_to_output(d, "\r\nWhat is your sex (@WM/F@n)? ");
//     STATE(d) = CON_QSEX;
        GET_RACE(d->character) = 23;
/*        load_result = enter_player_game(d);
      send_to_char(d->character, "%s", CONFIG_WELC_MESSG);
      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
      d->character->time.logon = time(0);
      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      STATE(d) = CON_PLAYING;
      look_at_room(IN_ROOM(d->character), d->character, 0);
*/
    } else {
      save_char(d->character);
      write_to_output(d, "\r\nDone.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->sex = SEX_FEMALE;
      break;
    default:
      write_to_output(d, "That is not a sex..\r\n"
		"What IS your sex? ");
      return;
    }

    display_races(d);
    STATE(d) = CON_QRACE;
    break;

    case CON_QRACE:
    switch (*arg) {
      case 'r':
      case 'R':
        while (load_result == RACE_UNDEFINED) {
          rr = rand_number(1, NUM_RACES);
          load_result = parse_race(d->character, rr);
        }
        break;
      case 't':
      case 'T':
        display_races_help(d);
        STATE(d) = CON_RACE_HELP;
        return;
    }
    if (load_result == RACE_UNDEFINED)
      load_result = parse_race(d->character, atoi(arg));
      if (load_result == RACE_UNDEFINED) {
        write_to_output(d, "\r\nThat's not a race.\r\nRace: ");
        return;
      } else
        GET_RACE(d->character) = load_result;

    display_classes(d);
    STATE(d) = CON_QCLASS;
    break;

  case CON_RACE_HELP:
    if (*arg == 't' || *arg == 'T') {
      display_races(d);
      STATE(d) = CON_QRACE;
      return;
    }
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i > NUM_RACES || player_i < 1) {
	write_to_output(d, "\r\nThat's not a race.\r\nHelp on Race #: ");
	break;
      }
      player_i -= 1;
      if (race_ok_gender[(int)GET_SEX(d->character)][player_i])
	show_help(d, race_names[player_i]);
      else
	write_to_output(d, "\r\nThat's not a race.\r\nHelp on Race #: ");
    } else {
      display_races_help(d);
    }
    STATE(d) = CON_RACE_HELP;
    break;
  
  case CON_CLASS_HELP:
    if (*arg == 't' || *arg == 'T') {
      display_classes(d);
      STATE(d) = CON_QCLASS;
      return;
    }
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i > NUM_BASIC_CLASSES || player_i < 1) {
	write_to_output(d, "\r\nThat's not a class.\r\nHelp on Class #: ");
	break;
      }
      player_i -= 1;
      if (class_ok_race[(int)GET_SEX(d->character)][player_i])
        show_help(d, class_names[player_i]);
      else
	write_to_output(d, "\r\nThat's not a class.\r\nHelp on Class #: ");
    } else {
      display_classes_help(d);
    }
    STATE(d) = CON_CLASS_HELP;
    break;

  case CON_QCLASS:
    switch (*arg) {
      case 'r':
      case 'R':
        while (load_result == CLASS_UNDEFINED) {
          rr = rand_number(1, NUM_BASIC_CLASSES);
          load_result = parse_class(d->character, rr);
        }
        break;
      case 't':
      case 'T':
        display_classes_help(d);
        STATE(d) = CON_CLASS_HELP;
        return;
    }
    if (load_result == CLASS_UNDEFINED)
    load_result = parse_class(d->character, atoi(arg));
    if (load_result == CLASS_UNDEFINED) {
      write_to_output(d, "\r\nThat's not a class.\r\nClass: ");
      return;
    } else
      GET_CLASS(d->character) = load_result;

    write_to_output(d, "\r\n*** PRESS RETURN: ");
    STATE(d) = CON_QROLLSTATS;
    break;

  case CON_QROLLSTATS:
    if (CONFIG_REROLL_PLAYER_CREATION && 
       (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_1)) {
      switch (*arg) {
      case 'y':
      case 'Y':
        break;
      case 'n':
      case 'N':
      default:
        cedit_creation(d->character);
        write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                              "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                              "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
           GET_STR(d->character), GET_DEX(d->character), 
           GET_CON(d->character), GET_INT(d->character), 
           GET_WIS(d->character), GET_CHA(d->character));
        write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
        return;
      }
    } else if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2 ||
        CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3) {
        if (CONFIG_REROLL_PLAYER_CREATION && 
           (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2)) {
          switch (*arg) {
            case 'y':
            case 'Y':
              break;
            case 'n':
            case 'N':
            default:
              cedit_creation(d->character);
              write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                                    "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                                    "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
                 GET_STR(d->character), GET_DEX(d->character),
                 GET_CON(d->character), GET_INT(d->character),
                 GET_WIS(d->character), GET_CHA(d->character));
	      write_to_output(d, "Initial statistics, you may reassign individual numbers\r\n");
	      write_to_output(d, "between statistics after choosing yes.\r\n");
              write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
              return;
          }
        }
	else
      cedit_creation(d->character);
      if (!d->olc) {
        CREATE(d->olc, struct oasis_olc_data, 1);
      }
      if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3)
        OLC_VAL(d) = CONFIG_INITIAL_POINTS_POOL;
      else
        OLC_VAL(d) = 0;

      STATE(d) = CON_QSTATS;
      stats_disp_menu(d);
      break;
    } else {
      cedit_creation(d->character);
    }

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    save_char(d->character);
    save_player_index();
    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;
    total = GET_STR(d->character) / 2 + GET_CON(d->character) / 2 + 
            GET_WIS(d->character) / 2 + GET_INT(d->character) / 2 + 
            GET_DEX(d->character) / 2 + GET_CHA(d->character) / 2;
    total -= 30;
    mudlog(CMP, ADMLVL_GOD, TRUE, "New player: %s [%s %s]", 
           GET_NAME(d->character), pc_race_types[GET_RACE(d->character)], 
           pc_class_types[GET_CLASS(d->character)]);
    mudlog(CMP, ADMLVL_GOD, TRUE, "Str: %2d Dex: %2d Con: %2d Int: %2d "
           "Wis:  %2d Cha: %2d mod total: %2d", GET_STR(d->character), 
           GET_DEX(d->character), GET_CON(d->character), GET_INT(d->character),
           GET_WIS(d->character), GET_CHA(d->character), total);
    break;

  case CON_QSTATS:
    if (parse_stats(d, arg)) {

      if (d->olc) {
        free(d->olc);
        d->olc = NULL;
      }
      if (GET_PFILEPOS(d->character) < 0) {
        GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
      }
      /* Now GET_NAME() will work properly. */
      init_char(d->character);
      save_char(d->character);
      save_player_index();
      write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
      STATE(d) = CON_RMOTD;
    total = GET_STR(d->character) / 2 + GET_CON(d->character) / 2 + 
            GET_WIS(d->character) / 2 + GET_INT(d->character) / 2 + 
            GET_DEX(d->character) / 2 + GET_CHA(d->character) / 2;
    total -= 30;
    mudlog(CMP, ADMLVL_GOD, TRUE, "New player: %s [%s %s]", 
           GET_NAME(d->character), pc_race_types[GET_RACE(d->character)], 
           pc_class_types[GET_CLASS(d->character)]);
    mudlog(CMP, ADMLVL_GOD, TRUE, "Str: %2d Dex: %2d Con: %2d Int: %2d "
           "Wis:  %2d Cha: %2d mod total: %2d", GET_STR(d->character), 
           GET_DEX(d->character), GET_CON(d->character), GET_INT(d->character),
           GET_WIS(d->character), GET_CHA(d->character), total);
    } 
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
#ifdef HAVE_ZLIB_H
    if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
    }
#endif /* HAVE_ZLIB_H */
    write_to_output(d, "%s", CONFIG_MENU);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU: {		/* get selection from main menu  */

    switch (*arg) {
    case '0':
      write_to_output(d, "Goodbye.\r\n");
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      load_result = enter_player_game(d);
      send_to_char(d->character, "%s", CONFIG_WELC_MESSG);
      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      d->character->time.logon = time(0);
      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      STATE(d) = CON_PLAYING;
      if (GET_LEVEL(d->character) == 0) {
	do_start(d->character);
	send_to_char(d->character, "%s", CONFIG_START_MESSG);
      }
      look_at_room(IN_ROOM(d->character), d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char(d->character, "\r\nYou have mail waiting.\r\n");
      if (load_result == 2) {	/* rented items lost */
	send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
		"Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);
      break;

    case '2':
      if (d->character->description) {
	write_to_output(d, "Current description:\r\n%s", d->character->description);
	/* Don't free this now... so that the old description gets loaded
	 * as the current buffer in the editor.  Do setup the ABORT buffer
	 * here, however.
	 *
	 * free(d->character->description);
	 * d->character->description = NULL; */
	d->backstr = strdup(d->character->description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(d);
      d->str = &d->character->description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      write_to_output(d, "\r\nEnter your old password: ");
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      write_to_output(d, "\r\nEnter your password for verification: ");
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s", CONFIG_MENU);
      break;
    }
    break;
  }

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nEnter a new password: ");
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ");
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
		"Character not deleted.\r\n\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_ADMLEVEL(d->character) < ADMLVL_GRGOD)
	SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character);
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all
       * the player's immediately */
      if (selfdelete_fastwipe)
        if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }

      delete_aliases(GET_NAME(d->character));
      delete_variables(GET_NAME(d->character));
      write_to_output(d, "Character '%s' deleted!\r\n"
	      "Goodbye.\r\n", GET_NAME(d->character));
      mudlog(NRM, ADMLVL_GOD, TRUE, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_CLOSE;
      return;
    } else {
      write_to_output(d, "\r\nCharacter not deleted.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  /* It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them. */
  case CON_CLOSE:
    break;

  case CON_ASSEDIT:
    assedit_parse(d, arg);
    break;

  case CON_GEDIT:
    gedit_parse(d, arg);
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}

/* Code to disable or enable buggy commands on the run, saving
 * a list of disabled commands to disk. Originally created by
 * Erwin S. Andreasen (erwin@andreasen.org) for Merc. Ported to
 * CircleMUD by Alexei Svitkine (Myrdred), isvitkin@sympatico.ca.
 *
 * Syntax is:
 *   disable - shows disabled commands
 *   disable <command> - toggles disable status of command */

ACMD(do_disable)
{
  int i, length;
  DISABLED_DATA *p, *temp;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't disable commands, silly.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument) { /* Nothing specified. Show disabled commands. */
    if (!disabled_first) /* Any disabled at all ? */
      send_to_char(ch, "There are no disabled commands.\r\n");
    else {
      send_to_char(ch,
        "Commands that are currently disabled:\r\n\r\n"
        " Command       Disabled by     Level\r\n"
        "-----------   --------------  -------\r\n");
      for (p = disabled_first; p; p = p->next)
        send_to_char(ch, " %-12s   %-12s    %3d\r\n", p->command->command, p->disabled_by, p->level);
    }
    return;
  }

  /* command given - first check if it is one of the disabled commands */
  for (length = strlen(argument), p = disabled_first; p ;  p = p->next)
    if (!strncmp(argument, p->command->command, length))
    break;
        
  if (p) { /* this command is disabled */

    /* Was it disabled by a higher level imm? */
    if (GET_ADMLEVEL(ch) < p->level) {
      send_to_char(ch, "This command was disabled by a higher power.\r\n");
      return;
    }

    REMOVE_FROM_LIST(p, disabled_first, next, temp);
    send_to_char(ch, "Command '%s' enabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "(GC) %s has enabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    free(p->disabled_by);
    free(p);
    save_disabled(); /* save to disk */

  } else { /* not a disabled command, check if the command exists */

    for (length = strlen(argument), i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strncmp(cmd_info[i].command, argument, length))
        if (GET_LEVEL(ch) >= cmd_info[i].minimum_level &&
            GET_ADMLEVEL(ch) >= cmd_info[i].minimum_admlevel)
          break;

    /*  Found?     */            
    if (*cmd_info[i].command == '\n') {
      send_to_char(ch, "You don't know of any such command.\r\n");
      return;
    }

    if (!strcmp(cmd_info[i].command, "disable")) {
      send_to_char (ch, "You cannot disable the disable command.\r\n");
      return;
    }

    /* Disable the command */
    CREATE(p, struct disabled_data, 1);
    p->command = &cmd_info[i];
    p->disabled_by = strdup(GET_NAME(ch));	/* save name of disabler  */
    p->level = GET_ADMLEVEL(ch);		/* save admin level of disabler */    
    p->subcmd = cmd_info[i].subcmd;		/* the subcommand if any  */    
    p->next = disabled_first;
    disabled_first = p; 			/* add before the current first element */
    send_to_char(ch, "Command '%s' disabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "(GC) %s has disabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    save_disabled(); /* save to disk */
  }
}

/* check if a command is disabled */   
int check_disabled(const struct command_info *command)
{
  DISABLED_DATA *p;

  for (p = disabled_first; p ; p = p->next)
    if (p->command->command_pointer == command->command_pointer)
      if (p->command->subcmd == command->subcmd)
        return TRUE;

  return FALSE;
}

/* Load disabled commands */
void load_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;
  int i;
  char line[READ_SIZE], name[MAX_INPUT_LENGTH], temp[MAX_INPUT_LENGTH];

  if (disabled_first)
    free_disabled();

  if ((fp = fopen(DISABLED_FILE, "r")) == NULL)
    return; /* No disabled file.. no disabled commands. */

  while (get_line(fp, line)) { 
    if (!str_cmp(line, END_MARKER))
      break; /* break loop if we encounter the END_MARKER */
    CREATE(p, struct disabled_data, 1);
    sscanf(line, "%s %d %d %s", name, &(p->subcmd), &(p->level), temp);
    /* Find the command in the table */
    for (i = 0; *cmd_info[i].command != '\n'; i++)
      if (!str_cmp(cmd_info[i].command, name))
        break;
    if (*cmd_info[i].command == '\n') { /* command does not exist? */
      log("WARNING: load_disabled(): Skipping unknown disabled command - '%s'!", name);
      free(p);
    } else { /* add new disabled command */
      p->disabled_by = strdup(temp);
      p->command = &cmd_info[i];
      p->next = disabled_first;
      disabled_first = p;
    }
  }
  fclose(fp);
}

/* Save disabled commands */
void save_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;

  if (!disabled_first) {
    /* delete file if no commands are disabled */
    unlink(DISABLED_FILE);
    return;
   }

  if ((fp = fopen(DISABLED_FILE, "w")) == NULL) {
    log("SYSERR: Could not open " DISABLED_FILE " for writing");
    return;
  }

  for (p = disabled_first; p ; p = p->next)
    fprintf (fp, "%s %d %d %s\n", p->command->command, p->subcmd, p->level, p->disabled_by);
  fprintf(fp, "%s\n", END_MARKER);
  fclose(fp);
}
  
/* free all disabled commands from memory */
void free_disabled()
{
  DISABLED_DATA *p;

  while (disabled_first) {
    p = disabled_first;
    disabled_first = disabled_first->next;
    free(p->disabled_by);
    free(p);
  }
}

