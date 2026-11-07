/* ACL-ENGINE (PO 11-04: Shaw flies both sides): the built-in doctrine every AI fighter flies when MA_ACL_HERO is
   unset -- "hero" for the UN, "hero_red" for the MiGs. A copy of ~/acl/heroes/ma_period1.acl, compiled in so the
   AppImage needs no data file (and an install never keeps an old copy). Regenerate with tools/acl_default_hero.sh. */
#ifndef MA_ACL_HERO_H
#define MA_ACL_HERO_H
static const char ma_acl_default_hero[] = R"ACL(# ma_period1 (PO 11-04): sabre_ma14's manoeuvres and Sabre doctrine plus the period constraints, and the MiG's own doctrine
# (hero_red, flown by Red pilots in MA). History (SCRUM_STATUS MA-ENGAGE-1 / MA-RED-SCRAMBLE-1 sources): Sabres were barred
# from Manchuria and mostly, not always, broke off at the Yalu; MiGs were GCI-vectored onto the bombers and fighter-
# bombers (the Sabres were the screen), slashed through and zoomed away, went home over the Yalu when hit or short of fuel,
# and Soviet pilots were barred south of the Pyongyang-Wonsan line. The MiG-15 out-climbs the F-86; keep its turning
# fight aggressive (PO 10-16). The fog of war (who sees whom) is the runtime's (FOG-1), not the doctrine's.
manoeuvre headon(bandit) needs veteran:      # a deflection snap shot at 300+ m/s closure
  phase pass:    aim guns(bandit) g max              until range(bandit) > 2500 m or aot(bandit) < 120 deg or t > 10 s
manoeuvre defend(bandit):
  phase break:   aim into(bandit) g max              until his_aon(bandit) > 45 deg or t > 4 s
manoeuvre reversal(bandit) needs regular:     # Thyng, Shaw ch.2 p.49: "you let him gradually creep up ... reverse your turn. The hydraulic
                                              # controls [F-86] work beautifully ... When you pop your speed brakes, the MiG flashes by"
  phase turn:    aim into(bandit) g sustained       until range(bandit) < 500 m or t > 8 s
  phase brake:   aim into(bandit) g max brake       until his_aon(bandit) > 40 deg or t > 3 s
  phase slide:   aim guns(bandit) g max             until t > 4 s
manoeuvre regain(bandit) needs poor:
  phase unload:  aim dive(15 deg) g 0.3              until speed() > 120 m/s or t > 6 s 
manoeuvre pullup(bandit):
  phase recover: aim dive(10 deg) g 1 throttle 1     until speed() > 120 m/s or t > 20 s   # ENERGY-1 s2: unload first -- at the aoa cap a slow jet mushes, it cannot climb (sK: 220 s at 52 m/s into the ground)
  phase climb:   aim climb(15 deg) g sustained       until alt() > 2600 m or speed() < 110 m/s or t > 15 s
manoeuvre yoyo(bandit) needs veteran:
  phase up:      aim above(bandit, 25 deg) g max     until t > 3 s
  phase back:    aim lead(bandit) g max              until t > 3 s
manoeuvre attack(bandit):
  phase fight:   aim guns(bandit) g max              until t > 30 s     # Rowan's turning fight flies it (MA_ACL_TRACK_MAN=20)
manoeuvre lowyoyo(bandit) needs regular:
  phase down:    aim below(bandit, 20 deg) g sustained   until closure(bandit) > 40 m/s or t > 5 s
  phase back:    aim lead(bandit) g sustained            until t > 3 s
# ---- ma_period1 (PO 11-04): the period constraints and the MiG's own doctrine. Needs MA's runtime (damaged, bingo,
# outnumbered, crosser, yalu, limit, bomber, aim home); the Python engine does not model the map.
manoeuvre rtb(bandit):
  phase home:    aim home g 2 throttle 1                until t > 20 s      # damaged or bingo: straight home
manoeuvre turnback(bandit):
  phase home:    aim home g sustained                   until t > 15 s      # the rules of engagement: not past the line
manoeuvre zoom(bandit):
  phase up:      aim climb(30 deg) g sustained          until t > 12 s      # the MiG out-climbs the Sabre (Shaw p.227)
  phase out:     aim home g 2 throttle 1                until t > 15 s
manoeuvre slash(bandit):
  phase dive:    aim lead(bandit) g sustained           until range(bandit) < 700 m or t > 40 s
  phase fire:    aim guns(bandit) g max                 until range(bandit) < 350 m or t > 3 s
  phase zoom:    aim climb(30 deg) g max                until t > 10 s      # through and away -- never sit behind the B-29's guns
