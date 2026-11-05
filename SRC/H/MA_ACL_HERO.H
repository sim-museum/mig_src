/* ACL-ENGINE (PO 11-03: "make sabre_ma14 the default"): the built-in hero doctrine every UN AI fighter flies when
   MA_ACL_HERO is unset. A copy of ~/acl/heroes/sabre_ma14.acl, compiled in so the AppImage needs no data file (and an
   install never keeps an old copy). Regenerate with tools/acl_default_hero.sh after changing the hero. */
#ifndef MA_ACL_HERO_H
#define MA_ACL_HERO_H
static const char ma_acl_default_hero[] = R"ACL(# ma14: ma13 without the yo-yo rules and with regain only near the stall (< 60 m/s): in aclT ACL broke Rowan's turning
# fight off every 1-20 s (low yo-yo, regain below 85 m/s, yo-yo), so it never matured; Rowan's own Sabres stay in it.
# ma13: Shaw chooses, Rowan flies the turn -- ACL's attack is one guns phase at every range, handed to Rowan's turning
# fight (MA_ACL_TRACK_MAN=20); ACL keeps the decisions (pull-up, defend v a threat tracking me, head-on pass, regain,
# yo-yos). aclQ: wherever the handover was reached, lead-within-10 frames went up ~10x; it was reached in 2 fights of 8.
# ACL-ENGINE story 2 round 4: heroes/sabre_ma3.acl plus Shaw's low yo-yo (fig 2-8): behind a turning bandit 1.5-5 km
# out and not closing (< 10 m/s), drop the nose inside his turn for closure, then lead. Rowan's own Sabres fly the low
# yo-yo ~15% of a fight (aclSH: lead pursuit 39%, dive-and-zoom 16%, low yo-yo 15%); the ACL side, without one, spent
# its fights in the 1.5 km+ close phase and was inside 400 m ~40% less than Rowan's side.
# ma6: no low yo-yo below 3500 m (aclJ: the yo-yo dives took the fight down -- pull-ups 9% of frames).
# ma7: defend against the THREAT (an enemy inside 1.5 km pointing at me), not the nearest enemy (aclK: 3 of 4 Sabre
# losses died in defend/break pulling 1.3 g -- breaking on the wrong MiG while another one shot them).
# ma8: only a threat in my rear hemisphere (aclL: "threat and aot > 90" also broke away from head-on passes -- fired 462 -> 60).
# ma10: the head-on pass as its own manoeuvre (gun aim, MA's head-on shot). sabre_ma3-ma6's defend rule (nearest enemy
# pointing at me from ahead -> turn into him) was in effect this attack; dropping it in ma7-ma9 took firing 462 -> 57.
# ma9: break only when he is tracking me (nose within 20 deg, inside 1 km), for at most 4 s (ma8 fired 99: it broke too often).
manoeuvre headon(bandit) needs veteran:      # a deflection snap shot at 300+ m/s closure
  phase pass:    aim guns(bandit) g max              until range(bandit) > 2500 m or aot(bandit) < 120 deg or t > 10 s
manoeuvre defend(bandit):
  phase break:   aim into(bandit) g max              until his_aon(bandit) > 45 deg or t > 4 s
manoeuvre regain(bandit) needs poor:
  phase unload:  aim dive(15 deg) g 0.3              until speed() > 120 m/s or t > 6 s 
manoeuvre pullup(bandit):
  phase climb:   aim climb(20 deg) g max             until alt() > 2600 m or t > 15 s
manoeuvre yoyo(bandit) needs veteran:
  phase up:      aim above(bandit, 25 deg) g max     until t > 3 s
  phase back:    aim lead(bandit) g max              until t > 3 s
manoeuvre attack(bandit):
  phase fight:   aim guns(bandit) g max              until t > 30 s     # Rowan's turning fight flies it (MA_ACL_TRACK_MAN=20)
manoeuvre lowyoyo(bandit) needs regular:
  phase down:    aim below(bandit, 20 deg) g sustained   until closure(bandit) > 40 m/s or t > 5 s
  phase back:    aim lead(bandit) g sustained            until t > 3 s
doctrine hero:
  when alt() < 1800 m: do pullup(target)
  when threat and his_aot(threat) < 90 deg and his_aon(threat) < 20 deg and range(threat) < 1000 m: do defend(threat)
  when range(target) < 2500 m and aot(target) > 150 deg and his_aon(target) < 30 deg: do headon(target)
  when speed() < 60 m/s: do regain(target)
  otherwise: do attack(target)
)ACL";
#endif
