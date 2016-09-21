#700
Start Talisman~
2 c 100
start~
if (%actor.varexists(tal_playing)%)
  %send% %actor% @RYou are already playing @yTalisman@n!
else
  eval tal_token 700
  eval num_chars 3
  set tal_playing 1
  remote tal_playing %actor.id%
  %send% %actor% @cYou are now ready to play @yTalisman@n!
  eval character %%random.%num_chars%%%
  eval Ruins 709
  switch %character%
    case 1
      set tal_character Knight
      set tal_strength 3
      set tal_craft 3
      set tal_alignment Good
      set tal_start_name Chapel
      set tal_start_room 719
      break
    case 2
      set tal_character Warrior
      set tal_strength 4
      set tal_craft 3
      set tal_alignment Neutral
      set tal_start_name Tavern
      set tal_start_room 707
      break
    case 3
      set tal_character Warrior of Chaos
      set tal_strength 4
      set tal_craft 3
      set tal_alignment Evil
      set tal_start_name Ruins
      set tal_start_room 709
      break
    default
      %send% %actor% Reached default case.
      break
  done
  set tal_life 4
  set tal_gold 1
  set tal_movement_left 0
  set tal_region 1
  remote tal_character %actor.id%
  remote tal_strength %actor.id%
  remote tal_craft %actor.id%
  remote tal_life %actor.id%
  remote tal_alignment %actor.id%
  remote tal_start_name %actor.id%
  remote tal_start_room %actor.id%
  remote tal_gold %actor.id%
  remote tal_movement_left %actor.id%
  remote tal_region %actor.id%
  %send% %actor% You got charcter number %character% - %tal_character%.
  wait 1s
  %load% obj %tal_token% %actor% inv
  %send% %actor% You have been given a game token. Type 'end' at any time to quit the game of @YTalisman@n.
  %send% %actor% Please enjoy the game...
  %teleport% %actor% %tal_start_room%
  %force% %actor% look
end
~
#701
End Talisman~
1 c 2
end~
if (%actor.varexists(tal_playing)%)
  unset tal_playing 1
  rdelete tal_playing %actor.id%
  rdelete tal_character %actor.id%
  rdelete tal_strength %actor.id%
  rdelete tal_craft %actor.id%
  rdelete tal_life %actor.id%
  rdelete tal_alignment %actor.id%
  rdelete tal_start_name %actor.id%
  rdelete tal_start_room %actor.id%
  rdelete tal_gold %actor.id%
  rdelete tal_movement_left %actor.id%
  rdelete tal_region %actor.id%
  %send% %actor% @cYou are now done playing @yTalisman@n!
  wait 1s
  %teleport% %actor% 700
  %force% %actor% look
  %send% %actor% @cThe game token disintegrates itself@n!
  if %actor.inventory(700)%
    %purge% %actor.inventory(700)%
  end
else
  %send% %actor% @RYou aren't even currently playing @yTalisman@n!
end
~
#702
Talisman Character Sheet~
1 c 2
charsheet~
if (%actor.varexists(tal_playing)%)
%send% %actor% This is your @YTalisman@n Character Sheet:
%send% %actor% @b--------------------------------------@n
%send% %actor% Character: %actor.tal_character%
%send% %actor% @rStrength@n : @r%actor.tal_strength%         @CCraft@n: @C%actor.tal_craft%@n
%send% %actor% @YGold@n     : @Y%actor.tal_gold%@n         @gLives@n: @g%actor.tal_life%@n
%send% %actor% Alignment: %actor.tal_alignment%      Start: %actor.tal_start_name%
%send% %actor% @b--------------------------------------@n
else
  %send% %actor% @RYou aren't even currently playing @yTalisman@n!
end
~
#703
Roll for Movement~
1 c 2
roll~
  eval movement %random.6%
  set tal_movement_left %movement%
  remote tal_movement_left %actor.id%
  %send% %actor% You should move %movement% squares.
~
#704
Card Generator~
1 n 100
~
  eval tal_num_events 1
  eval tal_num_emonsters 1
  eval tal_num_espirits 1
  eval tal_num_strangers 1
  eval tal_num_objects 1
  eval tal_num_magicobjects 1
  eval tal_num_followers 1
  eval tal_num_places 1
  set here %self.room%
  switch %random.6%
    case 1
      set tal_card_type Event
      break
    case 2
      set tal_card_type Enemy - Monster
      break
    case 3
      set tal_card_type Enemy - Spirit
      break
    case 4
      set tal_card_type Stranger
      break
    case 5
      set tal_card_type Object
      break
    case 6
      set tal_card_type Place
      break
    default
      %echo% Reached default case.
  done
  *%echo% @gYou drew a %tal_card_type% Adventure Card.@n
  %send% %here.people% @gYou drew a %tal_card_type% Adventure Card.@n
