/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class. */



#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/class.c,v 1.31 2005/05/29 18:36:38 zizazat Exp $");

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "feats.h"
#include "oasis.h"

extern void racial_ability_modifiers(struct char_data *ch);
extern void set_height_and_weight_by_race(struct char_data *ch);
extern void racial_body_parts(struct char_data *ch);
extern int spell_lvlmax_table[NUM_CLASSES][LVL_EPICSTART][MAX_SPELL_LEVEL];
extern void add_innate_timer(struct char_data *ch, int spellnum, int sktype);

/* local functions */
void snoop_check(struct char_data *ch);
int parse_class(struct char_data *ch, int arg);
int num_attacks(struct char_data *ch, int offhand);
void roll_real_abils(struct char_data *ch);
void do_start(struct char_data *ch);
int backstab_dice(struct char_data *ch);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(int level);
byte object_saving_throws(int material_type, int type);
int load_levels();
void assign_auto_stats(struct char_data *ch);
void cedit_creation(struct char_data *ch);

/* Names first */

const char *class_abbrevs[NUM_CLASSES+1] = {
  "Wi",
  "Cl",
  "Ro",
  "Fi",
  "Mo",
  "Pa",
  "So",
  "Dr",
  "Ba",
  "Ra",
  "Bn",
  "Aa",
  "At",
  "Ar",
  "As",
  "Bl",
  "Dd",
  "Du",
  "Dw",
  "Ek",
  "Ht",
  "Hw",
  "Lo",
  "Mt",
  "Sh",
  "Th",
  "Ex",
  "Ad",
  "Co",
  "Ar",
  "Wa",
  "\n"
};