manoeuvre intercept(bandit):                 # MA-ESCORT-BOMBER-1 s5: close on a distant bomber first (GCI vectored the MiGs on)
  phase close:   aim pure(bandit) g 2 throttle 1        until range(bandit) < 2500 m or t > 60 s   # slash from 3-10 km: the lead
                                                                            # dive bled to 180 m/s on a 150 m/s B-29, closure ~0
manoeuvre hold(bandit):                      # the cover group: orbit high over the fight (Soviet gruppa prikrytiya)
  phase orbit:   aim right g 2 throttle 0.9                until t > 5 s
manoeuvre diveaway(bandit) needs regular:    # MACH-1: the Sabre's dive escape -- the F-86 dived on past the MiG-15's limit (M 0.92 in the
  phase dive:    aim dive(40 deg) g 1 throttle 1     until mach() > 0.95 or alt() < 3500 m or range(bandit) > 3000 m or t > 15 s   # MiG-15bis handbook), where the MiG had to pull out
  phase out:     aim away(bandit) g 2 throttle 1     until range(bandit) > 4000 m or t > 10 s
manoeuvre pullout(bandit):                     # MACH-1: a MiG does not follow a diving Sabre past his type's limit -- level off, airbrakes out
  phase level:   aim climb(5 deg) g 2 brake          until mach() < 0.80 or t > 6 s
manoeuvre climbaway(bandit) needs regular:   # the experienced MiG pilot's escape: zoom, the MiG out-climbs the F-86 (Shaw p.227);
  phase up:      aim climb(35 deg) g sustained throttle 1  until speed() < 140 m/s or t > 12 s   # novices broke or dived and were
  phase away:    aim away(bandit) g 2 throttle 1           until range(bandit) > 3000 m or t > 10 s   # caught (sF s1: 36 of 39 MiGs hit in defend/break)
doctrine hero:
  when alt() < 1800 m: do pullup(target)
  when threat and threat_level() > react(): do one of:      # first: breaking off never means ignoring a MiG on your tail (ACL-CHOICE-1; mix: heuristic)
    2: defend(threat)                              # the break
    1: reversal(threat)                            # Thyng (Shaw ch.2): let him creep up, reverse, brakes -> overshoot (needs regular)
    2 when mach() > 0.75 and alt() > 5000 m: diveaway(threat)   # MACH-1: fast and high, dive out of his reach (weight: heuristic)
  when damaged() or bingo(): do rtb(target)
  when yalu(target) < 0 and not crosser(): do turnback(target)              # most Sabres broke off at the river
  when yalu() + 30 < 0: do turnback(target)                                  # and the few who crossed did not go deep
  when range(target) < 2500 m and aot(target) > 150 deg and his_aon(target) < 30 deg: do headon(target)
  when speed() < 60 m/s: do regain(target)                                # ENERGY-1 s4: back to 60 m/s -- at 100 m/s (s1-s3) Sabres spent 55-61% of the fight in regain and case 1 fell to 0.6-0.8
  otherwise: do attack(target)
doctrine hero_red:
  when alt() < 1800 m: do pullup(target)
  when threat and threat_level() > react(): do one of:      # ACL-CHOICE-1: was climbaway when fast, else defend -- now a choice (mix: heuristic, the record gives none)
    3 when speed() > 200 m/s: climbaway(threat)    # Shaw p.209/p.227: stay fast, zoom on the T/W edge (needs regular)
    1: defend(threat)                              # the break
  when damaged() or bingo(): do rtb(target)                                 # over the Yalu: the sanctuary
  when limit() < 0 and not crosser(): do turnback(target)                   # not south of Pyongyang-Wonsan
  when cover() and not busy(target): do hold(target)                       # cover: hold height until a Sabre attacks a MiG
  when cover(): do slash(target)                                            # then dive on him, fire, zoom away
  when threat and outnumbered(): do zoom(threat)
  when mach() > 0.88 and alt(target) < alt(): do pullout(target)            # MACH-1: not past the MiG-15's limit after a diving Sabre
  when bomber(target) and range(target) > 2500 m: do intercept(target)
  when bomber(target): do slash(target)
  when range(target) < 2500 m and aot(target) > 150 deg and his_aon(target) < 30 deg: do headon(target)
  when speed() < 60 m/s: do regain(target)                                # ENERGY-1 s4: back to 60 m/s -- at 100 m/s (s1-s3) Sabres spent 55-61% of the fight in regain and case 1 fell to 0.6-0.8
  otherwise: do attack(target)
)ACL";
#endif