~
#705
Temple Move Command~
2 c 100
temple~
  if (%actor.varexists(tal_playing)%)
    if (%actor.varexists(tal_boatman)%)
      %send% %actor% The boatman ferries you across to the Temple in the Middle Region!
      %teleport% %actor% 733
      %force% %actor% look
      rdelete tal_boatman %actor.id%
    else
      %send% %actor% But the boatman hasn't offered you a ride!
    end
  end
~
#707
Tavern Die Roll~
2 g 100
~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left%)
      eval tal_movement_left %actor.tal_movement_left% - 1
      %send% %actor% You expend 1 move, %tal_movement_left% remain.
      remote tal_movement_left %actor.id%
    end
    if (%actor.tal_movement_left% == 0) 
      wait 1s
      eval dieroll %random.6%
      switch %dieroll%
        *case 1
        * Char should lose 1 turn.
        *  break
        case 2
          eval tal_enemy_name farmer
          eval tal_enemy_strength 3
          %send% %actor% You get tipsy and get in a fight with a %tal_enemy_name% of @rStrength %tal_enemy_strength%@n.
          wait 1s
          eval tal_player_roll %random.6%
          eval tal_enemy_roll %random.6%
          eval tal_player_total %actor.tal_strength% + %tal_player_roll%
          eval tal_enemy_total %tal_enemy_strength% + %tal_enemy_roll%
          %send% %actor% Your combat total was: %tal_player_total% (%actor.tal_strength% + %tal_player_roll%)
          %send% %actor% Enemy combat total was: %tal_enemy_total% (%tal_enemy_strength% + %tal_enemy_roll%)
          wait 1s
          if (%tal_player_total% >= %tal_enemy_total%)
            %send% %actor% You are victorious over the %tal_enemy_name%!
          else
            eval tal_life %actor.tal_life% - 1
            remote tal_life %actor.id%
            %send% %actor% The %tal_enemy_name% has won! You lose @g1 Life@n! Down to @g%tal_life% lives@n left.
            wait 1s
            if (%actor.tal_life% == 0)
              %send% %actor% You have lost your last @glife@n! Your game is now over...
              %force% %actor% end
            end
          end
          break
        case 3
          %send% %actor% You gamble and lose @Y1 Gold@n.
          if (%actor.tal_gold%)
            eval tal_gold %actor.tal_gold% - 1
            remote tal_gold %actor.id%
          else
            %send% %actor% However...as you have no @YGold@n you lose @g1 life@n instead!
            eval tal_life %actor.tal_life% - 1
            remote tal_life %actor.id%
            if (%actor.tal_life% == 0)
              %send% %actor% You have lost your last @glife@n! Your game is now over...
              %force% %actor% end
            end
          end
          break
        case 4
          %send% %actor% You gamble and win @Y1 Gold@n.
          eval tal_gold %actor.tal_gold% + 1
          remote tal_gold %actor.id%
          break
        *case 5
        * Wizards offers to teleport you to any space in the Outer Region.
        * Complex at best. Need at least some kind of ASCII Map + Destination Trigger.
        *  break
        case 6
          %send% %actor% The boatman offers to ferry you to the Temple as your next Move.
          %send% %actor% You may use the 'temple' command instead of 'roll' for you next Move.
          set tal_boatman 1
          remote tal_boatman %actor.id%
          break
        default
          %send% %actor% You rolled a %dieroll%.
      done
    end
  end