/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *pc_class_types[NUM_CLASSES+1] = {
  "Wizard",
  "Cleric",
  "Rogue",
  "Fighter",
  "Monk",
  "Paladin",
  "Sorcerer",
  "Druid",
  "Bard",
  "Ranger",
  "Barbarian",
  "Arcane Archer",
  "Arcane Trickster",
  "Archmage",
  "Assassin",
  "Blackguard",
  "Dragon Disciple",
  "Duelist",
  "Dwarven Defender",
  "Eldritch Knight",
  "Hierophant",
  "Horizon Walker",
  "Loremaster",
  "Mystic Theurge",
  "Shadowdancer",
  "Thaumaturgist",
  "Expert",
  "Adept",
  "Commoner",
  "Aristrocrat",
  "Warrior",
  "\n"
};

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *class_names[NUM_CLASSES+1] = {
  "wizard",
  "cleric",
  "rogue",
  "fighter",
  "monk",
  "paladin",
  "sorcerer",
  "druid",
  "bard",
  "ranger",
  "barbarian",
  "arcane archer",
  "arcane trickster",
  "archmage",
  "assassin",
  "blackguard",
  "dragon disciple",
  "duelist",
  "dwarven defender",
  "eldritch knight",
  "hierophant",
  "horizon walker",
  "loremaster",
  "mystic theurge",
  "shadowdancer",
  "thaumaturgist",
  "artisan",
  "magi",
  "normal",
  "noble",
  "soldier",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_display[NUM_CLASSES] = {
  "@B1@W) @MWizard\r\n",
  "@B2@W) @WCleric\r\n",
  "@B3@W) @YRogue\r\n",
  "@B4@W) @BFighter\r\n"
  "@B5@W) @BMonk\r\n"
  "@B6@W) @BPaladin\r\n"
  "@B7@W) @WSorcerer\r\n",
  "@B8@W) @YDruid\r\n",
  "@B9@W) @BBard\r\n"
  "@B10@W) @BRanger\r\n"
  "@B11@W) @BBarbarian\r\n"
  "arcane archer (P)\r\n",
  "arcane trickster (P)\r\n",
  "archmage (P)\r\n",
  "assassin (P)\r\n",
  "blackguard (P)\r\n",
  "dragon disciple (P)\r\n",
  "duelist (P)\r\n",
  "dwarven defender (P)\r\n",
  "eldritch knight (P)\r\n",
  "hierophant (P)\r\n",
  "horizon walker (P)\r\n",
  "loremaster (P)\r\n",
  "mystic theurge (P)\r\n",
  "shadowdancer (P)\r\n",
  "thaumaturgist (P)\r\n",
  "Artisan NPC\r\n",
  "Magi NPC\r\n",
  "Normal NPC\r\n",
  "Noble NPC\r\n",
  "Soldier NPC\r\n",
};

#define Y   TRUE
#define N   FALSE

/* Some races copied from the SRD under OGL, see ../doc/srd.txt for information */
int class_ok_race[NUM_RACES][NUM_CLASSES] = {
  /*                 Wi,Cl,Ro,Fi,Mo,Pa,So,Dr,Ba,Ra,Bn,Aa,At,Am,As,Bg,Dd,Du,Dw,Ek,Ht,Hw,Lr,Mt,Sd,Th,Ad,Ma,No,Nl,So,   */
  /* Human      */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Elf        */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Gnome      */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Dwarf      */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Half Elf   */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Halfling   */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Drow Elf   */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Half Orc   */ { Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Animal     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Construct  */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, N, N, N },
  /* Demon      */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Dragon     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Fish       */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Giant      */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Goblin     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Insect     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Orc        */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Snake      */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Troll      */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Minotaur   */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Kobold     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Mindflayer */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Warhost    */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Faerie     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Spirit     */ { Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N }
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int class_ok_align[NUM_ALIGNS][NUM_CLASSES] = {
/*         Wi,Cl,Ro,Fi,Mo,Pa,So,Dr,Ba,Ra,Bn,Aa,At,Am,As,Bg,Dd,Du,Dw,Ek,He,Hw,Lm,Mt,Sd,Th*/
/* LG */ { Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* NG */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* CG */ { Y, Y, Y, Y, N, N, Y, N, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* LN */ { Y, Y, Y, Y, Y, N, Y, Y, N, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* NN */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* CN */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* LE */ { Y, Y, Y, Y, Y, N, Y, N, N, Y, N, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* NE */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* CE */ { Y, Y, Y, Y, N, N, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y }
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int favored_class[NUM_RACES] = {
/* -1 means highest class is considered favored */
/* Human      */ -1,
/* Elf        */ CLASS_WIZARD,
/* Gnome      */ CLASS_BARD,
/* Dwarf      */ CLASS_FIGHTER,
/* Half Elf   */ -1,
/* Halfling   */ CLASS_ROGUE,
/* Drow Elf   */ CLASS_WIZARD,
/* Half Orc   */ CLASS_BARBARIAN,
/* Animal     */ -1,
/* Construct  */ -1,
/* Demon      */ -1,
/* Dragon     */ -1,
/* Fish       */ -1,
/* Giant      */ -1,
/* Goblin     */ CLASS_ROGUE,
/* Insect     */ -1,
/* Orc        */ CLASS_BARBARIAN,
/* Snake      */ -1,
/* Troll      */ -1,
/* Minotaur   */ CLASS_FIGHTER,
/* Kobold     */ -1,
/* Lizardfolk */ -1,
/* Warhost    */ -1,
/* Faerie     */ -1,
/* Spirit     */ -1
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int prestige_classes[NUM_CLASSES] = {
/* WIZARD	*/ N,
/* CLERIC	*/ N,
/* ROGUE	*/ N,
/* FIGHTER	*/ N,
/* MONK		*/ N,
/* PALADIN	*/ N,
/* SORCERER	*/ N,
/* DRUID	*/ N,
/* BARD 	*/ N,
/* RANGER	*/ N,
/* BARBARIAN	*/ N,
/* ARCANE ARCHER    */ Y,
/* ARCANE TRICKSTER */ Y,
/* ARCHMAGE         */ Y,
/* ASSASSIN         */ Y,
/* BLACKGUARD       */ Y,
/* DRAGON DISCIPLE  */ Y,
/* DUELIST          */ Y,
/* DWARVEN DEFENDER */ Y,
/* ELDRITCH KNIGHT  */ Y,
/* HIEROPHANT       */ Y,
/* HORIZON WALKER   */ Y,
/* LOREMASTER       */ Y,
/* MYSTIC THEURGE   */ Y,
/* SHADOWDANCER     */ Y,
/* THAUMATURGIST    */ Y,
/* ARTISAN	*/ N,
/* MAGI		*/ N,
/* NORMAL	*/ N,
/* NOBLE	*/ N,
/* SOLDIER	*/ N
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
/* -1 indicates no limit to the number of levels in this class under
 * epic rules */
int class_max_ranks[NUM_CLASSES] = {
/* WIZARD	*/ 20,
/* CLERIC	*/ 20,
/* ROGUE	*/ 20,
/* FIGHTER	*/ 20,
/* MONK		*/ 20,
/* PALADIN	*/ 20,
/* SORCERER	*/ 20,
/* DRUID	*/ 20,
/* BARD 	*/ 20,
/* RANGER	*/ 20,
/* BARBARIAN	*/ 20,
/* ARCANE ARCHER    */ 10,
/* ARCANE TRICKSTER */ 10,
/* ARCHMAGE         */ 5,
/* ASSASSIN         */ 10,
/* BLACKGUARD       */ 10,
/* DRAGON DISCIPLE  */ 10,
/* DUELIST          */ 10,
/* DWARVEN DEFENDER */ 10,
/* ELDRITCH KNIGHT  */ 10,
/* HIEROPHANT       */ 5,
/* HORIZON WALKER   */ 10,
/* LOREMASTER       */ 10,
/* MYSTIC THEURGE   */ 10,
/* SHADOWDANCER     */ 10,
/* THAUMATURGIST    */ 5,
/* ARTISAN	*/ -1,
/* MAGI		*/ -1,
/* NORMAL	*/ -1,
/* NOBLE	*/ -1,
/* SOLDIER	*/ -1
};


/* The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c. */

int parse_class(struct char_data *ch, int arg)
{
  int chclass = CLASS_UNDEFINED;

  switch (arg) {
  case 1 : chclass = CLASS_WIZARD   ; break;
  case 2 : chclass = CLASS_CLERIC   ; break;
  case 3 : chclass = CLASS_ROGUE    ; break;
  case 4 : chclass = CLASS_FIGHTER  ; break;
  case 5 : chclass = CLASS_MONK     ; break;
  case 6 : chclass = CLASS_PALADIN  ; break;
  case 7 : chclass = CLASS_SORCERER ; break;
  case 8 : chclass = CLASS_DRUID    ; break;
  case 9 : chclass = CLASS_BARD     ; break;
  case 10: chclass = CLASS_RANGER   ; break;
  case 11: chclass = CLASS_BARBARIAN; break;
  default:  chclass = CLASS_UNDEFINED; break;
  }
  if (chclass >= 0 && chclass < NUM_BASIC_CLASSES)
    if (!class_ok_race[(int)GET_RACE(ch)][chclass])
      chclass = CLASS_UNDEFINED;

  return (chclass);
}


/* Is anything preventing this character from advancing in this class? */
int class_ok_general(struct char_data *ch, int whichclass)
{
  if (whichclass < 0 || whichclass >= NUM_BASIC_CLASSES) {
    log("Invalid class %d in class_ok_general", whichclass);
    return 0;
  }
  if (!class_ok_race[(int)GET_RACE(ch)][whichclass])
    return 0;
  if (!class_ok_align[ALIGN_TYPE(ch)][whichclass])
    return 0;
  if (class_max_ranks[whichclass] > -1 &&
      GET_CLASS_RANKS(ch, whichclass) >= class_max_ranks[whichclass])
    return 0;
  switch (whichclass) {
  case CLASS_MONK:
    if (GET_CLASS_RANKS(ch, CLASS_MONK) && GET_CLASS(ch) != CLASS_MONK)
      return 0; /* You can't go back to monk if you were one and changed */
    else
  default:
    return 1; /* All other classes can be taken */
  }
}

int class_ok_prestige(struct char_data *ch, int whichclass)
{
  if (!CONFIG_ALLOW_PRESTIGE)
    return 0;

  if (whichclass < NUM_BASIC_CLASSES || whichclass >= (NUM_CLASSES - NUM_NPC_CLASSES)) {
    log("Invalid class %d in class_ok_prestige", whichclass);
    return 0;
  }
  if (!class_ok_race[(int)GET_RACE(ch)][whichclass])
    return 0;
  if (!class_ok_align[ALIGN_TYPE(ch)][whichclass])
    return 0;
  
  if (class_max_ranks[whichclass] > -1 &&
     GET_CLASS_RANKS(ch, whichclass) >= class_max_ranks[whichclass])
   return 0;
   
  switch(whichclass) {
    case CLASS_ARCANE_ARCHER:
      if (GET_BAB(ch) < 6)
         return FALSE;
      if (!HAS_FEAT(ch, FEAT_PRECISE_SHOT) && !HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_LONGBOW) &&
          !HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_SHORTBOW))
         return FALSE;
      /* Race must be elf or half elf */
      
      /* Check for 1st level arcane caster */
      return TRUE;
    case CLASS_ARCANE_TRICKSTER:
      /* Any Non-Lawful */
      if (GET_SKILL(ch, SKILL_DECIPHER_SCRIPT) < 7)
        return FALSE;
      if (GET_SKILL(ch, SKILL_DISABLE_DEVICE) < 7)
        return FALSE;
      if (GET_SKILL(ch, SKILL_ESCAPE_ARTIST) < 7)
        return FALSE;  
      /* Ability to cast mage hand and at least one arcane spell of 3rd level or higher */
      /* Knowledge (arcana) 4 ranks. */
      if (GET_SKILL(ch, SKILL_KNOW_ARCANA) < 3)
        return FALSE;  
      /* Sneak attack +2d6. */
      return TRUE; /*change this when checks are in */
    case CLASS_ARCHMAGE:
      /* Knowledge (arcana) 15 ranks. */
      if (GET_SKILL(ch, SKILL_KNOW_ARCANA) < 14)
        return FALSE;  
      /* Spellcraft 15 ranks. */
      if (GET_SKILL(ch, SKILL_SPELLCRAFT) < 14)
        return FALSE;  
      /* Feat Skill Focus (Spellcraft). */
      /* Spell Focus in two schools of magic */
      /* Ability to cast 7th level arcane spells */
      if (spell_lvlmax_table[CLASS_WIZARD][GET_LEVEL(ch)][7] < 0 && IS_WIZARD(ch))
        return FALSE;
       return TRUE;
    case CLASS_ASSASSIN:
      /* MUST BE EVIL - Checked in align check above*/
      /*if (GET_MURDER(ch) == 0)         
        return FALSE; */
      if (GET_SKILL(ch, SKILL_DISGUISE) < 4)
        return FALSE;
      if (GET_SKILL(ch, SKILL_HIDE) < 8)
        return FALSE;
      if (GET_SKILL(ch, SKILL_MOVE_SILENTLY) < 8)
        return FALSE;   
      return TRUE;
    case CLASS_BLACKGUARD:
       /* MUST BE EVIL - Checked in align check above*/
      if (GET_BAB(ch) < 6)
        return FALSE;
      /* Needs feat_power_attack but that is a requirement for cleave and improved sunder */
      if (!HAS_FEAT(ch, FEAT_CLEAVE) && !HAS_FEAT(ch, FEAT_IMPROVED_SUNDER))
        return FALSE;
      if (GET_SKILL(ch, SKILL_HIDE) < 5)
        return FALSE;
      /* Knowledge (religion) 2 ranks */        
      if (GET_SKILL(ch, SKILL_KNOW_RELIGION) < 1)
        return FALSE;  
      return TRUE;
    case CLASS_DRAGON_DISCIPLE:
      /* RACE any non dragon - Checked above */
      /* Must know Draconic language */
      if (!GET_SKILL(ch, SKILL_LANG_DRACONIC))
        return FALSE;
      /* Ability to cast arcane spells without preparation */
      /* Knowledge (arcana) 8 ranks. */
      if (GET_SKILL(ch, SKILL_KNOW_ARCANA) < 7)
        return FALSE;  
      /* The player chooses a dragon variety when taking the first level in this prestige class. */ 
      return TRUE;
    case CLASS_DUELIST:
      if (GET_BAB(ch) < 6)
        return FALSE;
       /* should have feat_dodge also but Feat mobility has that as a requirement for it */ 
      if (!HAS_FEAT(ch, FEAT_MOBILITY) && !HAS_FEAT(ch, FEAT_WEAPON_FINESSE)) 
         return FALSE;      
      if (GET_SKILL(ch, SKILL_PERFORM) < 3)
        return FALSE;
      if (GET_SKILL(ch, SKILL_TUMBLE) < 5)
        return FALSE;
      return TRUE;
    case CLASS_DWARVEN_DEFENDER:
      /* Race must be dwarf - Checked in race check above */
      /* Any lawful - checked in align check above */
      if (GET_BAB(ch) < 7)
        return FALSE;
      if (!HAS_FEAT(ch, FEAT_DODGE) && !HAS_FEAT(ch, FEAT_ENDURANCE) &&
          !HAS_FEAT(ch, FEAT_TOUGHNESS))
        return FALSE;
     return TRUE; 
    case CLASS_ELDRITCH_KNIGHT:
      if (!HAS_FEAT(ch, FEAT_MARTIAL_WEAPON_PROFICIENCY))
        return FALSE;
      /* Able to cast 3rd-level arcane spells. */
      if (spell_lvlmax_table[CLASS_WIZARD][GET_LEVEL(ch)][3] < 0 || !IS_WIZARD(ch))
        return FALSE;
      return TRUE;
    case CLASS_HIEROPHANT:
      /* Any metamagic feat. */
      /* Knowledge (religion) 15 ranks. */
      /* Able to cast 7th-level divine spells. */            
      if (spell_lvlmax_table[CLASS_CLERIC][GET_LEVEL(ch)][7] < 0 || !IS_CLERIC(ch))
        return FALSE;
      return TRUE;
    case CLASS_HORIZON_WALKER:
      /* Knowledge (geography) 8 ranks. */
      if (GET_SKILL(ch, SKILL_KNOW_GEOGRAPHY) < 7)
        return FALSE;  
      /* Endurance feat. */
      if (!HAS_FEAT(ch, FEAT_ENDURANCE))
        return FALSE;
     return TRUE;
    case CLASS_LOREMASTER:
     /* Knowledge (any two) 10 ranks in each. */
     /* Any three metamagic or item creation feats, plus Skill Focus (Knowledge [any individual Knowledge skill]). */
     /* Able to cast seven different divination spells, one of which must be 3rd level or higher. */
     return FALSE;
    case CLASS_MYSTIC_THEURGE:
     /*Knowledge (arcana) 6 ranks, Knowledge (religion) 6 ranks. */
      if (GET_SKILL(ch, SKILL_KNOW_ARCANA) < 5)
        return FALSE;  
      if (GET_SKILL(ch, SKILL_KNOW_RELIGION) < 5)
        return FALSE;  
     /* Able to cast 2nd-level divine spells and 2nd-level arcane spells. */
      if ((spell_lvlmax_table[CLASS_WIZARD][GET_LEVEL(ch)][2] < 0) || !IS_WIZARD(ch)) {
        return FALSE;
      }
      if ((spell_lvlmax_table[CLASS_CLERIC][GET_LEVEL(ch)][2] < 0) || !IS_CLERIC(ch)) {
        return FALSE;
      }
     return TRUE;
    case CLASS_SHADOWDANCER:
      if (!HAS_FEAT(ch, FEAT_COMBAT_REFLEXES) && !HAS_FEAT(ch, FEAT_DODGE) &&
          !HAS_FEAT(ch, FEAT_MOBILITY))
        return FALSE;
      if (GET_SKILL(ch, SKILL_MOVE_SILENTLY) < 8)
        return FALSE;
      if (GET_SKILL(ch, SKILL_HIDE) < 10)
        return FALSE;
      if (GET_SKILL(ch, SKILL_PERFORM) < 6)
        return FALSE;
      /* Perform (dance) 5 ranks */ 
     return TRUE;
    case CLASS_THAUMATURGIST:
     /* Holy christ this class is hard */
     /* Spell Focus (conjuration). */
     /* Able to cast lesser planar ally. */
      if (spell_lvlmax_table[CLASS_CLERIC][GET_LEVEL(ch)][4] < 0 || !IS_CLERIC(ch))
        return FALSE;
      return TRUE;
    default:
      return FALSE;
 }
}    

/* ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only WIZARDS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here. */
struct guild_info_type guild_info[6] = {

/* Kortaal */
  { CLASS_WIZARD,	3017,	SCMD_EAST	},
  { CLASS_CLERIC,	3004,	SCMD_NORTH	},
  { CLASS_ROGUE,	3027,	SCMD_EAST	},
  { CLASS_FIGHTER,	3021,	SCMD_EAST	},

/* Brass Dragon */
  { -999 /* all */ ,	5065,	SCMD_WEST	},

/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};
/* These tables hold the various level configuration setting;
 * experience points, base hit values, character saving throws.
 * They are read from a configuration file (normally etc/levels)
 * as part of the boot process.  The function load_levels() at
 * the end of this file reads in the actual values. */

const char *config_sect[NUM_CONFIG_SECTIONS+1] = {
  "version",
  "experience",
  "vernum",
  "fortitude",
  "reflex",
  "will",
  "basehit",
  "\n"
};

#define CONFIG_LEVEL_VERSION	0
#define CONFIG_LEVEL_EXPERIENCE	1
#define CONFIG_LEVEL_VERNUM	2
#define CONFIG_LEVEL_FORTITUDE	3
#define CONFIG_LEVEL_REFLEX	4
#define CONFIG_LEVEL_WILL	5
#define CONFIG_LEVEL_BASEHIT	6

char level_version[READ_SIZE];
int level_vernum = 0;
int save_classes[SAVING_WILL+1][NUM_CLASSES];
int basehit_classes[NUM_CLASSES];
int exp_multiplier;

byte object_saving_throws(int material_type, int type)
{
  switch (type) {
  case SAVING_OBJ_IMPACT:
    switch (material_type) {
    case MATERIAL_GLASS:
      return 20;
    case MATERIAL_CERAMIC:
    case MATERIAL_WOOD:
      return 35;
    case MATERIAL_ORGANIC:
      return 40;
    case MATERIAL_IRON:
      return 55;
    case MATERIAL_LEATHER:
      return 65;
    case MATERIAL_METAL:
      return 70;
    case MATERIAL_SILVER:
    case MATERIAL_STEEL:
      return 85;
    case MATERIAL_MITHRIL:
    case MATERIAL_STONE:
      return 90;
    case MATERIAL_DIAMOND:
      return 95;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_HEAT:
    switch (material_type) {
    case MATERIAL_WOOL:
      return 15;
    case MATERIAL_PAPER:
      return 20;
    case MATERIAL_COTTON:
    case MATERIAL_SATIN:
    case MATERIAL_SILK:
    case MATERIAL_BURLAP:
    case MATERIAL_VELVET:
      return 25;
    case MATERIAL_ONYX:
    case MATERIAL_CURRENCY:
      return 45;
    case MATERIAL_GLASS:
      return 55;
    case MATERIAL_PLATINUM:
      return 75;
    case MATERIAL_ADAMANTINE:
      return 85;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_COLD:
    switch (material_type) {
    case MATERIAL_GLASS:
      return 35;
    case MATERIAL_ORGANIC:
    case MATERIAL_CURRENCY:
      return 45;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_BREATH:
    switch (material_type) {
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_SPELL:
    switch (material_type) {
    default:
      return 50;
      break;
    }
  default:
    log("SYSERR: Invalid object saving throw type.");
    break;
  }
  /* Should not get here unless something is wrong. */
  return 100;
}


#define SAVE_MANUAL 0
#define SAVE_LOW 1
#define SAVE_HIGH 2

char *save_type_names[] = {
  "manual",
  "low",
  "high"
};

#define BASEHIT_MANUAL 0
#define BASEHIT_LOW     1
#define BASEHIT_MEDIUM  2
#define BASEHIT_HIGH    3

char *basehit_type_names[] = {
  "manual",
  "low",
  "medium",
  "high"
};

/* This is for the new saving throws, that slowly go up as you level
 * If save_lev == 0, use the class tables
 * otherwise ignore the class and just determine the save for this level
 * of that type of save. */
int saving_throw_lookup(int save_lev, int chclass, int savetype, int level)
{
  if (level < 0) {
    log("SYSERR: Requesting saving throw for invalid level %d!", level);
    level = 0;
  }
  if (chclass >= NUM_CLASSES || chclass < 0) {
    log("SYSERR: Requesting saving throw for invalid class %d!", chclass);
    chclass = 0;
  }

  if (!save_lev)
    save_lev = save_classes[savetype][chclass];

  switch (save_lev) {
  case SAVE_MANUAL:
    log("Save undefined for class %s", pc_class_types[chclass]);
    return 0;
  case SAVE_LOW:
    return level / 3;
    break;
  case SAVE_HIGH:
    return (level / 2) + 2;
    break;
  default:
    log("Unknown save type %d in load_levels", save_lev);
    return 0;
    break;
  }
}


/* Base hit for classes and levels.
 * If hit_type == 0, use the class tables
 * otherwise ignore the class and just determine the base hit for this level
 * of that hit_type. */
int base_hit(int hit_type, int chclass, int level)
{
  if (level < 0 ) {
    log("SYSERR: Requesting base hit for invalid level %d!", level);
    level = 0;
  }
  if (chclass >= NUM_CLASSES || chclass < 0) {
    log("SYSERR: Requesting base hit for invalid class %d!", chclass);
    chclass = 0;
  }

  if (!hit_type)
    hit_type = basehit_classes[chclass];

  switch (hit_type) {
  case BASEHIT_MANUAL:
    log("Base hit undefined for class %s", pc_class_types[chclass]);
    return 0;
  case BASEHIT_LOW:
    return level / 2;
    break;
  case BASEHIT_MEDIUM:
    return level * 3 / 4;
    break;
  case BASEHIT_HIGH:
    return level;
    break;
  default:
    log("Unknown basehit type %d in load_levels", hit_type);
    return 0;
    break;
  }
}


/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int num_attacks(struct char_data *ch, int offhand)
{
  int attk;
  if (offhand) {
    if (HAS_FEAT(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
      return 3;
    if (HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING))
      return 2;
    else
      return 1;
  }
  attk = (GET_ACCURACY_BASE(ch) + 4) / 5;
  attk = MIN(4, attk);
  attk = MAX(1, attk);
  return attk;
}

/* Class Template Attribute values were created so all default PC classes would
 * total 80 before racial modifiers. Non defaults add up to 60. */
int class_template[NUM_BASIC_CLASSES][6] = {
/* 		      S,  D,  C,  I,  W,  C */
/* Wizard 	*/ { 10, 13, 13, 18, 16, 10 },
/* Cleric 	*/ { 13, 10, 13, 10, 18, 16 },
/* Rogue 	*/ { 13, 18, 13, 16, 10, 10 },
/* Fighter 	*/ { 18, 13, 16, 10, 13, 10 },
/* Monk 	*/ { 13, 16, 13, 10, 18, 10 },
/* Paladin 	*/ { 18, 10, 13, 10, 16, 13 },
/* Sorcerer 	*/ { 10, 13, 13, 18, 16, 10 },
/* Druid 	*/ { 13, 10, 13, 10, 18, 16 },
/* Bard 	*/ { 13, 18, 13, 16, 10, 10 },
/* Ranger 	*/ { 18, 13, 13, 10, 16, 10 },
/* Barbarian 	*/ { 18, 12, 18, 10, 12, 10 }
};

/* Race Template Attribute values were created so all default PC races would
 * total 80 before racial modifiers. Non defaults add up to 60. */
int race_template[NUM_RACES][6] = {
/* 		      S,  D,  C,  I,  W,  C */
/* Human 	*/ { 14, 14, 12, 14, 14, 12 },
/* Elf  	*/ { 13, 13, 12, 14, 15, 13 },
/* Gnome 	*/ { 13, 14, 14, 15, 12, 12 },
/* Dwarf 	*/ { 15, 12, 14, 14, 13, 12 },
/* Half Elf 	*/ { 14, 14, 13, 13, 13, 13 },
/* Halfling 	*/ { 13, 15, 12, 13, 13, 14 },
/* Drow Elf 	*/ { 15, 13, 14, 12, 14, 12 },
/* Half Orc 	*/ { 16, 14, 14, 12, 12, 12 },
/* Animal 	*/ { 10, 10, 10, 10, 10, 10 },
/* Construct 	*/ { 10, 10, 10, 10, 10, 10 },
/* Demon 	*/ { 10, 10, 10, 10, 10, 10 },
/* Dragon 	*/ { 10, 10, 10, 10, 10, 10 },
/* Fish 	*/ { 10, 10, 10, 10, 10, 10 },
/* Giant 	*/ { 10, 10, 10, 10, 10, 10 },
/* Goblin 	*/ { 10, 10, 10, 10, 10, 10 },
/* Insect 	*/ { 10, 10, 10, 10, 10, 10 },
/* Orc  	*/ { 10, 10, 10, 10, 10, 10 },
/* Snake  	*/ { 10, 10, 10, 10, 10, 10 },
/* Troll  	*/ { 10, 10, 10, 10, 10, 10 },
/* Minotaur  	*/ { 10, 10, 10, 10, 10, 10 },
/* Kobold  	*/ { 10, 10, 10, 10, 10, 10 },
/* Lizardfolk  	*/ { 10, 10, 10, 10, 10, 10 },
/* Warhost  	*/ { 10, 10, 10, 10, 10, 10 },
/* Faerie 	*/ { 10, 10, 10, 10, 10, 10 },
/* Spirit 	*/ { 10, 10, 10, 10, 10, 10 }
};

void cedit_creation(struct char_data *ch)
{
  switch (CONFIG_CREATION_METHOD) {
    case CEDIT_CREATION_METHOD_3: /* Points Pool */
      ch->real_abils.str = 10;
      ch->real_abils.dex = 10;
      ch->real_abils.con = 10;
      ch->real_abils.intel = 10;
      ch->real_abils.wis = 10;
      ch->real_abils.cha = 10;
      break;
    case CEDIT_CREATION_METHOD_4: /* Racial based template */
        ch->real_abils.str = race_template[GET_RACE(ch)][0];
        ch->real_abils.dex = race_template[GET_RACE(ch)][1];
        ch->real_abils.con = race_template[GET_RACE(ch)][2];
        ch->real_abils.intel = race_template[GET_RACE(ch)][3];
        ch->real_abils.wis = race_template[GET_RACE(ch)][4];
        ch->real_abils.cha = race_template[GET_RACE(ch)][5];
      break;
    case CEDIT_CREATION_METHOD_5: /* Class based template */
        ch->real_abils.str = class_template[GET_CLASS(ch)][0];
        ch->real_abils.dex = class_template[GET_CLASS(ch)][1];
        ch->real_abils.con = class_template[GET_CLASS(ch)][2];
        ch->real_abils.intel = class_template[GET_CLASS(ch)][3];
        ch->real_abils.wis = class_template[GET_CLASS(ch)][4];
        ch->real_abils.cha = class_template[GET_CLASS(ch)][5];
      break;
    case CEDIT_CREATION_METHOD_2: /* Random rolls, player can adjust */
    case CEDIT_CREATION_METHOD_1: /* Standard random roll, system assigned */
    default:
      roll_real_abils(ch);
      break;
  }
  racial_ability_modifiers(ch);
  racial_body_parts(ch);
  ch->aff_abils = ch->real_abils;
  set_height_and_weight_by_race(ch);
}

/* Roll the 6 stats for a character... each stat is made of some combination
 * of 6-sided dice with various rolls discarded.  Each class then decides
 * which priority will be given for the best to worst stats. */

/* This character creation method is original */
void roll_real_abils(struct char_data *ch)
{
  int i, j, k, temp, temp2, total, count = 20;
  ubyte table[6];
  ubyte rolls[6];

  do {
    for (i = 0; i < 6; i++)
      table[i] = 0;
    for (i = 0; i < 6; i++) {
      for (j = 0; j < 6; j++)
        rolls[j] = rand_number(1, 6);

      switch (i) {
      case 0: /* We want one bad roll */
        /* Worst 3 out of 4 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
               MAX(rolls[0], MAX(rolls[1], MAX(rolls[2], rolls[3])));
        break;
      case 5: /* We want one very good roll */
        /* Best 3 out of 4 + best of 2 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] + rolls[4] + rolls[5] -
               (MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3]))) +
                MIN(rolls[4], rolls[5]));
        break;
      default: /* We want the rest to be 'better than average' */
        /* Best 3 out of 4 */
        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
               MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));
        break;
      }

      for (k = 0; k < 6; k++)
        if (table[k] < temp) {
	  temp2 = table[k];
	  table[k] = temp;
	  temp = temp2;
        }
    }

    switch (GET_CLASS(ch)) {
    case CLASS_WIZARD:
      ch->real_abils.intel = table[0];
      ch->real_abils.wis = table[1];
      ch->real_abils.dex = table[2];
      ch->real_abils.con = table[3];
      switch (GET_RACE(ch)) {
        case RACE_GNOME:
        case RACE_HALFLING:
        case RACE_HALF_ORC:
          ch->real_abils.cha = table[4];
          ch->real_abils.str = table[5];
          break;
        default:
          ch->real_abils.str = table[4];
          ch->real_abils.cha = table[5];
      }
      break;
    case CLASS_CLERIC:
      ch->real_abils.wis = table[0];
      switch (GET_RACE(ch)) {
        case RACE_DWARF:
        case RACE_HALF_ORC:
          ch->real_abils.str = table[1];
          ch->real_abils.cha = table[2];
          ch->real_abils.dex = table[4];
          ch->real_abils.intel = table[5];
        break;
        case RACE_HALFLING:
        case RACE_ELF:
          ch->real_abils.cha = table[1];
          ch->real_abils.str = table[2];
          ch->real_abils.dex = table[4];
          ch->real_abils.intel = table[5];
        default:
          ch->real_abils.cha = table[1];
          ch->real_abils.str = table[2];
          ch->real_abils.intel = table[4];
          ch->real_abils.dex = table[5];
        break;
      }
      ch->real_abils.con = table[3];
      break;
    case CLASS_ROGUE:
      ch->real_abils.dex = table[0];
      ch->real_abils.intel = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.str = table[3];
      ch->real_abils.cha = table[4];
      ch->real_abils.wis = table[5];
      break;
    case CLASS_FIGHTER:
      switch (GET_RACE(ch)) {
      case RACE_ELF:
        ch->real_abils.str = table[0];
        ch->real_abils.dex = table[1];
        ch->real_abils.con = table[2];
      case RACE_HALFLING:
        ch->real_abils.dex = table[0];
        ch->real_abils.con = table[1];
        ch->real_abils.str = table[2];
      default:
        ch->real_abils.str = table[0];
        ch->real_abils.con = table[1];
        ch->real_abils.dex = table[2];
        break;
      }
      ch->real_abils.cha = table[5];
      ch->real_abils.intel = table[4];
      ch->real_abils.wis = table[3];
      break;
      case CLASS_MONK:
        switch (GET_RACE(ch)) {
        case RACE_HALF_ORC:
          ch->real_abils.str = table[0];
          ch->real_abils.wis = table[1];
          ch->real_abils.dex = table[2];
        break;
        case RACE_GNOME:
        case RACE_ELF:
          ch->real_abils.dex = table[0];
          ch->real_abils.wis = table[1];
          ch->real_abils.str = table[2];
        break;
        default:
          ch->real_abils.wis = table[0];
          ch->real_abils.dex = table[1];
          ch->real_abils.str = table[2];
        break;
        }
        ch->real_abils.con = table[3];
        ch->real_abils.intel = table[4];
        ch->real_abils.cha = table[5];
        break;
      case CLASS_PALADIN:
        switch (GET_RACE(ch)) {
        case RACE_HALF_ORC:
          ch->real_abils.str = table[0];
          ch->real_abils.con = table[1];
          ch->real_abils.cha = table[2];
          ch->real_abils.wis = table[3];
          ch->real_abils.dex = table[4];
        break;
        case RACE_ELF:
          ch->real_abils.str = table[0];
          ch->real_abils.dex = table[1];
          ch->real_abils.wis = table[2];
          ch->real_abils.con = table[4];
          ch->real_abils.cha = table[3];
        case RACE_HALFLING:
          ch->real_abils.con = table[0];
          ch->real_abils.wis = table[1];
          ch->real_abils.cha = table[2];
          ch->real_abils.dex = table[4];
          ch->real_abils.str = table[3];
        break;
        default:
          ch->real_abils.str = table[0];
          ch->real_abils.con = table[1];
          ch->real_abils.wis = table[2];
          ch->real_abils.cha = table[3];
          ch->real_abils.dex = table[4];
          break;
        }
        ch->real_abils.intel = table[5];

      break;
    default:
      ch->real_abils.str   = table[0];
      ch->real_abils.dex   = table[1];
      ch->real_abils.con   = table[2];
      ch->real_abils.intel = table[3];
      ch->real_abils.wis   = table[4];
      ch->real_abils.cha   = table[5];
      break;
    }

    total = ch->real_abils.str / 2 + ch->real_abils.con / 2 +
            ch->real_abils.wis / 2 + ch->real_abils.cha / 2 +
            ch->real_abils.dex / 2 + ch->real_abils.intel / 2 ;
    total -= 30;
    /* If the character is not worth playing, reroll.
     * total == total ability_mod_value for all stats. If the totals for
     * ability mods would be 0 or less, the character sucks.
     * If the highest ability is 13, the character also sucks.
     * If the totals for ability mods would be higher than 12, the char
     * is just too powerful.
     * They get to keep the first acceptable character rolled; if no good
     * characters appear in {count} rerolls, they are stuck with the last
     * roll. */
  } while (--count && (total < 0 || table[0] < 14 || total > 12));

  if (!count)
    mudlog(NRM, ADMLVL_GOD, TRUE,
           "Unlucky new player %s has stat mods totalling %d and highest stat %d",
           GET_NAME(ch), total, table[0]);
}

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
int class_hit_die_size[NUM_CLASSES] = {
/* Wi */ 4,
/* Cl */ 8,
/* Ro */ 6,
/* Fi */ 10,
/* Mo */ 8,
/* Pa */ 10,
/* So */ 4,
/* Dr */ 8,
/* Ba */ 6,
/* Ra */ 8,
/* Ba */ 12,
/* AA */ 8,
/* AT */ 4,
/* AM */ 4,
/* AS */ 6,
/* BG */ 10,
/* DD */ 12,
/* Du */ 10,
/* Dw */ 12,
/* EK */ 6,
/* HE */ 8,
/* HW */ 8,
/* LM */ 4,
/* MT */ 4,
/* SD */ 8,
/* TH */ 4,
/* Ex */ 6,
/* Ad */ 6,
/* Co */ 4,
/* Ar */ 8,
/* Wa */ 8
};

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  int i;
  struct obj_data *obj;

  GET_CLASS_LEVEL(ch) = 1;
  GET_HITDICE(ch) = 0;
  GET_LEVEL_ADJ(ch) = 0;
  GET_CLASS_NONEPIC(ch, GET_CLASS(ch)) = 1;
  GET_EXP(ch) = 1;

  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);

  if (GET_CLASS(ch) < 0 || GET_CLASS(ch) > NUM_CLASSES) {
    log("Unknown character class %d in do_start, resetting.", GET_CLASS(ch));
    GET_CLASS(ch) = 0;
  }

  set_title(ch, NULL);
  /* roll_real_abils(ch); */

  GET_MAX_HIT(ch)  = class_hit_die_size[GET_CLASS(ch)];
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 82;
  GET_GOLD(ch) = dice(3, 6) * 10;

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (GET_RACE(ch)) {
  case RACE_HUMAN:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_ELF:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_LOW_LIGHT);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    SET_SKILL(ch, SKILL_LANG_ELVEN, 1);
    break;
  case RACE_HALF_ELF:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_LOW_LIGHT);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    SET_SKILL(ch, SKILL_LANG_ELVEN, 1);
    break;
  case RACE_GNOME:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_LOW_LIGHT);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    SET_SKILL(ch, SKILL_LANG_GNOME, 1);
    break;
  case RACE_DWARF:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_INFRAVISION);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    SET_SKILL(ch, SKILL_LANG_DWARVEN, 1);
    break;
  case RACE_HALF_ORC:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_INFRAVISION);
    SET_SKILL(ch, SKILL_LANG_ORC, 1);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_HALFLING:
    SET_SKILL(ch, SKILL_LANG_HALFLING, 1);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  default:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  }

  SPEAKING(ch) = SKILL_LANG_COMMON;

  /* assign starting items etc...*/
  switch GET_CLASS(ch) {
    case CLASS_WIZARD:
      obj = read_object(1020, VIRTUAL);
      obj_to_char(obj, ch);
      break;
    case CLASS_CLERIC:
      obj = read_object(1000, VIRTUAL);
      obj_to_char(obj, ch);
      break;
    case CLASS_ROGUE:
      obj = read_object(1022, VIRTUAL);
      obj_to_char(obj, ch);
      break;
    case CLASS_MONK:
      GET_ETHIC_ALIGNMENT(ch) = 1000;
      break;
    case CLASS_PALADIN:
      GET_ALIGNMENT(ch) = 1000;
      GET_ETHIC_ALIGNMENT(ch) = 1000;
      break;
    default:
      break;
  }

  if (GET_CLASS(ch) == CLASS_DRUID)
    SET_SKILL(ch, SKILL_LANG_DRUIDIC, 1);

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    SET_SKILL_CLASS(ch, i, spell_info[i].can_learn_skill[GET_CLASS(ch)]);
  }

  advance_level(ch, GET_CLASS(ch));
  mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));

  GET_MAX_HIT(ch) += 10;
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, HUNGER) = 24;
  GET_COND(ch, DRUNK) = 0;

  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);

  ch->player_specials->olc_zone = NOWHERE;
}


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_wizard[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_SCRIBE_SCROLL,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_sorcerer[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_cleric[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_rogue[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_fighter[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_paladin[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_barbarian[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_bard[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_ranger[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_monk[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_STUNNING_FIST,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int free_start_feats_druid[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int no_free_start_feats[] = {
 0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int *free_start_feats[] = {
 /* CLASS_WIZARD	*/ free_start_feats_wizard,
 /* CLASS_CLERIC	*/ free_start_feats_cleric,
 /* CLASS_ROGUE		*/ free_start_feats_rogue,
 /* CLASS_FIGHTER	*/ free_start_feats_fighter,
 /* CLASS_MONK		*/ free_start_feats_monk,
 /* CLASS_PALADIN	*/ free_start_feats_paladin,
 /* CLASS_SORCERER      */ free_start_feats_sorcerer,
 /* CLASS_DRUID         */ free_start_feats_druid,
 /* CLASS_BARD          */ free_start_feats_bard,
 /* CLASS_RANGER        */ free_start_feats_ranger,
 /* CLASS_BARBARIAN     */ free_start_feats_barbarian,
 /* CLASS_ARCANE_AR	*/ no_free_start_feats,
 /* CLASS_ARCANE_TR	*/ no_free_start_feats,
 /* CLASS_ARCHMAGE	*/ no_free_start_feats,
 /* CLASS_ASSASSIN	*/ no_free_start_feats,
 /* CLASS_BLACKGUARD	*/ no_free_start_feats,
 /* CLASS_DRAGON_D 	*/ no_free_start_feats,
 /* CLASS_DUELIST       */ no_free_start_feats,
 /* CLASS_DWARVEN_DEF   */ no_free_start_feats,
 /* CLASS_ELDRITCH_KN   */ no_free_start_feats,
 /* CLASS_HIEROPHANT    */ no_free_start_feats,
 /* CLASS_HORIZON_WALK  */ no_free_start_feats,
 /* CLASS_LOREMASTER    */ no_free_start_feats,
 /* CLASS_MYSTIC_THEU   */ no_free_start_feats,
 /* CLASS_SHADOWDANCER  */ no_free_start_feats,
 /* CLASS_THAUMATURGIST */ no_free_start_feats
};

/* This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level. */
void advance_level(struct char_data *ch, int whichclass)
{
  struct levelup_data *llog;
  int add_hp, add_move = 0, add_mana = 0, add_prac = 1, add_ki = 0, add_train, i, j, ranks;
  int add_acc = 0, add_fort = 0, add_reflex = 0, add_will = 0;
  int add_gen_feats = 0, add_class_feats = 0;
  char buf[MAX_STRING_LENGTH];

  if (whichclass < 0 || whichclass >= NUM_CLASSES) {
    log("Invalid class %d passed to advance_level, resetting.", whichclass);
    whichclass = 0;
  }

  if (!CONFIG_ALLOW_MULTICLASS && whichclass != GET_CLASS(ch)) {
    log("Attempt to gain a second class without multiclass enabled for %s", GET_NAME(ch));
    whichclass = GET_CLASS(ch);
  }
  ranks = GET_CLASS_RANKS(ch, whichclass);

  CREATE(llog, struct levelup_data, 1);
  llog->next = ch->level_info;
  llog->prev = NULL;
  if (llog->next)
    llog->next->prev = llog;
  ch->level_info = llog;

  llog->skills = llog->feats = NULL;
  llog->type = LEVELTYPE_CLASS;
  llog->spec = whichclass;
  llog->level = GET_LEVEL(ch);

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (ranks) {
  case 1:
    for (i = 0; (j = free_start_feats[whichclass][i]); i++) {
      HAS_FEAT(ch, j) = 1;
    }
    switch (whichclass) {
      case CLASS_MONK:
        add_innate_timer(ch, FEAT_STUNNING_FIST, SKTYPE_FEAT);
        break;
    }
    break;
  case 2:
    switch (whichclass) {
    case CLASS_ROGUE:
      HAS_FEAT(ch, FEAT_EVASION) = 1;
      break;
    case CLASS_MONK:
      HAS_FEAT(ch, FEAT_COMBAT_REFLEXES) = 1;
      HAS_FEAT(ch, FEAT_EVASION) = 1;
      break;
    }
    break;
  case 6:
    switch (whichclass) {
    case CLASS_MONK:
      HAS_FEAT(ch, FEAT_IMPROVED_DISARM) = 1;
      break;
    }
    break;
  case 9:
    switch (whichclass) {
    case CLASS_MONK:
      HAS_FEAT(ch, FEAT_IMPROVED_EVASION) = 1;
      break;
    }
    break;
  }

  if (GET_CLASS_LEVEL(ch) == 1 && GET_HITDICE(ch) < 2) { /* Filled in below */
      GET_HITDICE(ch) = 0;
      GET_MAX_HIT(ch) = 0;
      GET_ACCURACY_BASE(ch) = 0;
      GET_SAVE_BASE(ch, SAVING_FORTITUDE) = 0;
      GET_SAVE_BASE(ch, SAVING_REFLEX) = 0;
      GET_SAVE_BASE(ch, SAVING_WILL) = 0;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART) { /* Epic character */
    if (GET_LEVEL(ch) % 2) {
      add_acc = 1;
    } else {
      add_fort = 1;
      add_reflex = 1;
      add_will = 1;
    }
  } else if (ranks == 1) { /* First level of a given class */
    add_fort = saving_throw_lookup(0, whichclass, SAVING_FORTITUDE, 1);
    add_reflex = saving_throw_lookup(0, whichclass, SAVING_REFLEX, 1);
    add_will = saving_throw_lookup(0, whichclass, SAVING_WILL, 1);
    add_acc = base_hit(0, whichclass, 1);
  } else { /* Normal level of a non-epic class */
    add_fort = saving_throw_lookup(0, whichclass, SAVING_FORTITUDE, ranks) -
               saving_throw_lookup(0, whichclass, SAVING_FORTITUDE, ranks - 1);
    add_reflex = saving_throw_lookup(0, whichclass, SAVING_REFLEX, ranks) -
                 saving_throw_lookup(0, whichclass, SAVING_REFLEX, ranks - 1);
    add_will = saving_throw_lookup(0, whichclass, SAVING_WILL, ranks) -
               saving_throw_lookup(0, whichclass, SAVING_WILL, ranks - 1);
    add_acc = base_hit(0, whichclass, ranks) - base_hit(0, whichclass, ranks - 1);
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (ranks >= LVL_EPICSTART) { /* Epic class */
    j = ranks - 20;
    switch (whichclass) {
    case CLASS_ROGUE:
    case CLASS_ARCANE_ARCHER:
    case CLASS_DRAGON_DISCIPLE:
    case CLASS_ELDRITCH_KNIGHT:
    case CLASS_HORIZON_WALKER:
      if (!(j % 4))
        add_class_feats++;
      break;
    case CLASS_FIGHTER:
      if (!(j % 2))
        add_class_feats++;
      break;
    case CLASS_MONK:
      if (!(j % 5))
        add_class_feats++;
      break;
    case CLASS_NPC_EXPERT:
    case CLASS_NPC_ADEPT:
    case CLASS_NPC_COMMONER:
    case CLASS_NPC_ARISTOCRAT:
    case CLASS_NPC_WARRIOR:
      break;
    case CLASS_DWARVEN_DEFENDER:
    case CLASS_MYSTIC_THEURGE:
     if (!(j % 6))
       add_class_feats++;
      break;
    default:
      if (!(j % 3))
        add_class_feats++;
      break;
    }
  } else {
    switch (whichclass) {
    case CLASS_FIGHTER:
      if (ranks == 1 || !(ranks % 2))
        add_class_feats++;
      break;
    case CLASS_ROGUE:
      if (ranks > 9 && !(ranks % 3))
        add_class_feats++;
      break;
    case CLASS_WIZARD:
      if (!(ranks % 5))
        add_class_feats++;
      break;
    default:
      break;
    }
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (whichclass) {
  case CLASS_SORCERER:
  case CLASS_WIZARD:
    add_move = rand_number(0, 2);
    add_prac = 2 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_CLERIC:
    add_move = rand_number(0, 2);
    add_prac = 2 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_ROGUE:
    add_move = rand_number(1, 3);
    add_prac = 8 + ability_mod_value(GET_INT(ch));
    if (ranks % 2)
      HAS_FEAT(ch, FEAT_SNEAK_ATTACK) += 1;
    break;

  case CLASS_FIGHTER:
    add_move = rand_number(1, 3);
    add_prac = 2 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_MONK:
    add_move = rand_number(ranks, (int)(1.5 * ranks));
    add_prac = 4 + ability_mod_value(GET_INT(ch));
    add_ki = 10 + ability_mod_value(GET_WIS(ch));
    break;

  case CLASS_PALADIN:
    add_move = rand_number(1, 3);
    add_prac = 2 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_BARBARIAN:
    add_move = rand_number(1, 3);
    add_prac = 4 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_BARD:
    add_move = rand_number(1, 3);
    add_prac = 6 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_RANGER:
    add_move = rand_number(1, 3);
    add_prac = 6 + ability_mod_value(GET_INT(ch));
    break;


  case CLASS_DRUID:
    add_move = rand_number(1, 3);
    add_prac = 4 + ability_mod_value(GET_INT(ch));
    break;

  case CLASS_ARCANE_ARCHER:
    add_prac = 4 + ability_mod_value(GET_INT(ch));
    break;    
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_LEVEL(ch) == 1 || !(GET_LEVEL(ch) % 3)) {
    add_gen_feats += 1;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_RACE(ch) == RACE_HUMAN) /* Humans get 4 extra at level 1 and  */
    add_prac += 1;                /* 1 extra per level for adaptability */

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  i = ability_mod_value(GET_CON(ch));
  if (GET_LEVEL(ch) > 1) {
    j = rand_number(1, class_hit_die_size[whichclass]);
    add_hp = MAX(1, i + j);
  } else {
    j = class_hit_die_size[whichclass];
    add_hp = MAX(1, i + j);
    GET_MAX_HIT(ch) = 0; /* Just easier this way */
    add_prac *= 4;
  }
  llog->hp_roll = j;

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  add_train = (GET_LEVEL(ch) % 4) ? 0 : 1;
  if (add_train) {
    GET_TRAINS(ch) += add_train;
  }

  llog->mana_roll = add_mana;
  llog->move_roll = add_move;
  llog->ki_roll = add_ki;
  llog->add_skill = add_prac;
  add_prac = MAX(1, add_prac);
  GET_PRACTICES(ch, whichclass) += add_prac;
  GET_MAX_HIT(ch) += add_hp;
  GET_MAX_MOVE(ch) += add_move;
  GET_MAX_KI(ch) += add_ki;

  if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART) { /* Epic character */
    GET_EPIC_FEAT_POINTS(ch) += add_gen_feats;
    llog->add_epic_feats = add_gen_feats;
    GET_EPIC_CLASS_FEATS(ch, whichclass) += add_class_feats;
    llog->add_class_epic_feats = add_class_feats;
  } else {
    GET_FEAT_POINTS(ch) += add_gen_feats;
    llog->add_gen_feats = add_gen_feats;
    GET_CLASS_FEATS(ch, whichclass) += add_class_feats;
    llog->add_class_feats = add_class_feats;
  }

  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  if (add_ki)
    send_to_char(ch, "You gain %d hit, %d move, and %d ki.\r\n", add_hp, add_move, add_ki);
  else
    send_to_char(ch, "You gain %d hit and %d move.\r\n", add_hp, add_move);

  if (whichclass == CLASS_MONK) {
    buf[0] = 0;
    j = 0;
    for (i = 0; i < SKILL_TABLE_SIZE; i++)
      if (IS_SET(spell_info[i].skilltype, SKTYPE_ART))
        if (ranks >= spell_info[i].min_level[CLASS_MONK] && !GET_SKILL(ch, i)) {
          if (j)
            strncat(buf, ", ", sizeof(buf)-1);
          strncat(buf, spell_info[i].name, sizeof(buf)-1);
          SET_SKILL(ch, i, 1);
          j += 1;
        }
    if (j) {
      send_to_char(ch, "You gain the following abilit%s, usable via the \"art\" command:%s%s\r\n",
                   j > 1 ? "ies" : "y", j > 1 ? "\r\n" : " ", buf);
    }
  }
  llog->accuracy = add_acc;
  llog->fort = add_fort;
  llog->reflex = add_reflex;
  llog->will = add_will;
  GET_ACCURACY_BASE(ch) += add_acc;
  GET_SAVE_BASE(ch, SAVING_FORTITUDE) += add_fort;
  GET_SAVE_BASE(ch, SAVING_REFLEX) += add_reflex;
  GET_SAVE_BASE(ch, SAVING_WILL) += add_will;
  snoop_check(ch);
  save_char(ch);
}


/* How many +1d6 dice does the character get for sneak attacks? */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int backstab_dice(struct char_data *ch)
{
  int cmdice;
  if (!IS_NPC(ch)) {
    cmdice = HAS_FEAT(ch, FEAT_SNEAK_ATTACK);
    if (cmdice < 0)
      return 0;
    return cmdice;
  } else {
    if (GET_CLASS(ch) == CLASS_ROGUE)
      return (GET_LEVEL(ch) + 1) / 2;
    else
      return 0;
  }
}


/* invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors. */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_WIZARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_FIGHTER) && IS_FIGHTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE) && IS_ROGUE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_WIZARD) && !IS_WIZARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_CLERIC) && !IS_CLERIC(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_ROGUE) && !IS_ROGUE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_FIGHTER) && !IS_FIGHTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_PALADIN) && !IS_PALADIN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_MONK) && !IS_MONK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_WIZARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_PALADIN) && IS_PALADIN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARBARIAN) && IS_BARBARIAN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MONK) && IS_MONK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARD) && IS_BARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANE_ARCHER) && IS_ARCANE_ARCHER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANE_TRICKSTER) && IS_ARCANE_TRICKSTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ARCHMAGE) && IS_ARCHMAGE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BLACKGUARD) && IS_BLACKGUARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRAGON_DISCIPLE) && IS_DRAGON_DISCIPLE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DUELIST) && IS_DUELIST(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DWARVEN_DEFENDER) && IS_DWARVEN_DEFENDER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ELDRITCH_KNIGHT) && IS_ELDRITCH_KNIGHT(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HIEROPHANT) && IS_HIEROPHANT(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HORIZON_WALKER) && IS_HORIZON_WALKER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_LOREMASTER) && IS_LOREMASTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MYSTIC_THEURGE) && IS_MYSTIC_THEURGE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_SHADOWDANCER) && IS_SHADOWDANCER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_THAUMATURGIST) && IS_THAUMATURGIST(ch))
    return TRUE;


  return FALSE;
}


/* SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill. */

void init_skill_race_classes(void)
{
  /* HUMANS */
  skill_race_class(SKILL_LANG_COMMON, RACE_HUMAN, SKLEARN_BOOL);
  /* ELVES */
  skill_race_class(SKILL_LANG_COMMON, RACE_ELF, SKLEARN_BOOL);
  skill_race_class(SKILL_LANG_ELVEN, RACE_ELF, SKLEARN_BOOL);
  /* GNOMES */
  skill_race_class(SKILL_LANG_COMMON, RACE_GNOME, SKLEARN_BOOL);
  skill_race_class(SKILL_LANG_GNOME, RACE_GNOME, SKLEARN_BOOL);
  /* DWARVES */
  skill_race_class(SKILL_LANG_COMMON, RACE_DWARF, SKLEARN_BOOL);
  skill_race_class(SKILL_LANG_DWARVEN, RACE_DWARF, SKLEARN_BOOL);

}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
void init_skill_classes(void)
{
  /* WIZARDS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_WIZARD, SKLEARN_CANT);
  skill_class(SKILL_CRAFT_ALCHEMY, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_CONCENTRATION, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_WIZARD, SKLEARN_CANT);
  skill_class(SKILL_SPELLCRAFT, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_ARCANA, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_ARCHITECTURE, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_DUNGEONEERING, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_GEOGRAPHY, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_HISTORY, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_LOCAL, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_NATURE, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_NOBILITY, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_RELIGION, CLASS_WIZARD, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_PLANES, CLASS_WIZARD, SKLEARN_CLASS);

  /* CLERICS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_CLERIC, SKLEARN_CANT);
  skill_class(SKILL_CONCENTRATION, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_CLERIC, SKLEARN_CANT);
  skill_class(SKILL_HEAL, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_ARCANA, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_HISTORY, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_RELIGION, CLASS_CLERIC, SKLEARN_CLASS);
  skill_class(SKILL_KNOW_PLANES, CLASS_CLERIC, SKLEARN_CLASS);

  /* ROGUES */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_ROGUE, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_TUMBLE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_BALANCE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_CLIMB, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SPOT, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DISABLE_DEVICE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DISGUISE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_APPRAISE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_FORGERY, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_HIDE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_INTIMIDATE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_JUMP, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_LISTEN, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_GATHER_INFORMATION, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_OPEN_LOCK, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SEARCH, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_MOVE_SILENTLY, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SLEIGHT_OF_HAND, CLASS_ROGUE, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_ROGUE, SKLEARN_CLASS);

  /* FIGHTERS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_FIGHTER, SKLEARN_CANT);
  skill_class(SKILL_CLIMB, CLASS_FIGHTER, SKLEARN_CLASS);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_FIGHTER, SKLEARN_CANT);
  skill_class(SKILL_SURVIVAL, CLASS_FIGHTER, SKLEARN_CANT);
  skill_class(SKILL_JUMP, CLASS_FIGHTER, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_FIGHTER, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_FIGHTER, SKLEARN_CLASS);
  skill_class(SKILL_USE_ROPE, CLASS_FIGHTER, SKLEARN_CLASS);

  /* MONKS */
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_MONK, SKLEARN_CANT);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_MONK, SKLEARN_CANT);
  skill_class(SKILL_TUMBLE, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_MONK, SKLEARN_CANT);
  skill_class(SKILL_BALANCE, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_CLIMB, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_CONCENTRATION, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_HIDE, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_JUMP, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_LISTEN, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_MOVE_SILENTLY, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_MONK, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_MONK, SKLEARN_CLASS);

  /* PALADINS */
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_PALADIN, SKLEARN_CANT);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_PALADIN, SKLEARN_CANT);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_PALADIN, SKLEARN_CANT);
  skill_class(SKILL_CONCENTRATION, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_USE_ROPE, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_PALADIN, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_PALADIN, SKLEARN_CLASS);

  /* BARBARIANS */
  skill_class(SKILL_CLIMB, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_INTIMIDATE, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_JUMP, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_LISTEN, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_SURVIVAL, CLASS_BARBARIAN, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_BARBARIAN, SKLEARN_CLASS);

  /* BARDS */
  skill_class(SKILL_APPRAISE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_BALANCE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_BLUFF, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_CLIMB, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_DECIPHER_SCRIPT, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_DIPLOMACY, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_DISGUISE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_ESCAPE_ARTIST, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_HIDE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_JUMP, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_LISTEN, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_MOVE_SILENTLY, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_PERFORM, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SENSE_MOTIVE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SLEIGHT_OF_HAND, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_TUMBLE, CLASS_BARD, SKLEARN_CLASS);
  skill_class(SKILL_USE_MAGIC_DEVICE, CLASS_BARD, SKLEARN_CLASS);

  /* DRUIDS */
  skill_class(SKILL_DIPLOMACY, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_LISTEN, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_SPELLCRAFT, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_SPOT, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_SURVIVAL, CLASS_DRUID, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_DRUID, SKLEARN_CLASS);
  
  /* RANGERS */
  skill_class(SKILL_CLIMB, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_HANDLE_ANIMAL, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_HEAL, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_HIDE, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_JUMP, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_LISTEN, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_MOVE_SILENTLY, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_RIDE, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_SEARCH, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_SPOT, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_SURVIVAL, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_SWIM, CLASS_RANGER, SKLEARN_CLASS);
  skill_class(SKILL_USE_ROPE, CLASS_RANGER, SKLEARN_CLASS);

}