~
#711
Forest Die Roll~
2 g 100
~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left%)
      eval tal_movement_left %actor.tal_movement_left% - 1
      %send% %actor% You expend 1 move, %tal_movement_left% remain.
      remote tal_movement_left %actor.id%
    end
    if (%actor.tal_movement_left% == 0) 
      wait 1s
      eval dieroll %random.6%
      switch %dieroll%
        case 1
          eval tal_enemy_name brigand
          eval tal_enemy_strength 4
          %send% %actor% You are attacked by a %tal_enemy_name% of @rStrength %tal_enemy_strength%@n.
          wait 1s
          eval tal_player_roll %random.6%
          eval tal_enemy_roll %random.6%
          eval tal_player_total %actor.tal_strength% + %tal_player_roll%
          eval tal_enemy_total %tal_enemy_strength% + %tal_enemy_roll%
          %send% %actor% Your combat total was: %tal_player_total% (%actor.tal_strength% + %tal_player_roll%)
          %send% %actor% Enemy combat total was: %tal_enemy_total% (%tal_enemy_strength% + %tal_enemy_roll%)
          wait 1s
          if (%tal_player_total% >= %tal_enemy_total%)
            %send% %actor% You are victorious over the %tal_enemy_name%!
          else
            eval tal_life %actor.tal_life% - 1
            remote tal_life %actor.id%
            %send% %actor% The %tal_enemy_name% has won! You lose @g1 Life@n! Down to @g%tal_life% lives@n left.
            wait 1s
            if (%actor.tal_life% == 0)
              %send% %actor% You have lost your last @glife@n! Your game is now over...
              %force% %actor% end
            end
          end
          break
        *case 2
        *case 3
        * Char should lose 1 turn.
        *break
        case 4
        case 5
          %send% %actor% You are safe for this turn.
          break
        case 6
          %send% %actor%  A ranger guides you out. Gain @C1 Craft@n.
          eval tal_craft %actor.tal_craft% + 1
          remote tal_craft %actor.id%
          break
        default
          %send% %actor% You rolled a %dieroll%.
      done
    end
  end
~
#712
Village Heal Command~
2 c 100
heal~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left% == 0) 
      if (!%arg%)
        %send% %actor% You may be Healed back up to your original quota at the cost of @Y1G@n per @gLife@n.
        %send% %actor% You may use the 'heal' command followed by the number of @gLives@n to recover.
        %send% %actor% IE: heal 2 to heal @g2 Lives@n.
        halt
      end
      if (%arg% > 3)
        %send% %actor% You should not be able to heal THAT many @gLives@n anyway Starting quota is @g4 Lives@n.
        halt
      end
      if (%actor.tal_life% > 3)
        %send% %actor% You are already at (or over) your starting quota of @g4 Lives@n.
        halt
      end
      if (%arg% > 4 - %actor.tal_life%)
        %send% %actor% But that would take you over your starting quota of @g4 Lives@n.
        halt
      end
      if (%arg% > %actor.tal_gold%)
        %send% %actor% You do not have enough @YGold@n to heal that many @gLives@n. Try something less than @Y%actor.tal_gold%@n.
        halt
      end
      * Probably still missing cases...but here goes...
      eval tal_life %actor.tal_life% + %arg%
      eval tal_gold %actor.tal_gold% - %arg%
      remote tal_life %actor.id% 
      remote tal_gold %actor.id% 
      %send% %actor% You have been healed @g%arg% Lives@n and have @Y%tal_gold% Gold@n remaining.
      halt
    end
  end
~
#713
Village Mystic Command~
2 c 100
mystic~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left% == 0) 
      wait 1s
      eval dieroll %random.6%
      switch %dieroll%
        case 1
        case 2
        case 3
          %send% %actor% The Mystic ignores you.
          break
        case 4
          if (%actor.tal_alignment% != Good)
            %send% %actor% The Mystic has changed your alignment to Good!
            eval tal_alignment Good
            remote tal_alignment %actor.id%
          else
            %send% %actor% The Mystic ignores you.
          end
          break
        case 5
          %send% %actor%  You gain @C1 Craft@n.
          eval tal_craft %actor.tal_craft% + 1
          remote tal_craft %actor.id%
          break
        *case 6
        * Gain 1 Spell. Not going there yet...
        default
          %send% %actor% You rolled a %dieroll%.
      done
    end
  end
~
#715
Graveyard Entry~
2 g 100
~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left%)
      eval tal_movement_left %actor.tal_movement_left% - 1
      %send% %actor% You expend 1 move, %tal_movement_left% remain.
      remote tal_movement_left %actor.id%
    end
    if (%actor.tal_movement_left% == 0) 
      if (%actor.tal_alignment% == Evil)
        wait 1s
        eval dieroll %random.6%
        switch %dieroll%
          *case 1
          * Char loses 1 turn.
          case 2
          case 3
          case 4
            if (%actor.tal_life% < 4)
              %send% %actor% Heal @g1 life@n.
              eval tal_life %actor.tal_life% + 1
              remote tal_life %actor.id%
            else
              %send% %actor% No effect as you have @g4 (or more) lives@n!
            end
            break
          *case 5
          *case 6
          * Gain 1 Spell. Not going there yet...
          default
            %send% %actor% You rolled a %dieroll%.
        done
      end
      if (%actor.tal_alignment% == Good)
        eval tal_life %actor.tal_life% - 1
        remote tal_life %actor.id%
        %send% %actor% This Unholy place punishes the rightous! You lose @g1 life@n! Down to @g%tal_life% lives@n left.
        if (%actor.tal_life% == 0)
          %send% %actor% You have lost your last @glife@n! Your game is now over...
          %force% %actor% end
        end
      end
    end
  end