/* Function to return the exp required for each class/level */
int level_exp(int level)
{
  switch (level) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return exp_multiplier;
  default:
    return exp_multiplier * (level * (level - 1)) / 2;
  }
  return 0;
}


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
sbyte ability_mod_value(int abil)
{
  return ((int)(abil / 2)) - 5;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
sbyte dex_mod_capped(const struct char_data *ch)
{
  sbyte mod;
  struct obj_data *armor;
  mod = ability_mod_value(GET_DEX(ch));
  armor = GET_EQ(ch, WEAR_BODY);
  if (armor && GET_OBJ_TYPE(armor) == ITEM_ARMOR) {
    mod = MIN(mod, GET_OBJ_VAL(armor, VAL_ARMOR_MAXDEXMOD));
  }
  return mod;
}

int cabbr_ranktable[NUM_CLASSES];

int comp_rank(const void *a, const void *b)
{
  int first, second;
  first = *(const int *)a;
  second = *(const int *)b;
  return cabbr_ranktable[second] - cabbr_ranktable[first];
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
char *class_desc_str(struct char_data *ch, int howlong, int wantthe)
{
  static char str[MAX_STRING_LENGTH];
  char *ptr = str;
  int i, rank, j;
  int rankorder[NUM_CLASSES];
  char *buf, *buf2, *buf3;

  if (IS_NPC(ch)) {
    snprintf(ptr, sizeof(str) - (ptr - str), "%s%d", class_abbrevs[GET_CLASS(ch)], GET_LEVEL(ch) );
    return str;
  }

  if (wantthe)
    ptr += sprintf(str, "the ");

  if (howlong) {
    buf2 = buf = buf3 = "";
    if (howlong == 2) {
      buf3 = " ";
      if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART)
        ptr += sprintf(ptr, "Epic ");
    }
    for (i = 0; i < NUM_CLASSES; i++) {
      cabbr_ranktable[i] = GET_CLASS_RANKS(ch, i);
      rankorder[i] = i;
    }
    rankorder[0] = GET_CLASS(ch); /* we always want primary class first */
    rankorder[GET_CLASS(ch)] = 0;
    qsort((void *)rankorder, NUM_CLASSES, sizeof(int), comp_rank);
    for (i = 0; i < NUM_CLASSES; i++) {
      rank = rankorder[i];
      if (cabbr_ranktable[rank] == 0)
        continue;
      ptr += snprintf(ptr, sizeof(str) - (ptr - str), "%s%s%s%s%s%d", buf, buf2, buf,
                      (howlong == 2 ? pc_class_types : class_abbrevs)[rank], buf3,
                      cabbr_ranktable[rank]);
      buf2 = "/";
      if (howlong == 2)
        buf = " ";
    }
    return str;
  } else {
    rank = GET_CLASS_RANKS(ch, GET_CLASS(ch));
    j = GET_CLASS(ch);
    for (i = 0; i < NUM_CLASSES; i++)
      if (GET_CLASS_RANKS(ch, i) > rank) {
        j = i;
        rank = GET_CLASS_RANKS(ch, j);
      }
    rank = GET_CLASS_RANKS(ch, GET_CLASS(ch));
    snprintf(ptr, sizeof(str) - (ptr - str), "%s%d%s", class_names[GET_CLASS(ch)],
             rank, GET_LEVEL(ch) == rank ? "" : "+");
    return str;
  }
}


int total_skill_levels(struct char_data *ch, int skill)
{
  int i = 0, j, total = 0;
  for (i = 0; i < NUM_CLASSES; i++) {
    j = 1 + GET_CLASS_RANKS(ch, i) - spell_info[skill].min_level[i];
    if (j > 0)
     total += j;
  }
  return total;
}


int load_levels()
{
  FILE *fp;
  char line[READ_SIZE], sect_name[READ_SIZE] = { '\0' }, *ptr;
  int  linenum = 0, tp, cls, sect_type = -1;

  if (!(fp = fopen(LEVEL_CONFIG, "r"))) {
    log("SYSERR: Could not open level configuration file, error: %s!", 
         strerror(errno));
    return -1;
  }

  for (cls = 0; cls < NUM_CLASSES; cls++) {
    for (tp = 0; tp <= SAVING_WILL; tp++) {
      save_classes[tp][cls] = 0;
    }
    basehit_classes[cls] = 0;
  }

  for (;;) {
    linenum++;
    if (!fgets(line, READ_SIZE, fp)) {  /* eof check */
      log("SYSERR: Unexpected EOF in file %s.", LEVEL_CONFIG);
      return -1;
    } else if (*line == '$') { /* end of file */
      break;
    } else if (*line == '*') { /* comment line */
      continue;
    } else if (*line == '#') { /* start of a section */
      if ((tp = sscanf(line, "#%s", sect_name)) != 1) {
        log("SYSERR: Format error in file %s, line number %d - text: %s.", 
             LEVEL_CONFIG, linenum, line);
        return -1;
      } else if ((sect_type = search_block(sect_name, config_sect, FALSE)) == -1) {
          log("SYSERR: Invalid section in file %s, line number %d: %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
      }
    } else {
      if (sect_type == CONFIG_LEVEL_VERSION) {
        if (!strncmp(line, "Suntzu", 6)) {
          log("SYSERR: Suntzu %s config files are not compatible with rasputin", LEVEL_CONFIG);
          return -1;
        } else {
          strcpy(level_version, line); /* OK - both are READ_SIZE */
        }
      } else if (sect_type == CONFIG_LEVEL_VERNUM) {
	level_vernum = atoi(line);
      } else if (sect_type == CONFIG_LEVEL_EXPERIENCE) {
        tp = atoi(line);
        exp_multiplier = tp;
      } else if ((sect_type >= CONFIG_LEVEL_FORTITUDE && sect_type <= CONFIG_LEVEL_WILL) ||
                 sect_type == CONFIG_LEVEL_BASEHIT) {
        for (ptr = line; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (!ptr || !*ptr || !isdigit(*ptr)) {
          log("SYSERR: Cannot find class number in file %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        cls = atoi(ptr);
        for (; ptr && *ptr && isdigit(*ptr); ptr++);
        for (; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (ptr && *ptr && !isdigit(*ptr)) {
          log("SYSERR: Non-numeric entry in file %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (ptr && *ptr) /* There's a value */
          tp = atoi(ptr);
        else {
          log("SYSERR: Need 1 value in %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (cls < 0 || cls >= NUM_CLASSES) {
          log("SYSERR: Invalid class number %d in file %s, line number %d.", 
              cls, LEVEL_CONFIG, linenum);
          return -1;
        } else {
          if (sect_type == CONFIG_LEVEL_BASEHIT) {
            basehit_classes[cls] = tp;
          } else {
            save_classes[SAVING_FORTITUDE + sect_type - CONFIG_LEVEL_FORTITUDE][cls] = tp;
          }
        }
      } else {
        log("Unsupported level config option");
      }
    }
  }
  fclose(fp);

  /*for (cls = 0; cls < NUM_CLASSES; cls++) 
    log("Base hit for class %s: %s", class_names[cls], basehit_type_names[basehit_classes[cls]]);

  for (cls = 0; cls < NUM_CLASSES; cls++)
    log("Saves for class %s: fort=%s, reflex=%s, will=%s", class_names[cls],
        save_type_names[save_classes[SAVING_FORTITUDE][cls]],
        save_type_names[save_classes[SAVING_REFLEX][cls]],
        save_type_names[save_classes[SAVING_WILL][cls]]); */

  return 0;
}


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int highest_skill_value(int level, int type)
{
  switch (type) {
    case SKLEARN_CROSSCLASS:
      return (level + 3) / 2;
    case SKLEARN_CLASS:
      return level + 3;
    case SKLEARN_BOOL:
      return 1;
    case SKLEARN_CANT:
    default:
      return 0;
  }
}


/* This returns the number of classes that should be penalized. */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int exp_penalty(struct char_data *ch)
{
  int high, highclass, i, fcl, pen;
  fcl = favored_class[GET_RACE(ch)]; /* This class is skipped for penalties */
  if (fcl == -1) { /* -1 means find highest and use it as 'favored' class */
    for (high = fcl = i = 0; i < NUM_CLASSES; i++) {
      if (GET_CLASS_RANKS(ch, i) > high) {
        fcl = i;
        high = GET_CLASS_RANKS(ch, i);
      }
    }
  }
  for (high = highclass = i = 0; i < NUM_CLASSES; i++) {
    /* Favored class and prestige classes don't count */
    if (i == fcl || prestige_classes[i])
      continue;
    if (GET_CLASS_RANKS(ch, i) > high) {
      highclass = i;
      high = GET_CLASS_RANKS(ch, i);
    }
  }
  /* OK, we know their favored class and highest class, now we can figure
   * out the penalty */
  pen = 0;
  for (i = 0; i < NUM_CLASSES; i++) {
    if (i == fcl || prestige_classes[i] || !GET_CLASS_RANKS(ch, i))
      continue;
    if (GET_CLASS_RANKS(ch, i) < (high - 1))
      pen++;
  }
  return pen;
}

/* -20% for each class 2 or more less than highest class, ignoring favored
 * class and any prestige classes */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int calc_penalty_exp(struct char_data *ch, int gain)
{
  int pen;
  int a = 1, b = 1;
  for (pen = exp_penalty(ch); pen > 0; pen--) {
    a *= 4;
    b *= 5;
  }
  gain *= a;
  gain /= b;
  return gain;
}


int size_scaling_table[NUM_SIZES][4] = {
/*                   str       dex     con  nat arm */
/* Fine		*/ { -10,	-2,	-2,	0 },
/* Diminutive	*/ { -10,	-2,	-2,	0 },
/* Tiny		*/ { -8,	-2,	-2,	0 },
/* Small	*/ { -4,	-2,	-2,	0 },
/* Medium	*/ { 0,		0,	0,	0 },
/* Large	*/ { 8,		-2,	4,	2 },
/* Huge		*/ { 16,	-4,	8,	5 },
/* Gargantuan	*/ { 24,	-4,	12,	9 },
/* Colossal	*/ { 32,	-4,	16,	14 }
};


/* Assign stats to a mob based on race/class/size/level */
/* Original algorithm */
void assign_auto_stats(struct char_data *ch)
{
  struct abil_data *ab;
  int sz, extra, tmp;

  if (!ch)
    return;

  ab = &ch->real_abils;
 
  ab->intel = ab->wis = ab->str = ab->con = ab->dex = ab->cha = 10;
  racial_ability_modifiers(ch);

  sz = get_size(ch);
  
  ab->str += size_scaling_table[sz][0];
  ab->dex += size_scaling_table[sz][1];
  ab->con += size_scaling_table[sz][2];

  extra = GET_LEVEL(ch) / 4; /* For PC's it's every 4, we want to make it compensate */

  switch (GET_CLASS(ch)) {
  case CLASS_WIZARD:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->intel += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->dex > ab->str)
      ab->dex += extra;
    else
      ab->str += extra;
    break;
  case CLASS_CLERIC:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->wis += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    ab->cha += extra;
    break;
  case CLASS_ROGUE:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->dex += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->intel += tmp;
    if (ab->con > ab->str)
      ab->con += extra;
    else
      ab->str += extra;
    break;
  case CLASS_MONK:
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->wis += tmp;
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->str > ab->dex)
      ab->str += extra;
    else
      ab->dex += extra;
    break;
  case CLASS_PALADIN:
    tmp = extra / 2;
    extra -= tmp;
    if (ab->str > ab->dex)
      ab->str += tmp;
    else
      ab->dex += tmp;
    tmp = (extra + 1) / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->wis > ab->cha)
      ab->wis += extra;
    else
      ab->cha += extra;
    break;
  case CLASS_FIGHTER:
  default:
    tmp = extra / 2;
    extra -= tmp;
    ab->con += tmp;
    if (ab->dex > ab->str)
      ab->dex += extra;
    else
      ab->str += extra;
    break;
  }

  ch->aff_abils = ch->real_abils;
  return;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
time_t birth_age(struct char_data *ch)
{
  int cltype;
  struct aging_data *aging;
  int tmp;

  switch (GET_CLASS(ch)) {
  case CLASS_WIZARD:
  case CLASS_CLERIC:
  case CLASS_MONK:
    cltype = 2;
    break;
  case CLASS_FIGHTER:
  case CLASS_PALADIN:
    cltype = 1;
    break;
  case CLASS_ROGUE:
  default:
    cltype = 0;
    break;
  }
  aging = racial_aging_data + GET_RACE(ch);

  tmp = aging->adult + dice(aging->classdice[cltype][0], aging->classdice[cltype][1]);
  tmp *= SECS_PER_MUD_YEAR;
  tmp += rand_number(0, SECS_PER_MUD_YEAR);

  return tmp;
}

time_t max_age(struct char_data *ch)
{
  struct aging_data *aging;
  size_t tmp;

  if (ch->time.maxage)
    return ch->time.maxage - ch->time.birth;

  aging = racial_aging_data + GET_RACE(ch);

  tmp = aging->venerable + dice(aging->maxdice[0], aging->maxdice[1]);
  tmp *= SECS_PER_MUD_YEAR;
  tmp += rand_number(0, SECS_PER_MUD_YEAR);

  return tmp;
}

/* Returns:
 * 0 on failure
 * 1 on raising normal class level
 * 2 on raising epic class level
 * 3 on raising both */
int raise_class_only(struct char_data *ch, int cl, int v)
{
  if (cl < 0 || cl >= NUM_CLASSES)
    return 0;
  if (GET_CLASS_LEVEL(ch) + v < LVL_EPICSTART) {
    GET_CLASS_NONEPIC(ch, cl) += v;
    return 1;
  }
  if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART - 1) {
    GET_CLASS_EPIC(ch, cl) += v;
    return 2;
  }
  GET_CLASS_NONEPIC(ch, cl) += (v -= (LVL_EPICSTART - (1 + GET_CLASS_LEVEL(ch))));
  GET_CLASS_EPIC(ch, cl) += v;
  return 3;
}

const int class_feats_wizard[] = {
  FEAT_SPELL_MASTERY,
  FEAT_BREW_POTION,
  FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR,
  FEAT_CRAFT_ROD,
  FEAT_CRAFT_STAFF,
  FEAT_CRAFT_WAND,
  FEAT_CRAFT_WONDEROUS_ITEM,
  FEAT_FORGE_RING,
  FEAT_SCRIBE_SCROLL,
  FEAT_EMPOWER_SPELL,
  FEAT_ENLARGE_SPELL,
  FEAT_EXTEND_SPELL,
  FEAT_HEIGHTEN_SPELL,
  FEAT_MAXIMIZE_SPELL,
  FEAT_QUICKEN_SPELL,
  FEAT_SILENT_SPELL,
  FEAT_STILL_SPELL,
  FEAT_WIDEN_SPELL,
  FEAT_UNDEFINED
};

/* Rogues follow opposite logic - they can take any feat in place of these,
 * all of these are abilities that are not normally able to be taken as
 * feats. Most classes can ONLY take from these lists for their class
 * feats. */
const int class_feats_rogue[] = {
  FEAT_IMPROVED_EVASION,
  FEAT_CRIPPLING_STRIKE,
  FEAT_DEFENSIVE_ROLL,
  FEAT_IMPROVED_EVASION,
  FEAT_OPPORTUNIST,
  FEAT_SKILL_MASTERY,
  FEAT_SLIPPERY_MIND,
  FEAT_UNDEFINED
};

const int class_feats_fighter[] = {
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_DEFLECT_ARROWS,
  FEAT_DODGE,
  FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD,
  FEAT_EXOTIC_WEAPON_PROFICIENCY,
  FEAT_FAR_SHOT,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_GREATER_WEAPON_FOCUS,
  FEAT_GREATER_WEAPON_SPECIALIZATION,
  FEAT_IMPROVED_BULL_RUSH,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_DISARM,
  FEAT_IMPROVED_FEINT,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_IMPROVED_INITIATIVE,
  FEAT_IMPROVED_OVERRUN,
  FEAT_IMPROVED_PRECISE_SHOT,
  FEAT_IMPROVED_SHIELD_BASH,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TRIP,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_MANYSHOT,
  FEAT_MOBILITY,
  FEAT_MOUNTED_ARCHERY,
  FEAT_MOUNTED_COMBAT,
  FEAT_POINT_BLANK_SHOT,
  FEAT_POWER_ATTACK,
  FEAT_PRECISE_SHOT,
  FEAT_QUICK_DRAW,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_RIDE_BY_ATTACK,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_SNATCH_ARROWS,
  FEAT_SPIRITED_CHARGE,
  FEAT_SPRING_ATTACK,
  FEAT_STUNNING_FIST,
  FEAT_TRAMPLE,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,
  FEAT_WEAPON_SPECIALIZATION,
  FEAT_WHIRLWIND_ATTACK,
  FEAT_UNDEFINED
};

const int class_feats_monk[] = {
  FEAT_STUNNING_FIST,
  FEAT_UNDEFINED
};

const int no_class_feats[] = {
  FEAT_UNDEFINED
};

const int *class_bonus_feats[NUM_CLASSES] = {
/* WIZARD		*/ class_feats_wizard,
/* CLERIC		*/ no_class_feats,
/* ROGUE		*/ class_feats_rogue,
/* FIGHTER		*/ class_feats_fighter,
/* MONK			*/ class_feats_monk,
/* PALADIN		*/ no_class_feats,
/* NPC_EXPERT		*/ no_class_feats,
/* NPC_ADEPT		*/ no_class_feats,
/* NPC_COMMONER		*/ no_class_feats,
/* NPC_ARISTOCRAT	*/ no_class_feats,
/* NPC_WARRIOR		*/ no_class_feats
};

int highest_rank(struct char_data *ch, int type)
{

  int high=-1, rank=0, i;

  switch (type) {
    case HRANK_ARCANE:
      if (GET_CLASS_RANKS(ch, CLASS_WIZARD) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_WIZARD);
        high = CLASS_WIZARD;
      }
      if (GET_CLASS_RANKS(ch, CLASS_SORCERER) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_SORCERER);
        high = CLASS_SORCERER;
      }
      if (GET_CLASS_RANKS(ch, CLASS_BARD) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_BARD);
        high = CLASS_BARD;
      }
    break;
    case HRANK_DIVINE:
      if (GET_CLASS_RANKS(ch, CLASS_CLERIC) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_CLERIC);
        high = CLASS_CLERIC;
      }
      if (GET_CLASS_RANKS(ch, CLASS_PALADIN) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_PALADIN);
        high = CLASS_PALADIN;
      }
      if (GET_CLASS_RANKS(ch, CLASS_DRUID) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_DRUID);
        high = CLASS_DRUID;
      }
      if (GET_CLASS_RANKS(ch, CLASS_RANGER) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_RANGER);
        high = CLASS_RANGER;
      }
    case HRANK_CASTER:
      if (GET_CLASS_RANKS(ch, CLASS_WIZARD) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_WIZARD);
        high = CLASS_WIZARD;
      }
      if (GET_CLASS_RANKS(ch, CLASS_SORCERER) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_SORCERER);
        high = CLASS_SORCERER;
      }
      if (GET_CLASS_RANKS(ch, CLASS_BARD) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_BARD);
        high = CLASS_BARD;
      }
      if (GET_CLASS_RANKS(ch, CLASS_CLERIC) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_CLERIC);
        high = CLASS_CLERIC;
      }
      if (GET_CLASS_RANKS(ch, CLASS_PALADIN) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_PALADIN);
        high = CLASS_PALADIN;
      }
      if (GET_CLASS_RANKS(ch, CLASS_DRUID) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_DRUID);
        high = CLASS_DRUID;
      }
      if (GET_CLASS_RANKS(ch, CLASS_RANGER) > rank) {
        rank = GET_CLASS_RANKS(ch, CLASS_RANGER);
        high = CLASS_RANGER;
      }
    break;
    default:
      for(i = 0; i < NUM_CLASSES; i++) {
        if (GET_CLASS_RANKS(ch, i) > rank) {
          rank = GET_CLASS_RANKS(ch, i);
          high = i;
        }
      }
    break;

  }
  return high;
}