~
#716
Graveyard Invoke Command~
2 c 100
invoke~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left% == 0) 
      if (%actor.tal_alignment% == Evil)
        wait 1s
        eval dieroll %random.6%
        switch %dieroll%
          *case 1
          * Char loses 1 turn.
          case 2
          case 3
          case 4
            %send% %actor% Heal @g1 life@n.
            if (%actor.tal_life < 4)
              eval tal_life %actor.tal_life% + 1
              remote tal_life %actor.id%
              halt
            end
          *case 5
          *case 6
          * Gain 1 Spell. Not going there yet...
          default
            %send% %actor% You rolled a %dieroll%.
        done
      else
        %send% %actor% Only the Wicked may Invoke at this Graveyard!
      end
    end
  end
~
#718
Chapel Entry~
2 g 100
~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left%)
      eval tal_movement_left %actor.tal_movement_left% - 1
      %send% %actor% You expend 1 move, %tal_movement_left% remain.
      remote tal_movement_left %actor.id%
    end
    if (%actor.tal_movement_left% == 0) 
      if (%actor.tal_alignment% == Good)
        %send% %actor% You may either be Healed free of charge back up to your starting quota, or you may Pray by rolling 1 die.
        %send% %actor% You may type either the 'pray' or 'heal' command.
      end
      if (%actor.tal_alignment% == Neutral)
        %send% %actor% You may be Healed back up to your original quota at the cost of @Y1G@n per @gLife@n.
        %send% %actor% You may use the 'heal' command followed by the number of @gLives@n to recover.
        %send% %actor% IE: heal 2 to heal @g2 Lives@n.
      end
      if (%actor.tal_alignment% == Evil)
        eval tal_life %actor.tal_life% - 1
        remote tal_life %actor.id%
        %send% %actor% This Holy place punishes the wicked! You lose @g1 life@n! Down to @g%tal_life% lives@n left.
        if (%actor.tal_life% == 0)
          %send% %actor% You have lost your last @glife@n! Your game is now over...
          %force% %actor% end
        end
      end
    end
  end
~
#719
Chapel Heal Command~
2 c 100
heal~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left% == 0) 
      if (%actor.tal_alignment% == Good)
        eval tal_life 4
        remote tal_life %actor.id%
        %send% %actor% You have been Healed back to @g%tal_life% Lives@n free of charge!
      end
      if (%actor.tal_alignment% == Neutral && !%arg%)
        %send% %actor% You may be Healed back up to your original quota at the cost of @Y1G@n per @gLife@n.
        %send% %actor% You may use the 'heal' command followed by the number of @gLives@n to recover.
        %send% %actor% IE: heal 2 to heal @g2 Lives@n.
      end
      if (%actor.tal_alignment% == Neutral && %arg%)
        if (%arg% > 3)
          %send% %actor% You should not be able to heal THAT many @gLives@n anyway Starting quota is @g4 Lives@n.
          halt
        end
        if (%actor.tal_life% > 3)
          %send% %actor% You are already at (or over) your starting quota of @g4 Lives@n.
          halt
        end
        if (%arg% > 4 - %actor.tal_life%)
          %send% %actor% But that would take you over your starting quota of @g4 Lives@n.
          halt
        end
        if (%arg% > %actor.tal_gold%)
          %send% %actor% You do not have enough @YGold@n to heal that many @gLives@n. Try something less than @Y%actor.tal_gold%@n.
          halt
        end
        * Probably still missing cases...but here goes...
        eval tal_life %actor.tal_life% + %arg%
        eval tal_gold %actor.tal_gold% - %arg%
        remote tal_life %actor.id% 
        remote tal_gold %actor.id% 
        %send% %actor% You have been healed @g%arg% Lives@n and have @Y%tal_gold% Gold@n remaining.
        halt
      end
      if (%actor.tal_alignment% == Evil)
        %send% %actor% This Holy place does not Heal the wicked!
      end
    end
  end
~
#720
Chapel Pray Command~
2 c 100
pray~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left% == 0) 
      if (%actor.tal_alignment% == Good)
        wait 1s
        eval dieroll %random.6%
        switch %dieroll%
          *case 1
          *case 2
          *case 3
          *case 4
          case 5
            %send% %actor% Gain @g1 life@n.
            eval tal_life %actor.tal_life% + 1
            remote tal_life %actor.id%
          *case 6
          * Gain 1 Spell. Not going there yet...
          default
            %send% %actor% You rolled a %dieroll%.
        done
      else
        %send% %actor% Only the Rightous may Pray at this Chapel!
      end
    end
  end
~
#721
Crags Die Roll~
2 g 100
~
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left%)
      eval tal_movement_left %actor.tal_movement_left% - 1
      %send% %actor% You expend 1 move, %tal_movement_left% remain.
      remote tal_movement_left %actor.id%
    end
    if (%actor.tal_movement_left% == 0) 
      wait 1s
      eval dieroll %random.6%
      switch %dieroll%
        case 1
          eval tal_enemy_name Spirit
          eval tal_enemy_craft 4
          %send% %actor% You are attacked by a %tal_enemy_name% of @CCraft %tal_enemy_craft%@n.
          wait 1s
          eval tal_player_roll %random.6%
          eval tal_enemy_roll %random.6%
          eval tal_player_total %actor.tal_craft% + %tal_player_roll%
          eval tal_enemy_total %tal_enemy_craft% + %tal_enemy_roll%
          %send% %actor% Your combat total was: %tal_player_total% (%actor.tal_craft% + %tal_player_roll%)
          %send% %actor% Enemy combat total was: %tal_enemy_total% (%tal_enemy_craft% + %tal_enemy_roll%)
          wait 1s
          if (%tal_player_total% >= %tal_enemy_total%)
            %send% %actor% You are victorious over the %tal_enemy_name%!
          else
            eval tal_life %actor.tal_life% - 1
            remote tal_life %actor.id%
            %send% %actor% The %tal_enemy_name% has won! You lose @g1 Life@n! Down to @g%tal_life% lives@n left.
            wait 1s
            if (%actor.tal_life% == 0)
              %send% %actor% You have lost your last @glife@n! Your game is now over...
              %force% %actor% end
            end
          end
          break
        *case 2
        *case 3
        * Char should lose 1 turn.
        *break
        case 4
        case 5
          %send% %actor% You are safe for this turn.
          break
        case 6
          %send% %actor%  A Barbarian leads you out. Gain @r1 Strength@n.
          eval tal_strength %actor.tal_strength% + 1
          remote tal_strength %actor.id%
          break
        default
          %send% %actor% You rolled a %dieroll%.
      done
    end
  end
~
#798
Movement Check~
2 q 100
~
  *if (%actor.varexists(tal_movement_left)% && %actor.varexists(tal_playing)% && %actor.tal_movement_left% == 0)
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_boatman%)
      eval tal_boatman 0
      remote tal_boatman %actor.id%
      %send% %actor% You pass up the boatman's offer.
    end
    if (%actor.tal_movement_left% == 0)
      %send% %actor% You have no movement left.
      return 0
    end
  end
~
#799
Draw 1 Card~
2 g 100
~
  *if (%actor.varexists(tal_movement_left)% && %actor.varexists(tal_playing)% && %actor.tal_movement_left%)
  if (%actor.varexists(tal_playing)%)
    if (%actor.tal_movement_left%)
      eval tal_movement_left %actor.tal_movement_left% - 1
      %send% %actor% You expend 1 move, %tal_movement_left% remain.
      remote tal_movement_left %actor.id%
    end
  *if (%actor.varexists(tal_movement_left)% && %actor.varexists(tal_playing)% && %actor.tal_movement_left% == 0) 
    if (%actor.tal_movement_left% == 0) 
      wait 1s
      set CARDOBJ 799
      eval obj %self.contents(%CARDOBJ%)%
      if %obj%
        set CARDISHERE 1
      end
      if %CARDISHERE%
        %send% %actor% @rThere is already a face up Adventure card here@n!
      else
        %send% %actor% @gYou reach into the Adventure deck and select the top card...@n
        %echoaround% %actor% %actor.name% @greaches into the Adventure deck and selects the top card...@n
        %load% obj %CARDOBJ%
      end
    end
  end
~
$~
