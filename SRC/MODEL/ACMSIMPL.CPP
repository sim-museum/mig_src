/*
	 MiG Alley
	 Copyright (C) 1998, 1999, 2000, 2001 Empire Interactive (Europe) Ltd,
	 677 High Road, North Finchley, London N12 0DA

	 Please see the document licence.doc for the full licence agreement

2. LICENCE
 2.1 	
 	Subject to the provisions of this Agreement we now grant to you the 
 	following rights in respect of the Source Code:
  2.1.1 
  	the non-exclusive right to Exploit  the Source Code and Executable 
  	Code on any medium; and 
  2.1.2 
  	the non-exclusive right to create and distribute Derivative Works.
 2.2 	
 	Subject to the provisions of this Agreement we now grant you the
	following rights in respect of the Object Code:
  2.2.1 
	the non-exclusive right to Exploit the Object Code on the same
	terms and conditions set out in clause 3, provided that any
	distribution is done so on the terms of this Agreement and is
	accompanied by the Source Code and Executable Code (as
	applicable).

3. GENERAL OBLIGATIONS
 3.1 
 	In consideration of the licence granted in clause 2.1 you now agree:
  3.1.1 
	that when you distribute the Source Code or Executable Code or
	any Derivative Works to Recipients you will also include the
	terms of this Agreement;
  3.1.2 
	that when you make the Source Code, Executable Code or any
	Derivative Works ("Materials") available to download, you will
	ensure that Recipients must accept the terms of this Agreement
	before being allowed to download such Materials;
  3.1.3 
	that by Exploiting the Source Code or Executable Code you may
	not impose any further restrictions on a Recipient's subsequent
	Exploitation of the Source Code or Executable Code other than
	those contained in the terms and conditions of this Agreement;
  3.1.4 
	not (and not to allow any third party) to profit or make any
	charge for the Source Code, or Executable Code, any
	Exploitation of the Source Code or Executable Code, or for any
	Derivative Works;
  3.1.5 
	not to place any restrictions on the operability of the Source 
	Code;
  3.1.6 
	to attach prominent notices to any Derivative Works stating
	that you have changed the Source Code or Executable Code and to
	include the details anddate of such change; and
  3.1.7 
  	not to Exploit the Source Code or Executable Code otherwise than
	as expressly permitted by  this Agreement.

questions about this file may be asked at http://www.simhq.com/
*/



#define F_GRAFIX											//DAW 05Aug96
#define F_COMMON
#define F_BATTLE



#include "collided.h"										//PD 06Sep96
#include "Dosdefs.h"		
#include "Worldinc.h"


#include "modvec.h"
#include "model.h"
#include "acmmodel.h"



#include "FlyModel.h"
#include "mymath.h"
#include "enumbits.m"
#include "mytime.h"											//RDH 25Apr96
#include "ranges.h"
#include "missman2.h"										//JIM 22Aug96
#include "movement.h"
#include "transite.h"


#include "ai.h"
#include "miles.h"											//RDH 25Jul96
#include "landscap.h"										//RDH 18Dec96
#include "viewsel.h"										//RDH 25Jul96
#include "3dcom.h"											//RJS 22Aug96
#include "speed.h"											//RDH 01Oct96
#include "persons2.h"

#include "model.h"

#if defined(BOB_LINUX)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* PILOT-MODEL-1 49c (PO 10-17): AI pilot fatigue, ported from Battle of Britain (bob MOVEALL.CPP). PO: "the mig15 is
   much more tiring to fly, and requires more strength, than with the hydraulics 'power steering' of the F86. Thus
   mig15 pilots tire while F86 pilots don't"; "in a mig15, you're always fighting the wicked handling, e.g. adverse
   yaw". Shaw (roll performance): "hydraulically boosted ailerons improved the agility of the American F-86 Sabre jets
   relative to their Russian MiG-15 adversaries"; p.377: ~5 G sustained, a G suit adds ~1 G (USAF Sabre pilots wore
   them in Korea). Model: fatigue F 0..1 grows with sustained G and with the share of the pilot's strength on the stick
   (aileron force ~ k * (IAS / 400 mph)^2; F-86 boosted k 0.15, MiG-15 1.3, MiG-15bis 1.0 -- the bis added an aileron
   booster but kept the heavy unboosted elevator -- others 1.0) plus, for the MiGs, the rudder work against adverse yaw
   whenever they roll. Tired = less stick force (lazier rolls at speed) and a lower G tolerance (5.4 G, F-86 +1 G suit).
   ON by default (PO 10-17: "make the ma improvements (fatigue, theory of mind) opt-out"): MA_NO_FATIGUE=1 turns
   fatigue and theory of mind off; MA_TRACE_FATIGUE=1 traces. The player never tires (PO 10-17). */
struct MaPilot { unsigned id; float F, tiring, roll, tr, gs, gsus, vx, vy, vz, sax, say, saz, bo, pull; SWord ohdg; bool seen; };
static MaPilot s_ma_pil[2048];
static MaPilot* ma_pil(AirStrucPtr a, bool create)
{
	const unsigned id = a->uniqueID.count;
	MaPilot& p = s_ma_pil[((id * 2654435761u) >> 9) & 2047];
	if (p.id != id) {
		if (!create) return 0;
		unsigned h = id * 2246822519u; h ^= h >> 13;
		p.id = id; p.F = 0; p.roll = 0; p.tr = 0; p.gs = 1; p.gsus = 1; p.sax = 0; p.say = 9.80665f; p.saz = 0; p.bo = 0; p.pull = 1; p.seen = false;
		p.tiring = 0.8f + 0.45f * float(h & 1023) / 1023.0f;
	}
	return &p;
}
bool ma_fatigue_on() { static int on = -1; if (on < 0) on = getenv("MA_NO_FATIGUE") ? 0 : 1; return on != 0; }
static int ma_type(AirStrucPtr a) { return a->fly.pModel ? int(a->fly.pModel->Type) : -1; }
static bool ma_is_sabre(AirStrucPtr a) { const int t = ma_type(a); return t == AT_F86A || t == AT_F86E || t == AT_F86F; }
static bool ma_is_mig(AirStrucPtr a) { const int t = ma_type(a); return t == AT_MIG15 || t == AT_MIG15BIS; }
static float ma_roll_k(AirStrucPtr a)
{
	if (ma_is_sabre(a)) return 0.15f;
	if (ma_type(a) == AT_MIG15) return 1.3f;
	return 1.0f;	// MiG-15bis (aileron booster, heavy elevator) and the rest
}
static float ma_q400(AirStrucPtr a)	// (IAS / 400 mph)^2; vel is mm/cs = 0.1 m/s
{
	float s = 1.0f - 2.2558e-5f * float(a->World.Y) * 0.01f; if (s < 0.2f) s = 0.2f;
	const float v = float(a->vel) * 0.1f / 178.8f;
	return v * v * powf(s, 4.2559f);
}
static float ma_g_tol(AirStrucPtr a, const MaPilot& p) { return 5.4f + (ma_is_sabre(a) ? 1.0f : 0.0f) - 0.8f * (p.tiring - 1.0f) - p.F; }
static float ma_g_lim(AirStrucPtr a, const MaPilot& p) { return (p.bo > 0) ? 0.0f : ma_g_tol(a, p) + ((int(a->ai.combatskill) < SKILL_REGULAR) ? 1.5f : 0.0f); }
/* item 51 (PO 10-18: "if historically migs scored with BnZ, then that behavior should also be seen in ma"): the tolerance above
   is for SUSTAINED G (blackout follows G held for some seconds -- the brain's oxygen reserve covers ~4-6 s). A pilot fresh in
   G may pull up to 1.5 G more for a short tracking pull; the allowance shrinks as the ~5 s mean G builds. Capping every
   instant at the sustained figure took the MiG's slashing pass away (QM 4, 4 seeds: MiG kills 9 -> 4 with fatigue, 9 with
   the cap off). */
static float ma_g_inst(AirStrucPtr a, const MaPilot& p)
{
	const float lim = ma_g_lim(a, p); if (lim <= 1.0f) return lim;
	float f = (lim - p.gsus) / (lim - 1.0f); if (f < 0) f = 0; if (f > 1) f = 1;
	return lim + 1.5f * f;
}
/* MA-KILL-RATIO-1 (51) sprint 3: pulling out of a dive near the ground is an emergency, not sustained manoeuvring -- the
   pilot pulls with everything he has (and strains against the G). The fatigue limits on roll and G and the onset of a
   blackout stand aside in MANOEUVRE_LOWALT; a blackout already under way still runs its course. Without this, tired
   MiG pilots could not make the pull-out and flew into the ground (QM 9, veteran Sabres v ace MiGs: 6 MiGs lost, all
   in LOWALT, against 1 with fatigue off). */
static bool ma_emergency(AirStrucPtr a, const MaPilot* p) { return a->ai.manoeuvre == MANOEUVRE_LOWALT && !(p && p->bo > 0); }
static int ma_fat_test() { static int t = -1; if (t < 0) { const char* s = getenv("MA_FAT_TEST"); t = !s ? 0 : !strcmp(s, "roll") ? 1 : !strcmp(s, "g") ? 2 : 0; } return t; }	// item 51 test hook: roll / g = that limit off
float ma_g_limit(AirStrucPtr a) { if (!ma_fatigue_on() || ma_fat_test() == 2) return 99.0f; MaPilot* p = ma_pil(a, false); return !p || ma_emergency(a, p) ? 99.0f : ma_g_inst(a, *p); }
float ma_pilot_fatigue(AirStrucPtr a) { if (!ma_fatigue_on()) return 0; MaPilot* p = ma_pil(a, false); return p ? p->F : 0; }
float ma_g_capacity(AirStrucPtr a) { if (!ma_fatigue_on() || ma_fat_test() == 2) return 1.0f; MaPilot* p = ma_pil(a, false); return !p || ma_emergency(a, p) ? 1.0f : (p->bo > 0) ? 0.0f : p->pull; }
float ma_roll_capacity(AirStrucPtr a)
{
	if (!ma_fatigue_on() || ma_fat_test() == 1) return 1.0f;
	MaPilot* p = ma_pil(a, false); const float F = p ? p->F : 0;
	if (p && p->bo > 0) return 0.0f;
	if (ma_emergency(a, p)) return 1.0f;
	const float need = ma_roll_k(a) * ma_q400(a), have = 1.0f - 0.45f * F;
	return ((need > have) ? have / need : 1.0f) * (1.0f - 0.2f * F);
}
void ma_fatigue_roll(AirStrucPtr a, float rollfrac) { if (ma_fatigue_on()) { MaPilot* p = ma_pil(a, true); if (rollfrac > p->roll) p->roll = rollfrac; } }
/* PILOT-MODEL-1 49d (PO 10-17): theory of mind, ported from Battle of Britain (bob MOVEALL.CPP bdg_tom_*). Each AI
   pilot builds a model of the bandit he is fighting from what he can SEE of it (its flight path, never its internal
   state); the model sharpens the longer they fight, faster for a better pilot. Observed: acquaintance (time in
   contact), the bandit's turn rate against his own when both pull hard (Shaw p.180: angular gains per turn), and the
   bandit's roll crispness (peak roll rate now vs his fresh one: a tiring MiG pilot's rolls go lazy). Used in
   SelectNextEngageManoeuvre (ACMMAN.CPP): he chooses like a better pilot against a bandit he has learned, presses a
   tiring one, and drops a defence that keeps failing against him. MA_NO_TOM=1 turns it off; MA_TRACE_TOM=1 traces. */
struct MaTom { unsigned bandit; float acq, turn, crisp, best, peak, win, tr; SWord broll, bhdg; float bvx, bvy, bvz, bg, pt; int nref, pman; unsigned char fail[128], ok[128]; float w3[3], rrs; bool seen; };
static MaTom s_ma_tom[2048];
static bool ma_tom_on() { static int on = -1; if (on < 0) on = getenv("MA_NO_TOM") ? 0 : 1; return on && ma_fatigue_on(); }
static AirStrucPtr ma_bandit(AirStrucPtr me) { return (me->ai.unfriendly && me->ai.unfriendly->Status.size == AIRSTRUCSIZE) ? AirStrucPtr(*me->ai.unfriendly) : AirStrucPtr(0); }
static MaTom* ma_tom(AirStrucPtr me) { MaPilot* p = ma_pil(me, false); return p ? &s_ma_tom[p - s_ma_pil] : 0; }
static float ma_tom_skill(AirStrucPtr me)	// how fast and how well he reads a bandit: novice 0.4 .. ace 1.0
{
	const int s = int(me->ai.combatskill);
	return (s >= SKILL_ACE) ? 1.0f : (s >= SKILL_VETERAN) ? 0.85f : (s >= SKILL_REGULAR) ? 0.7f : 0.4f;
}
float ma_tom_knowledge(AirStrucPtr me)	// 0 = knows nothing of this bandit, 1 = has his measure
{
	if (!ma_tom_on()) return 0; MaTom* t = ma_tom(me); AirStrucPtr b = ma_bandit(me);
	return (t && b && t->bandit == b->uniqueID.count) ? t->acq * ma_tom_skill(me) : 0;
}
float ma_tom_turn(AirStrucPtr me) { MaTom* t = ma_tom(me); const float k = ma_tom_knowledge(me); return t ? 1.0f + (t->turn - 1.0f) * k : 1.0f; }
float ma_tom_crisp(AirStrucPtr me) { MaTom* t = ma_tom(me); const float k = ma_tom_knowledge(me); return t ? 1.0f + (t->crisp - 1.0f) * k : 1.0f; }
bool ma_tom_threatened(AirStrucPtr me)	// bandit inside 1 km, his nose within 40 deg of me, he is within 70 deg of my tail
{
	AirStrucPtr b = ma_bandit(me); if (!b) return false;
	const float dx = float(me->World.X - b->World.X), dy = float(me->World.Y - b->World.Y), dz = float(me->World.Z - b->World.Z);
	const float r = sqrtf(dx * dx + dy * dy + dz * dz); if (r > 100000.0f || r < 1.0f) return false;
	const float bv = sqrtf(float(b->velx) * b->velx + float(b->vely) * b->vely + float(b->velz) * b->velz) + 1.0f;
	const float mv = sqrtf(float(me->velx) * me->velx + float(me->vely) * me->vely + float(me->velz) * me->velz) + 1.0f;
	return (dx * b->velx + dy * b->vely + dz * b->velz) / (r * bv) > 0.766f && (dx * me->velx + dy * me->vely + dz * me->velz) / (r * mv) > 0.342f;
}
static int ma_tom_need(AirStrucPtr me) { const int s = int(me->ai.combatskill); return (s >= SKILL_VETERAN) ? 1 : (s >= SKILL_REGULAR) ? 2 : 3; }
bool ma_tom_failed(AirStrucPtr me, int man)
{
	if (!ma_tom_on() || man < 0 || man >= 128) return false;
	MaTom* t = ma_tom(me); AirStrucPtr b = ma_bandit(me);
	return t && b && t->bandit == b->uniqueID.count && t->fail[man] >= t->ok[man] + ma_tom_need(me);
}
void ma_tom_tried(AirStrucPtr me, int man) { MaTom* t = ma_tom(me); if (t && man >= 0 && man < 128) { t->pman = man; t->pt = 8.0f; } }
static void ma_tom_tick(AirStrucPtr me, MaPilot* p, float dt, float myg)
{
	if (!ma_tom_on()) return;
	MaTom& t = s_ma_tom[p - s_ma_pil];
	AirStrucPtr b = ma_bandit(me);
	if (!b) { t.acq -= dt / 300.0f; if (t.acq < 0) t.acq = 0; return; }
	if (t.bandit != b->uniqueID.count) { t.bandit = b->uniqueID.count; t.acq = 0; t.turn = 1; t.crisp = 1; t.best = 0; t.peak = 0; t.win = 0; t.tr = 0; t.bg = 1; t.rrs = 0; t.nref = 0; t.pt = 0; t.pman = -1; memset(t.fail, 0, sizeof t.fail); memset(t.ok, 0, sizeof t.ok); t.seen = false; }
	static int tr = -1; if (tr < 0) tr = getenv("MA_TRACE_TOM") ? 1 : 0;
	if (t.pt > 0 && (t.pt -= dt) <= 0 && t.pman >= 0) {
		const bool still = ma_tom_threatened(me);
		unsigned char& c = still ? t.fail[t.pman] : t.ok[t.pman]; if (c < 255) c++;
		if (tr) fprintf(stderr, "[tom] id=%u learn man=%d %s (failed %d, worked %d)\n", p->id, t.pman, still ? "FAILED" : "worked", t.fail[t.pman], t.ok[t.pman]);
		t.pman = -1;
	}
	const float dx = float(b->World.X - me->World.X), dy = float(b->World.Y - me->World.Y), dz = float(b->World.Z - me->World.Z);
	const bool sees = (dx * dx + dy * dy + dz * dz < 2.5e5f * 2.5e5f) && p->bo <= 0;	// within 2.5 km, not blacked out
	const float bvx = float(b->velx) * 0.1f, bvy = float(b->vely) * 0.1f, bvz = float(b->velz) * 0.1f;
	if (sees && t.seen) {
		t.acq += dt * ma_tom_skill(me) / 90.0f; if (t.acq > 1) t.acq = 1;
		const float rr = fabsf(float(SWord(SWord(b->roll) - t.broll))) * 5.4932e-3f / dt;	// deg/s
		if (rr < 720.0f) t.rrs += (rr - t.rrs) * (dt < 0.3f ? dt / 0.3f : 1.0f);	// 0.3 s smoothing: a sustained roll, not a one-frame flick
		if (rr < 720.0f && t.rrs > t.peak) t.peak = t.rrs;
		const float ax = (bvx - t.bvx) / dt, ay = (bvy - t.bvy) / dt + 9.80665f, az = (bvz - t.bvz) / dt;
		float bg = sqrtf(ax * ax + ay * ay + az * az) / 9.80665f; if (bg > 9.0f) bg = 9.0f;
		t.bg += (bg - t.bg) * (dt < 1.0f ? dt : 1.0f); bg = t.bg;
		const float bw = fabsf(float(SWord(SWord(b->hdg) - t.bhdg))) * 5.4932e-3f / dt;
		const float mw = fabsf(float(SWord(SWord(me->hdg) - p->ohdg))) * 5.4932e-3f / dt;
		if (bg > 2.5f && myg > 2.5f && bw < 60.0f && mw > 5.0f && mw < 60.0f && bw > 5.0f)	// both pulling hard: who gains?
			t.turn += (bw / mw - t.turn) * dt / 20.0f;
		if ((t.win += dt) >= 8.0f) {	// 8 s windows: his peak roll rate now vs his fresh one (mean of the first 6 hard-rolling windows)
			if (t.peak > 30.0f) {
				if (t.nref < 6) { t.best = (t.best * t.nref + t.peak) / (t.nref + 1); t.w3[t.nref % 3] = t.peak; t.nref++; }
				else {	// his best roll in the last 3 rolling windows: one window without a hard roll is not fatigue
					t.w3[t.nref % 3] = t.peak; t.nref++;
					const float m = (t.w3[0] > t.w3[1]) ? ((t.w3[0] > t.w3[2]) ? t.w3[0] : t.w3[2]) : ((t.w3[1] > t.w3[2]) ? t.w3[1] : t.w3[2]);
					if (m > t.best) t.best = m;	// his best roll so far: the read only falls when he can no longer match it
					t.crisp += (m / t.best - t.crisp) * 0.25f;
				}
			}
			t.peak = 0; t.win = 0;
		}
	}
	t.broll = SWord(b->roll); t.bhdg = SWord(b->hdg); t.bvx = bvx; t.bvy = bvy; t.bvz = bvz; t.seen = true;
	if (tr && (t.tr += dt) >= 5.0f) {
		t.tr = 0;
		fprintf(stderr, "[tom] id=%u type=%d skill=%d bandit=%u btype=%d sees=%d acq=%.2f know=%.2f turn=%.2f crisp=%.2f best=%.0f\n", p->id, ma_type(me),
		        int(me->ai.combatskill), t.bandit, ma_type(b), int(sees), t.acq, ma_tom_knowledge(me), t.turn, t.crisp, t.best);
	}
}
/* SHAW-TACTIC-1 (57, PO 10-19): every hero-level AI fighter unit carries the Shaw picture it is fighting in, shared by
   the unit and re-assessed every 2 s. PO 10-19: the state is a RECORD of the whole picture as Shaw sees it -- several
   tactics and considerations apply at once -- not one label. Layers (Shaw chapter): fight size, mine v theirs (ch.3-8);
   similar or dissimilar aircraft (ch.3 v ch.4); section mode (ch.5-6: welded wing, loose deuce = engaged + free,
   sandwich, bracket, split = two separate 1v1s); per member a 1v1 posture (angles / energy / defensive / extend) and
   role (engaged / free); energy state and position of the leader against his bandit. The enemy's tactic AS PERCEIVED
   (theory of mind, item 49) is a later layer. The unit is the element (leader + follower); the record lives with the
   element leader. Hero = SKILL_HERO (the top AI skill); units led by the player are left alone (under MA_AI_PILOT the
   player's jet is AI-flown, so its unit counts as AI-led). Sprint 1 is READ-ONLY: assessed and logged, nothing in the
   flying reads it yet. Counts use the true positions within 5 km for now (SENSE limits come with the perceived layer).
   MA_NO_SHAW=1 turns it off; MA_TRACE_SHAW=1 logs the whole record whenever any layer changes. */
extern "C" { int ma_acmi_active(void); void ma_acmi_event(unsigned long id, const char* type, const char* text); }
enum { SZ_NONE, SZ_1V1, SZ_2V1, SZ_1V2, SZ_2V2, SZ_FEWVMANY, SZ_MANYVFEW, SZ_DIVISION };
enum { SEC_ALONE, SEC_WELDED, SEC_LOOSE, SEC_SANDWICH, SEC_BRACKET, SEC_SPLIT };
enum { PO_NONE, PO_ANGLES, PO_ENERGY, PO_DEFENSIVE, PO_EXTEND };
enum { RO_NONE, RO_ENGAGED, RO_FREE };
enum { EN_NONE, EN_ADV, EN_PAR, EN_DIS };
enum { PS_NONE, PS_OFF, PS_NEU, PS_DEF };
static const char* const s_shaw_size[] = { "none", "1v1", "2v1", "1v2", "2v2", "few-v-many", "many-v-few", "division" };
static const char* const s_shaw_sec[] = { "alone", "welded-wing", "loose-deuce", "sandwich", "bracket", "split" };
static const char* const s_shaw_post[] = { "-", "angles", "energy", "defensive", "extend" };
static const char* const s_shaw_role[] = { "-", "engaged", "free" };
static const char* const s_shaw_en[] = { "-", "advantage", "parity", "disadvantage" };
static const char* const s_shaw_pos[] = { "-", "offensive", "neutral", "defensive" };
/* The PLAN: the section tactic Shaw prescribes for this picture (ch.5-6). Read-only like the rest of the record.
   press      -- engaged fighter presses the attack, free fighter covers (double attack / loose deuce, pp.203-204, 212)
   switch     -- engaged fighter losing the offensive (energy depleted): call the free fighter in, roles reverse (p.204, 213)
   free-attack-- engaged fighter defensive, partner near: "the free fighter is obligated to attack the bogey
                 immediately, while the engaged fighter pilot does his best not to get shot" (p.213)
   bracket    -- 2v2 with mutual support: brackets and drags to eliminate one bogey (p.264)
   separate   -- two separate 1v1s, sound only with a performance or position superiority (p.264)
   regain     -- separated without that superiority: regain mutual support (p.264)
   singleton  -- alone against two or more: hit and run, both bogeys on one side, high energy, watch for escape (p.235)
   two-v-one  -- our section in mutual support (not welded) against a bandit SEEN without it -- his pair welded, or
                 him alone or separated: "against welded wing bogeys, two-versus-one tactics can be employed with high
                 probability of success" (p.241); a welded pair "can be considered the singleton" (p.240-241)
   Welded against welded: "the two-versus-two scenario essentially becomes one-versus-one" (p.240) -> 1v1. */
enum { PL_NONE, PL_1V1, PL_PRESS, PL_SWITCH, PL_FREEATTACK, PL_BRACKET, PL_SEPARATE, PL_REGAIN, PL_SINGLETON, PL_TWOVONE };
static const char* const s_shaw_plan[] = { "-", "1v1", "press", "switch", "free-attack", "bracket", "separate", "regain", "singleton", "two-v-one" };
/* The ENEMY AS PERCEIVED (item 49's theory of mind; PO 10-19: inferred from what is seen, never read from his state,
   so a hero cannot cheat). From positions and velocities only: his section -- the bandit's nearest other aircraft
   of his side: welded (< 1 km), mutual support (< 3 km), separated, alone (none within 5 km); his posture --
   offensive (on my tail), energy (flight path 20 deg or more off the horizon, up or down, range not opening: zoom,
   climb, slashing dive), angles (turning 6 deg/s or harder over the last 2 s, about the jets' sustained rate at
   height), extending (range opening and his velocity pointed away from me), else neutral.
   Calibrated on QM 4 (MA_TRACE_SHAW=2 prints [shawp] every assessment): steep flight separates Rowan's energy
   manoeuvres from TURNINGFIGHT (climb q10/q90 -0.69/+0.61 v -0.21/+0.32); turn rate does not -- TURNINGFIGHT's
   median is 1.6 deg/s, the label covering fast slashing attacks too -- so the turn limit is physical, not fitted. */
enum { ES_NONE, ES_ALONE, ES_WELDED, ES_MUTUAL, ES_SEPARATED };
enum { EP_NONE, EP_OFF, EP_ENERGY, EP_ANGLES, EP_EXTEND, EP_NEUTRAL };
static const char* const s_shaw_esec[] = { "-", "alone", "welded", "mutual-support", "separated" };
static const char* const s_shaw_epost[] = { "-", "offensive", "energy", "angles", "extending", "neutral" };
struct MaShawRec { unsigned char mine, theirs, size, dissimilar, section, post[2], role[2], energy, pos, esec, epost, plan; };
static bool operator==(const MaShawRec& a, const MaShawRec& b) { return !memcmp(&a, &b, sizeof a); }
struct MaShaw { unsigned id; float next; MaShawRec rec, cand; unsigned bid; float bt, brange, bv[3];  float steer_at;};	// b*: the bandit as last seen
static MaShaw s_ma_shaw[1024];
static float s_shaw_t;				// sim seconds, advanced by one owning aircraft (re-elected when it stops ticking)
static unsigned s_shaw_owner, s_shaw_orphan;
static bool ma_shaw_on() { static int on = -1; if (on < 0) on = getenv("MA_NO_SHAW") ? 0 : 1; return on != 0; }
static bool ma_shaw_live(AirStrucPtr a)
{
	return a && a->Status.size == AIRSTRUCSIZE && !a->Status.deaded && a->movecode != AUTO_SPIRAL2GROUND
	    && a->movecode != AUTO_DEATHSEQUENCE && a->movecode != AUTO_CRASHSKID && a->movecode != AUTO_CRASHROLL;
}
static float ma_shaw_rng(AirStrucPtr a, AirStrucPtr b)	// m
{
	const float dx = float(a->World.X - b->World.X), dy = float(a->World.Y - b->World.Y), dz = float(a->World.Z - b->World.Z);
	return sqrtf(dx * dx + dy * dy + dz * dz) * 0.01f;
}
static float ma_shaw_es(AirStrucPtr a)	// energy height, m
{
	const float vx = float(a->velx) * 0.1f, vy = float(a->vely) * 0.1f, vz = float(a->velz) * 0.1f;
	return float(a->World.Y) * 0.01f + (vx * vx + vy * vy + vz * vz) / (2.0f * 9.80665f);
}
static bool ma_shaw_on_tail(AirStrucPtr att, AirStrucPtr vic)	// att inside 1.5 km, nose within 40 deg of vic, vic within 70 deg of his tail
{
	const float dx = float(vic->World.X - att->World.X), dy = float(vic->World.Y - att->World.Y), dz = float(vic->World.Z - att->World.Z);
	const float r = sqrtf(dx * dx + dy * dy + dz * dz); if (r > 150000.0f || r < 1.0f) return false;
	const float av = sqrtf(float(att->velx) * att->velx + float(att->vely) * att->vely + float(att->velz) * att->velz) + 1.0f;
	const float vv = sqrtf(float(vic->velx) * vic->velx + float(vic->vely) * vic->vely + float(vic->velz) * vic->velz) + 1.0f;
	return (dx * att->velx + dy * att->vely + dz * att->velz) / (r * av) > 0.766f && (dx * vic->velx + dy * vic->vely + dz * vic->velz) / (r * vv) > 0.342f;
}
static AirStrucPtr ma_shaw_engaged(AirStrucPtr m) { return (ma_shaw_live(m) && m->movecode == AUTO_COMBAT) ? ma_bandit(m) : AirStrucPtr(0); }
static int ma_shaw_post(AirStrucPtr m)	// one fighter's 1v1 posture against his bandit
{
	AirStrucPtr b = ma_shaw_engaged(m); if (!b) return PO_NONE;
	if (ma_shaw_on_tail(b, m)) return PO_DEFENSIVE;
	switch (int(m->ai.manoeuvre)) {
	case MANOEUVRE_EXTENSION: case MANOEUVRE_DIVEFORHOME: case MANOEUVRE_GOHOME: case MANOEUVRE_MOVEAWAY:
	case MANOEUVRE_CLIMBFORHOME: case MANOEUVRE_MAKEFORFRIENDLY: case MANOEUVRE_STRAIGHTDIVE: return PO_EXTEND;
	case MANOEUVRE_DIVEANDZOOM: case MANOEUVRE_ZOOM: case MANOEUVRE_ZOOMANDDROP: case MANOEUVRE_HIGHYOYO:
	case MANOEUVRE_GAINHEIGHT: case MANOEUVRE_INTERCEPTHIGH: return PO_ENERGY;
	}
	// Shaw ch.3: the energy fighter keeps an energy advantage and stays out of the opponent's turn
	return (ma_shaw_es(m) > ma_shaw_es(b) + 500.0f && m->World.Y > b->World.Y) ? PO_ENERGY : PO_ANGLES;
}
static int ma_shaw_section(AirStrucPtr L, AirStrucPtr W, AirStrucPtr bL, AirStrucPtr bW)
{
	const bool wl = W && ma_shaw_live(W);
	if (!wl) return SEC_ALONE;
	const bool welded = W->ai.manoeuvre == MANOEUVRE_WELDEDWINGMAN;
	const float d = ma_shaw_rng(L, W);
	if (bL && bW) {
		if (welded || (bL == bW && d < 600.0f)) return SEC_WELDED;
		if ((ma_shaw_on_tail(bW, L) && !ma_shaw_on_tail(bW, W)) || (ma_shaw_on_tail(bL, W) && !ma_shaw_on_tail(bL, L))) return SEC_SANDWICH;
		if (bL == bW) return SEC_BRACKET;
		return (d > 3000.0f) ? SEC_SPLIT : SEC_LOOSE;
	}
	if (welded) return SEC_WELDED;
	return (d < 3000.0f) ? SEC_LOOSE : SEC_SPLIT;		// one engaged, the partner free (Shaw ch.6) -- or gone off alone
}
static MaShawRec ma_shaw_assess(AirStrucPtr L, AirStrucPtr W, MaShaw& s)
{
	MaShawRec r; memset(&r, 0, sizeof r);
	AirStrucPtr bL = ma_shaw_engaged(L), bW = ma_shaw_engaged(W);
	if (!bL && !bW) return r;
	AirStrucPtr me = bL ? L : W, b = bL ? bL : bW;		// the member whose fight describes the unit's energy/position
	int mine = 0, theirs = 0;
	for (MobileItemPtr e = mobileitem::ACHead(); e; e = e->nextmobile) {
		AirStrucPtr ac = (AirStrucPtr)e;
		if (!ma_shaw_live(ac) || ma_shaw_rng(L, ac) > 5000.0f) continue;
		if (ac->nationality == L->nationality) { if (ac->movecode == AUTO_COMBAT) mine++; }
		else theirs++;
	}
	if (mine < 1) mine = 1; if (theirs < 1) theirs = 1;
	r.mine = (unsigned char)(mine > 255 ? 255 : mine); r.theirs = (unsigned char)(theirs > 255 ? 255 : theirs);
	r.size = (mine == 1 && theirs == 1) ? SZ_1V1 : (mine == 2 && theirs == 1) ? SZ_2V1 : (mine == 1 && theirs == 2) ? SZ_1V2
	       : (mine == 2 && theirs == 2) ? SZ_2V2 : (mine <= 2 && theirs >= 3) ? SZ_FEWVMANY : (mine >= 3 && theirs <= 2) ? SZ_MANYVFEW
	       : SZ_DIVISION;
	r.dissimilar = (me->classtype != b->classtype) ? 1 : 0;
	{	// the enemy as perceived
		float dmin = 5000.0f;
		for (MobileItemPtr e = mobileitem::ACHead(); e; e = e->nextmobile) {
			AirStrucPtr ac = (AirStrucPtr)e;
			if (ac != b && ma_shaw_live(ac) && ac->nationality == b->nationality) { const float d = ma_shaw_rng(b, ac); if (d < dmin) dmin = d; }
		}
		r.esec = (dmin < 1000.0f) ? ES_WELDED : (dmin < 3000.0f) ? ES_MUTUAL : (dmin < 5000.0f) ? ES_SEPARATED : ES_ALONE;
		const float vx = float(b->velx), vy = float(b->vely), vz = float(b->velz), vm = sqrtf(vx * vx + vy * vy + vz * vz) + 1.0f;
		const float bv[3] = { vx / vm, vy / vm, vz / vm }, rng = ma_shaw_rng(me, b);
		float turn = 0.0f;			// deg/s since the last look, if it was this bandit 0.5-5 s ago
		if (s.bid == b->uniqueID.count && s_shaw_t - s.bt > 0.5f && s_shaw_t - s.bt < 5.0f) {
			float c = bv[0] * s.bv[0] + bv[1] * s.bv[1] + bv[2] * s.bv[2]; c = c > 1.0f ? 1.0f : c < -1.0f ? -1.0f : c;
			turn = acosf(c) * 57.29578f / (s_shaw_t - s.bt);
		}
		const float ax = float(b->World.X - me->World.X), ay = float(b->World.Y - me->World.Y), az = float(b->World.Z - me->World.Z);
		const float am = sqrtf(ax * ax + ay * ay + az * az) + 1.0f;
		const bool opening = s.bid == b->uniqueID.count && rng > s.brange + 100.0f && (bv[0] * ax + bv[1] * ay + bv[2] * az) / am > 0.5f;
		r.epost = ma_shaw_on_tail(b, me) ? EP_OFF : (fabsf(bv[1]) > 0.342f) ? (opening ? EP_EXTEND : EP_ENERGY) : (turn > 6.0f) ? EP_ANGLES
		        : opening ? EP_EXTEND : EP_NEUTRAL;
		static int pr = -1; if (pr < 0) { const char* t = getenv("MA_TRACE_SHAW"); pr = (t && atoi(t) >= 2) ? 1 : 0; }
		if (pr && turn > 0.0f)		// every assessment, for calibrating the perception against the bandit's true manoeuvre
			fprintf(stderr, "[shawp] t=%.0f bandit=%u man%d turn=%.1f climb=%.2f open=%d tail=%d\n", double(s_shaw_t),
			        unsigned(b->uniqueID.count), int(b->ai.manoeuvre), double(turn), double(bv[1]), opening ? 1 : 0, ma_shaw_on_tail(b, me) ? 1 : 0);
		s.bid = b->uniqueID.count; s.bt = s_shaw_t; s.brange = rng; s.bv[0] = bv[0]; s.bv[1] = bv[1]; s.bv[2] = bv[2];
	}
	r.section = (unsigned char)ma_shaw_section(L, W, bL, bW);
	r.post[0] = (unsigned char)ma_shaw_post(L); r.role[0] = bL ? RO_ENGAGED : (L && ma_shaw_live(L)) ? RO_FREE : RO_NONE;
	r.post[1] = W ? (unsigned char)ma_shaw_post(W) : PO_NONE; r.role[1] = bW ? RO_ENGAGED : (W && ma_shaw_live(W)) ? RO_FREE : RO_NONE;
	const float de = ma_shaw_es(me) - ma_shaw_es(b);
	r.energy = (de > 500.0f) ? EN_ADV : (de < -500.0f) ? EN_DIS : EN_PAR;
	r.pos = ma_shaw_on_tail(me, b) ? PS_OFF : ma_shaw_on_tail(b, me) ? PS_DEF : PS_NEU;
	const bool pair = r.section != SEC_ALONE, near = pair && r.section != SEC_SPLIT;
	const bool def = r.post[0] == PO_DEFENSIVE || r.post[1] == PO_DEFENSIVE;
	if (r.mine < 2 && r.theirs >= 2) r.plan = PL_SINGLETON;		// alone here against two or more, whatever the partner does
	else if (pair && !near) r.plan = (r.pos == PS_OFF || r.energy == EN_ADV) ? PL_SEPARATE : PL_REGAIN;
	else if (!pair || r.mine < 2) r.plan = (r.pos == PS_OFF) ? PL_PRESS : PL_1V1;
	else if (def) r.plan = PL_FREEATTACK;
	else if (r.section == SEC_WELDED && r.esec == ES_WELDED) r.plan = PL_1V1;		// p.240
	else if (r.section != SEC_WELDED && (r.esec == ES_WELDED || r.esec == ES_ALONE || r.esec == ES_SEPARATED)) r.plan = PL_TWOVONE;	// p.241
	else if (r.pos == PS_OFF && r.energy == EN_DIS) r.plan = PL_SWITCH;
	else if (r.theirs >= 2) r.plan = PL_BRACKET;
	else r.plan = PL_PRESS;
	return r;
}
/* SHAW-TACTIC-1 (57) steering (PO 10-20: the plan may steer hero pilots' flying). Only the FREE member is moved, and
   only onto a bandit: an engaged fighter is never pulled off his fight, so Rowan's manoeuvres (and the MiG's
   two-circle aggression, PO 10-16) are untouched. free-attack: the free fighter attacks the bandit seen on the
   defensive member's tail (p.213); two-v-one: the free fighter takes the engaged member's bandit (p.241). Through
   Rowan's SetEngage, which also hands the decision to the multiplayer owner. MA_NO_SHAW_STEER=1 keeps the record
   read-only; MA_TRACE_SHAW=1 logs each steer. */
static bool ma_shaw_steer_on() { static int on = -1; if (on < 0) on = getenv("MA_NO_SHAW_STEER") ? 0 : 1; return on != 0; }
static void ma_shaw_steer(AirStrucPtr L, AirStrucPtr W, const MaShawRec& r, float& steer_at)
{
	if (s_shaw_t - steer_at < 30.0f) return;	// once per 30 s: Rowan takes the target back, and each SetEngage restarts his manoeuvre
	if (!ma_shaw_steer_on() || !W || W->Status.size != AIRSTRUCSIZE || (r.plan != PL_FREEATTACK && r.plan != PL_TWOVONE)) return;
	AirStrucPtr F, E;
	if (r.role[0] == RO_FREE && r.role[1] == RO_ENGAGED) { F = L; E = W; }
	else if (r.role[1] == RO_FREE && r.role[0] == RO_ENGAGED) { F = W; E = L; }
	else return;
	AirStrucPtr t = 0;
	if (r.plan == PL_TWOVONE) t = ma_bandit(E);
	else {
		float dmin = 1e9f;
		for (MobileItemPtr e = mobileitem::ACHead(); e; e = e->nextmobile) {
			AirStrucPtr ac = (AirStrucPtr)e;
			if (ac->nationality != E->nationality && ma_shaw_live(ac) && ma_shaw_on_tail(ac, E)) { const float d = ma_shaw_rng(ac, E); if (d < dmin) { dmin = d; t = ac; } }
		}
	}
	if (!t || !ma_shaw_live(t) || ma_bandit(F) == t) return;
	Art_Int.SetEngage(F, t, MANOEUVRE_SELECT, ANGLES_0Deg, ANGLES_0Deg, false);
	steer_at = s_shaw_t;
	static int tr = -1; if (tr < 0) tr = getenv("MA_TRACE_SHAW") ? 1 : 0;
	if (tr) fprintf(stderr, "[shaws] t=%.0f %s: free %u -> bandit %u (engaged %u)\n", double(s_shaw_t), s_shaw_plan[r.plan],
	                unsigned(F->uniqueID.count), unsigned(t->uniqueID.count), unsigned(E->uniqueID.count));
}
static void ma_shaw_tick(AirStrucPtr a, float dt)
{
	if (!ma_shaw_on() || !ma_shaw_live(a)) return;
	if (!s_shaw_owner || ++s_shaw_orphan > 400) { s_shaw_owner = a->uniqueID.count; s_shaw_orphan = 0; }
	if (a->uniqueID.count == s_shaw_owner) { s_shaw_t += dt; s_shaw_orphan = 0; }
	if (a->leader || a->ai.combatskill < SKILL_HERO) return;			// element leaders only; hero only
	if (!getenv("MA_AI_PILOT") && (a->InPlayersElement() || a->PlayerInFlight())) return;
	const unsigned id = a->uniqueID.count;
	MaShaw& s = s_ma_shaw[((id * 2654435761u) >> 9) & 1023];
	if (s.id != id) { s.id = id; s.next = 0; memset(&s.rec, 0, sizeof s.rec); s.cand = s.rec; s.steer_at = -1e9f; }
	if (s_shaw_t < s.next) return;
	s.next = s_shaw_t + 2.0f;
	AirStrucPtr W = a->Follower();
	const MaShawRec v = ma_shaw_assess(a, (W && W->Status.size == AIRSTRUCSIZE) ? W : AirStrucPtr(0), s);
	const bool changed = !(v == s.rec) && v == s.cand;			// hysteresis: two assessments 2 s apart must agree
	s.cand = v;
	if (changed) s.rec = v;
	ma_shaw_steer(a, (W && W->Status.size == AIRSTRUCSIZE) ? W : AirStrucPtr(0), s.rec, s.steer_at);
	if (!changed) return;
	AirStrucPtr b = ma_bandit(a); const bool w = W && W->Status.size == AIRSTRUCSIZE;
	char txt[320];
	snprintf(txt, sizeof txt, "Shaw: %s (%dv%d)%s; %s; lead %s/%s man%d; wing %d %s/%s man%d; energy %s; %s; enemy %s/%s; plan %s",
	         s_shaw_size[v.size], int(v.mine), int(v.theirs), v.dissimilar ? " dissimilar" : "", s_shaw_sec[v.section],
	         s_shaw_role[v.role[0]], s_shaw_post[v.post[0]], int(a->ai.manoeuvre),
	         w ? int(W->uniqueID.count) : -1, s_shaw_role[v.role[1]], s_shaw_post[v.post[1]], w ? int(W->ai.manoeuvre) : -1,
	         s_shaw_en[v.energy], s_shaw_pos[v.pos], s_shaw_esec[v.esec], s_shaw_epost[v.epost], s_shaw_plan[v.plan]);
	if (ma_acmi_active()) {		// the replay's object id is the 1-based place in the AC list (Replay.cpp's ACMI walk)
		unsigned long n = 0, at = 0;
		for (MobileItemPtr e = mobileitem::ACHead(); e && n < 256; e = e->nextmobile) { n++; if ((AirStrucPtr)e == a) { at = n; break; } }
		ma_acmi_event(at, "Message", txt);
	}
	static int tr = -1; if (tr < 0) tr = getenv("MA_TRACE_SHAW") ? 1 : 0;
	if (tr)
		fprintf(stderr, "[shaw] t=%.0f unit=%u nat=%d %s; bandit=%d man%d range=%.0f\n", double(s_shaw_t), id, int(a->nationality), txt,
		        b ? int(b->uniqueID.count) : -1, b ? int(b->ai.manoeuvre) : -1, b ? double(ma_shaw_rng(a, b)) : -1.0);
}

void ma_fatigue_tick(AirStrucPtr a, float g, int path)	// once a frame; g < 0: from the flight path (simple movecode)
{
	ma_shaw_tick(a, Timer_Code.FRAMETIME * 0.01f);
	if (!ma_fatigue_on()) return;
	MaPilot* p = ma_pil(a, a->movecode == AUTO_COMBAT);
	if (!p) return;
	const float dt = Timer_Code.FRAMETIME * 0.01f, q = ma_q400(a);
	if (g < 0) {
		const float vx = float(a->velx) * 0.1f, vy = float(a->vely) * 0.1f, vz = float(a->velz) * 0.1f;
		if (p->seen) {
			const float k = (dt < 0.5f) ? dt / 0.5f : 1.0f;
			float ax = (vx - p->vx) / dt, ay = (vy - p->vy) / dt + 9.80665f, az = (vz - p->vz) / dt;
			const float am = sqrtf(ax * ax + ay * ay + az * az);
			if (am > 98.0665f) { const float s = 98.0665f / am; ax *= s; ay *= s; az *= s; }
			p->sax += (ax - p->sax) * k; p->say += (ay - p->say) * k; p->saz += (az - p->saz) * k;
			g = sqrtf(p->sax * p->sax + p->say * p->say + p->saz * p->saz) / 9.80665f; if (g > 9.0f) g = 9.0f;
		} else g = 1.0f;
		p->vx = vx; p->vy = vy; p->vz = vz; p->seen = true;
		const float lim = ma_g_inst(a, *p); if (lim > 1.0f && g > lim) g = lim;
	}
	float force = p->roll * ma_roll_k(a) * q; if (force > 1.0f) force = 1.0f;
	const float graw = g;
	p->gs += (g - p->gs) * (dt < 1.0f ? dt : 1.0f);
	p->gsus += (graw - p->gsus) * (dt < 5.0f ? dt / 5.0f : 1.0f);	// ~5 s mean: what blackout and the pilot's ease-off respond to
	g = p->gs;
	float ge = (g - 2.5f) / 2.5f; if (ge < 0) ge = 0;
	float d = (ge * ge / 40.0f + force * 0.006f + (ma_is_mig(a) ? p->roll * 0.004f : 0.0f)) * p->tiring;	// MiG: rudder against adverse yaw
	if (g < 2.0f && force < 0.3f) d -= 0.003f;
	p->F += d * dt; if (p->F < 0) p->F = 0; if (p->F > 1) p->F = 1;
	static int tr = -1; if (tr < 0) tr = getenv("MA_TRACE_FATIGUE") ? 1 : 0;
	const float tol = ma_g_tol(a, *p);
	if (p->bo > 0) { if ((p->bo -= dt) <= 0 && tr) fprintf(stderr, "[fatigue] id=%u comes round\n", p->id); }
	else if (path == 1 && p->gsus > tol + 0.7f && !ma_emergency(a, p)) {
		p->bo = 3.0f + 3.0f * p->F;
		if (tr) fprintf(stderr, "[fatigue] id=%u type=%d BLACKOUT g=%.2f tol=%.2f skill=%d F=%.2f\n", p->id, ma_type(a), g, tol, int(a->ai.combatskill), p->F);
	}
	if (path == 1 && !ma_emergency(a, p)) {	/* he feels the grey-out coming and eases the pull: fast, in proportion to the overshoot (a jet at
		   250 m/s reaches 6 G on little elevator; the 49c 0.5/s ease-off let ace MiGs black out 7-14 times in 7 minutes) */
		const float o1 = p->gsus - (ma_g_lim(a, *p) - ((int(a->ai.combatskill) < SKILL_REGULAR) ? 0.0f : 0.3f)), o2 = p->gs - ma_g_inst(a, *p);	// the G he feels (1 s), not the measured spikes
		const float over = (o1 > o2) ? o1 : o2;
		p->pull += (over > 0) ? -(1.0f + 2.0f * over) * dt : 1.0f * dt; if (p->pull < 0.1f) p->pull = 0.1f; if (p->pull > 1.0f) p->pull = 1.0f; }
	if (tr && (p->tr += dt) >= 2.0f) {
		p->tr = 0;
		fprintf(stderr, "[fatigue] id=%u type=%d path=%d mc=%d g=%.2f ias=%.0f roll=%.2f force=%.2f F=%.3f rollcap=%.2f glim=%.1f pull=%.2f gsus=%.2f graw=%.2f\n", p->id, ma_type(a),
		        path, int(a->movecode), g, 400.0 * sqrt(q), p->roll, force, p->F, ma_roll_capacity(a), ma_g_limit(a), p->pull, p->gsus, graw);
	}
	if (a->movecode == AUTO_COMBAT) ma_tom_tick(a, p, dt, g);
	p->ohdg = SWord(a->hdg);
	p->roll = 0;
}
#endif




SLong	const	MAXAUTOTHRUST = 100;	
//static 
RequiredBankDataElement 	CombatReqBankData[] =
			{
				{ANGLES_FifthDeg, ANGLES_10Deg},
				{ANGLES_2Deg, ANGLES_20Deg},
				{ANGLES_10Deg, ANGLES_45Deg},
				{ANGLES_45Deg, MAXCOMBATROLL},
				{ANGLES_180BareDeg, MAXCOMBATROLL}
//DeadCode CSB 28/03/99					{ANGLES_FifthDeg, ANGLES_0Deg},
//DeadCode CSB 28/03/99					{ANGLES_HalfDeg, ANGLES_3Deg},
//DeadCode CSB 28/03/99					{ANGLES_1Deg, ANGLES_4Deg},
//DeadCode CSB 28/03/99					{ANGLES_2Deg, ANGLES_5Deg},
//DeadCode CSB 28/03/99					{ANGLES_3Deg, ANGLES_10Deg},
//DeadCode CSB 28/03/99					{ANGLES_4Deg, ANGLES_15Deg},
//DeadCode CSB 28/03/99					{ANGLES_5Deg, ANGLES_22Deg},
//DeadCode CSB 28/03/99					{ANGLES_6Deg, ANGLES_30Deg},
//DeadCode CSB 28/03/99					{ANGLES_10Deg, ANGLES_45Deg},
//DeadCode CSB 28/03/99	//DeadCode RDH 17Jul96 				{ANGLES_22Deg, ANGLES_60Deg},
//DeadCode CSB 28/03/99					{ANGLES_30Deg, MAXCOMBATROLL},
//DeadCode CSB 28/03/99					{ANGLES_180BareDeg, MAXCOMBATROLL}
			};

Float	AirStruc::CalcSimpleAccel ()
{
	if(classtype->aerobaticfactor == AEROBATIC_LOW)
	{
		INT3;
		return(0);
	}
	
	MODEL& MOD = *fly.pModel;
	ACMMODEL& ACM = *fly.pAcmModel;
	// Linux/GCC port (redundant, acmmodel.h defines it): extern enum ACM_COMMAND;
	Float Thrust, Drag, Lift;
	Float mass;		//kg		
	Float accel;
	Float fvel = vel * 0.1;	//m/s
	SWord sinroll, cosroll;
	Float N;
//DeadCode CSB 24/09/98		Float	aoa;
//DeadCode CSB 24/09/98		Float	Ps;			//m/s
//DeadCode CSB 24/09/98		Float	area;		//m^2
//DeadCode CSB 24/09/98		Float	CD;
//DeadCode CSB 24/09/98		Float	accel, accelh;
//DeadCode CSB 24/09/98		SWord	sinroll, cosroll, c, s;

//***************************************************************************************
	//CSB 24/09/98
	mass = classtype->weightempty * 0.01 + (weap.left.int_fuel + weap.centre.int_fuel + weap.right.int_fuel) * 0.00001;	// -> kg
	PMAINPLANE pMainPlane = MOD.MainPlaneList;

	FP qS = 0.5 * MOD.AmbDensity * /* 10000 * */ fvel * fvel * pMainPlane->Area; //* 0.0001;

	Math_Lib.high_sin_cos(roll, sinroll, cosroll);
	if(cosroll == 0) 
		Lift = mass * 9 * 9.8065;
	else
		Lift = mass * 32767 / cosroll * 9.8065;
	if(Lift < 0) Lift *= -1;

	if(Lift > 1.25 * qS) Lift = 1.25 * qS;

	Drag = pMainPlane->Cdo * qS + pMainPlane->k * Lift * Lift / qS;

	PENGINE pEngine = MOD.EngineList;

	UByte	FF = FlyingFactor();
//DeadCode CSB 03/06/99		fly.thrustpercent = 70 + (FF * FF * 30)/65536;
	AircraftAnimData*	adptr = (AircraftAnimData *)Anim;

	Float ThrustPercent =  fly.thrustpercent;
	ThrustPercent       *= (255.0 - Float(adptr->ENGINELEFTIN)) / 255.0;

	if(pEngine->Type == ET_JET)
		Thrust = pEngine->T100 * ThrustPercent * MOD.AmbDensity / 0.0001225 * 0.01;
	else
		Thrust = pEngine->T100 * ThrustPercent * MOD.AmbDensity / 0.0001225 * 0.1 / vel;

	Drag *= (2048 - adptr->LEFTWINGOUT - adptr->LEFTWINGIN - adptr->RIGHTWINGIN - adptr->RIGHTWINGOUT);
	Drag /= 2048;

	Drag *= (512.0 - FP(FF)) / 256.0;

//DeadCode CSB 29/04/99		if(!fly.thrustpercent)
//DeadCode CSB 29/04/99			Drag += fvel * fvel;	//AirBrakes
	
	accel = (Thrust - Drag)/mass;
	if(fvel != 0)
		accel -= 9.8065 * 0.1 * vely/fvel ; 

	return(accel);


//***************************************************************************************
	
//DeadCode CSB 24/09/98	//	Float temp, pres;
//DeadCode CSB 24/09/98		Float dens;									//kg/m^3
//DeadCode CSB 24/09/98	//DeadCode CSB 15/09/98		MMC.Sky.Ambient (World.Y, dens, temp, pres);
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		dens = MOD.AmbDensity * 10000;
//DeadCode CSB 24/09/98	//	pres = MOD.AmbPres;
//DeadCode CSB 24/09/98	//	temp = MOD.AmbTemp;
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		mass = 0.00001 * (weap.centre.int_fuel +						//cg
//DeadCode CSB 24/09/98							   classtype->weightempty * 1000);		//centi kilo g
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		area = 0;
//DeadCode CSB 24/09/98		PMAINPLANE pMainPlane = MOD.MainPlaneList;
//DeadCode CSB 24/09/98		while (pMainPlane != NULL)
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			area += pMainPlane->Area * 0.0001;				//cm^2
//DeadCode CSB 24/09/98			pMainPlane = pMainPlane->List.NextItem ();
//DeadCode CSB 24/09/98		}
//DeadCode CSB 24/09/98		FP modincArea = area;
//DeadCode CSB 24/09/98		FP moddecArea = area;
//DeadCode CSB 24/09/98		if (Manual_Pilot.ControlledAC2 != this)
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			UByte	FightingFactor = FlyingFactor();
//DeadCode CSB 24/09/98			FP areainc = area * (256 - FightingFactor)/1024;
//DeadCode CSB 24/09/98			modincArea = area + areainc * 2;
//DeadCode CSB 24/09/98			moddecArea = area - areainc;
//DeadCode CSB 24/09/98		}
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		Math_Lib.high_sin_cos(roll, sinroll, cosroll); 
//DeadCode CSB 24/09/98		Float	n; 
//DeadCode CSB 24/09/98		if (cosroll == 0)
//DeadCode CSB 24/09/98			n = 10;
//DeadCode CSB 24/09/98		else
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			n = 32767;
//DeadCode CSB 24/09/98			n = n/cosroll;
//DeadCode CSB 24/09/98		}
//DeadCode CSB 24/09/98		if (n < 0)
//DeadCode CSB 24/09/98			n = -n;
//DeadCode CSB 24/09/98		if (n > 7 )
//DeadCode CSB 24/09/98			n = 7;
//DeadCode CSB 24/09/98	 	Float wCL = (2 * n * mass * 9.80655) / (dens * fvel * fvel * moddecArea);
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		//assume only one set of wings
//DeadCode CSB 24/09/98		pMainPlane = MOD.MainPlaneList;
//DeadCode CSB 24/09/98		// Calc aoa
//DeadCode CSB 24/09/98		if (!pMainPlane->pClCurve->GetIndex (wCL, aoa))
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			wCL = 0;
//DeadCode CSB 24/09/98			pMainPlane->pClCurve->GetMaxValue (wCL, aoa);
//DeadCode CSB 24/09/98		}
//DeadCode CSB 24/09/98		// Calc CD
//DeadCode CSB 24/09/98		if ((aoa > pMainPlane->AoaMin) && (aoa < (pMainPlane->AoaMax)))
//DeadCode CSB 24/09/98			CD = pMainPlane->Cdo + (pMainPlane->k * wCL * wCL);
//DeadCode CSB 24/09/98		else
//DeadCode CSB 24/09/98			CD = pMainPlane->pCdCurve->GetValue (aoa);
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98	//perhaps we don't need this?	CD *= MainPlaneList->pCompCdCurve->GetValue (Mach);
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		// Calc Drag
//DeadCode CSB 24/09/98		Drag = CD * 0.5 * dens * fvel * fvel * modincArea;
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98		MOD.ProcessEngines (this);
//DeadCode CSB 24/09/98	
//DeadCode CSB 24/09/98	   	PENGINE pEngine = MOD.EngineList;
//DeadCode CSB 24/09/98		Thrust = 0;
//DeadCode CSB 24/09/98		while (pEngine != NULL)
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			Thrust += pEngine->Thrust;
//DeadCode CSB 24/09/98			pEngine = pEngine->List.NextItem();
//DeadCode CSB 24/09/98		}
//DeadCode CSB 24/09/98		if (fvel == 0)
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			accel = (Thrust - Drag)/ mass; 
//DeadCode CSB 24/09/98		}else
//DeadCode CSB 24/09/98		{
//DeadCode CSB 24/09/98			Ps = ((Thrust - Drag) * fvel) / (mass * 9.80655);
//DeadCode CSB 24/09/98			accel = (Ps - vely/10) * 9.80655/fvel;
//DeadCode CSB 24/09/98		}
//DeadCode CSB 24/09/98		return(accel);
//DeadCode CSB 24/09/98	//	Math_Lib.high_sin_cos(pitch, s, c);
//DeadCode CSB 24/09/98	//
//DeadCode CSB 24/09/98	//	accely = s * accel / 32768;
//DeadCode CSB 24/09/98	//	accelh = c * accel / 32768;
//DeadCode CSB 24/09/98	//
//DeadCode CSB 24/09/98	//	Math_Lib.high_sin_cos(hdg, s, c);
//DeadCode CSB 24/09/98	//	accelx = s * accelh / 32768;
//DeadCode CSB 24/09/98	//	accelz = c * accelh / 32768;
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		CalcMaxPitchRate
//Author		Craig Beeston 
//Date			Tue 1 June 1999
//
//Description	Calculates the maximum pitch rate for a simple movecode
//				Limited by G and stall limit
//
//Inputs		
//
//Returns		Max Pitch rate in Rowans per second
//
//------------------------------------------------------------------------------
UWord AirStruc::CalcMaxPitchRate()
{
	if(classtype->aerobaticfactor == AEROBATIC_LOW)
		return(classtype->maxpitchrate);

	SWord FF = FlyingFactor();
	SWord maxG = 1 + (10 * FF) / 256;
#if defined(BOB_LINUX)
	if ((movecode == AUTO_COMBAT) && (Status.size == AIRSTRUCSIZE))	/* 49c: the pilot's G tolerance (felt G incl. 1 G) */
	{ const float n = ma_g_limit(AirStrucPtr(this)); if (n - 1.0f < maxG) maxG = (n < 2.0f) ? 1 : SWord(n - 1.0f); }
#endif
	ULong pitchrateMG = ANGLES_90Deg;
	if(vel > 150) pitchrateMG = (1043100 * maxG) / vel;	//rowans/sec for 9 g
	FP maxpitchrate = (ANGLES_50Deg * FlyingFactor()) / 256.0;
	maxpitchrate *= vel * fly.pModel->AmbDensity / (0.0001225 * classtype->maxvel);
		//Pretty good figure for all MiG Alley Aircraft		//CSB 01/06/99	

	if(maxpitchrate > pitchrateMG) 
		maxpitchrate = pitchrateMG;
	
/* TEST CODE CSB 01/06/99 */	
/* TEST CODE CSB 01/06/99 */		
/* TEST CODE CSB 01/06/99 */	
/*	MODEL& MOD = *fly.pModel;
	int flightno = formpos & 0x03;
	int flightpo = (formpos & 0x30) / 16;
	int grouppos = 1 + flightno * 4 + flightpo;
	int line = grouppos + 1;
	if((MOD.Type == AT_MIG15) || (MOD.Type == AT_MIG15BIS))
		line += 9;

	PrintVar(60, line, "F %.0f ", FP(FF));

	FP MPR = maxpitchrate / 182.04;
	if(maxpitchrate == pitchrateMG)
	{
		PrintVar(65, line, "%.0f x G ", FP(maxG));
		PrintVar(70, line, " %.2f ", FP(MPR));
	}
	else
		PrintVar(65, line, "STALL %.2f ", FP(MPR));

	FP APR = Model_InstTurnRate(this) * 5730.0;
	if(APR < 0) APR *= -1;
	if(APR > MPR)
		PrintVar(76, line, "%.1f", FP(APR));
*/
/* TEST CODE CSB 01/06/99 */
/* TEST CODE CSB 01/06/99 */
/* TEST CODE CSB 01/06/99 */
	return((UWord)maxpitchrate);
}


//DeadCode CSB 29/03/99	//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//DeadCode CSB 29/03/99	//Procedure		TurnFight
//DeadCode CSB 29/03/99	//Author		R. Hyde 
//DeadCode CSB 29/03/99	//Date			Fri 13 Mar 1998
//DeadCode CSB 29/03/99	//
//DeadCode CSB 29/03/99	//Description	
//DeadCode CSB 29/03/99	//
//DeadCode CSB 29/03/99	//Inputs		
//DeadCode CSB 29/03/99	//
//DeadCode CSB 29/03/99	//Returns	
//DeadCode CSB 29/03/99	//
//DeadCode CSB 29/03/99	//------------------------------------------------------------------------------
//DeadCode CSB 29/03/99	void	AirStruc::SimpleTurningFight()
//DeadCode CSB 29/03/99	{
//DeadCode CSB 29/03/99		SWord	reqdeltahdg,reqdeltahdg_sign;
//DeadCode CSB 29/03/99		UWord	reqdeltahdg_pos;
//DeadCode CSB 29/03/99		COORDS3D	target;
//DeadCode CSB 29/03/99		MODEL& MOD = *fly.pModel;
//DeadCode CSB 29/03/99		AirStrucPtr	unfriendly = *ai.unfriendly;
//DeadCode CSB 29/03/99		SLong vx, vy, vz;
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99		reqdeltahdg = 	hdg - HdgIntercept;
//DeadCode CSB 29/03/99		Math_Lib.Pos (reqdeltahdg,reqdeltahdg_pos,reqdeltahdg_sign);
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99		if (!ai.unfriendly)			//AMM13nov98
//DeadCode CSB 29/03/99			return;			//AMM13nov98
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99		if (	(reqdeltahdg_pos < (SWord)ANGLES_90Deg)
//DeadCode CSB 29/03/99			)		
//DeadCode CSB 29/03/99		{
//DeadCode CSB 29/03/99			CalcLead(*this, *ai.unfriendly,vx,vy,vz, TRUE);
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99			target.X = ai.unfriendly->World.X + vx;
//DeadCode CSB 29/03/99			target.Y = ai.unfriendly->World.Y + vy;
//DeadCode CSB 29/03/99			target.Z = ai.unfriendly->World.Z + vz;
//DeadCode CSB 29/03/99			InterceptandRange (&target);
//DeadCode CSB 29/03/99		}else
//DeadCode CSB 29/03/99			target = ai.unfriendly->World;
//DeadCode CSB 29/03/99		if (		(classtype->visible == MIG15)
//DeadCode CSB 29/03/99				&&	(		(ai.combatskill >= SKILL_ACE)
//DeadCode CSB 29/03/99						||	(ai.morale < MORALE_GOOD)
//DeadCode CSB 29/03/99					)
//DeadCode CSB 29/03/99			)
//DeadCode CSB 29/03/99			SimpleTurnWithPitchChange(HdgIntercept, FALSE, target);
//DeadCode CSB 29/03/99		else
//DeadCode CSB 29/03/99			SimpleTurnWithPitchChange(HdgIntercept, TRUE, target);
//DeadCode CSB 29/03/99	}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		TurnFight
//Author		R. Hyde 
//Date			Fri 13 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleTurningFight()
{
	COORDS3D	target;
	MODEL& MOD = *fly.pModel;
	ACMMODEL& ACM = *fly.pAcmModel;
	AirStrucPtr	unfriendly = *ai.unfriendly;
	SLong vx, vy, vz;
	SWord	reqdeltahdg,reqdeltahdg_sign;
	UWord	reqdeltahdg_pos;
	SWord	reqdeltapitch,reqdeltapitch_sign;
	UWord	reqdeltapitch_pos;
	
	if (!ai.unfriendly)		
		return;			

	target.X = ai.unfriendly->World.X;
	target.Y = ai.unfriendly->World.Y;
	target.Z = ai.unfriendly->World.Z;
	InterceptandRange (&target);

	reqdeltahdg   =	HdgIntercept - hdg;
	Math_Lib.Pos (reqdeltahdg,reqdeltahdg_pos,reqdeltahdg_sign);
	SimpleMoveToThrust (ThrustfromFF());							  //RDH 06/05/99

//DeadCode CSB 03/06/99		if((reqdeltahdg_pos < ANGLES_90Deg) && (Range < ENGAGERANGE))
	if((reqdeltahdg_pos < ANGLES_45Deg) && (Range < WEAPONSRANGE))
	{
		if (	(classtype->phrasename == PHRASE_MIGS)
//DEADCODE RDH 10/05/99 		if( (classtype->visible == MIG15)
			&& ((ai.combatskill >= SKILL_ACE) || (ai.morale < MORALE_GOOD)) )
			if(reqdeltahdg_pos < ANGLES_90Deg)
			{
				if	(		(ai.manoeuvre != MANOEUVRE_DIVEANDZOOM)			  //RDH 04/07/99
						&&	(unfriendly->classtype->aerobaticfactor != AEROBATIC_LOW)						
					)
				{
					if(Range < WEAPONSRANGE)
						SimpleMoveToThrust(0);
					else
						SimpleMoveToThrust (ThrustfromFF());
					if(Range < 0.5 * WEAPONSRANGE)
						ACM.SpeedBrakesOut();
					else
						ACM.SpeedBrakesIn();
				}
			}

		CalcLead(*this, *ai.unfriendly,vx,vy,vz, FALSE);
		target.X = ai.unfriendly->World.X + vx;
		target.Y = ai.unfriendly->World.Y + vy;
		target.Z = ai.unfriendly->World.Z + vz;
		InterceptandRange (&target);
		reqdeltahdg   =	HdgIntercept - hdg;
		reqdeltapitch =	PitchIntercept - pitch;
		Math_Lib.Pos (reqdeltahdg,reqdeltahdg_pos,reqdeltahdg_sign);
		Math_Lib.Pos (reqdeltapitch,reqdeltapitch_pos,reqdeltapitch_sign);
//DeadCode CSB 06/04/99	PrintVar(60, 5, "DeltaPitch %.0f ", (FP)( (PitchIntercept - pitch) / 182.04) );
//DeadCode CSB 06/04/99	PrintVar(60, 6, "DeltaHeadg %.0f ", (FP)( (HdgIntercept - hdg) / 182.04) );
//DeadCode CSB 06/04/99	PrintVar(60, 7, "Targ Range %.0f ", (FP)( (Range) / 100.0) );
		
		if(reqdeltapitch != 0)
		{
			SWord DesRoll = Math_Lib.arctan(reqdeltahdg, reqdeltapitch);
			SimpleMoveToRoll(DesRoll);
		}

		SWord sin_roll, cos_roll;
		Math_Lib.high_sin_cos(roll, sin_roll, cos_roll);
//DeadCode CSB 01/06/99			FP MaxDeltaPitch = (FP)classtype->maxpitchrate * (FP)Timer_Code.FRAMETIME / 100.0 / 32768.0;
		FP MaxDeltaPitch = (FP)CalcMaxPitchRate() * (FP)Timer_Code.FRAMETIME / (100.0 * 32768.0);
		FP MaxDeltaYaw   = MaxDeltaPitch * sin_roll;
		MaxDeltaPitch	 = MaxDeltaPitch * cos_roll;

		SWord	maxdeltapitch_sign, maxdeltayaw_sign;
		UWord	maxdeltapitch_pos,  maxdeltayaw_pos;
		Math_Lib.Pos((SWord)MaxDeltaPitch, maxdeltapitch_pos, maxdeltapitch_sign);
		Math_Lib.Pos((SWord)MaxDeltaYaw  , maxdeltayaw_pos,   maxdeltayaw_sign);
		FP factor1 = 1, factor2 = 1;
		if(maxdeltapitch_pos > reqdeltapitch_pos) factor1 = reqdeltapitch_pos / maxdeltapitch_pos;
		if(maxdeltayaw_pos   > reqdeltahdg_pos)   factor2 = reqdeltahdg_pos / maxdeltayaw_pos;
		if(factor1 > factor2) factor1 = factor2;
		pitch += SWord((FP)MaxDeltaPitch * (FP)factor1);
		hdg   += SWord((FP)MaxDeltaYaw   * (FP)factor1);
		fly.cpitch = pitch;

		hdg -= (Angles) CalcHdg ();	//This means that all the heading change is done inside this routine
	}
	else
	{
		if((roll > ANGLES_90Deg) || (roll < ANGLES_270Deg))
			SimpleMoveToRoll(0);
		else
			SimpleMoveToHdg (HdgIntercept);	
		AutoCalcPitch();
	}
	
	if	(ai.manoeuvre != MANOEUVRE_DIVEANDZOOM)			  //RDH 04/07/99
	{
		if((reqdeltahdg_pos < ANGLES_10Deg) && (reqdeltapitch_pos < ANGLES_10Deg))
		{
			advantage = TRUE;
			disadvantage = FALSE;
			MoraleMod();
			SetManoeuvreTime (1000);
		}

		if (		(fly.leadflight)					
				&&	 (fly.leadflight->ai.manoeuvre == MANOEUVRE_TOPCOVER)
			)//force manoeuvre to allow disengage
				ai.manoeuvre = MANOEUVRE_SELECT;

		if(!TimeLeft())
		{
			if(    (World.Y < FT_1000)
				|| (Range > ENGAGERANGE)
				|| (pitch > ANGLES_30Deg)
				|| (pitch < ANGLES_330Deg) )
				ai.manoeuvre = MANOEUVRE_SELECT;
		}
		if(Math_Lib.rnd(4 * 65536) < (256 - FlyingFactor()))	//Poor skill -> every 20 secs
			ai.manoeuvre = MANOEUVRE_SCISSORS;
	}
}				

//DeadCode CSB 29/03/99		reqdeltahdg = 	hdg - HdgIntercept;
//DeadCode CSB 29/03/99		Math_Lib.Pos (reqdeltahdg,reqdeltahdg_pos,reqdeltahdg_sign);
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99		if (!ai.unfriendly)			//AMM13nov98
//DeadCode CSB 29/03/99			return;			//AMM13nov98
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99		if (	(reqdeltahdg_pos < (SWord)ANGLES_90Deg)
//DeadCode CSB 29/03/99			)		
//DeadCode CSB 29/03/99		{
//DeadCode CSB 29/03/99			CalcLead(*this, *ai.unfriendly,vx,vy,vz, TRUE);
//DeadCode CSB 29/03/99	
//DeadCode CSB 29/03/99			target.X = ai.unfriendly->World.X + vx;
//DeadCode CSB 29/03/99			target.Y = ai.unfriendly->World.Y + vy;
//DeadCode CSB 29/03/99			target.Z = ai.unfriendly->World.Z + vz;
//DeadCode CSB 29/03/99			InterceptandRange (&target);
//DeadCode CSB 29/03/99		if (		(classtype->visible == MIG15)
//DeadCode CSB 29/03/99				&&	(		(ai.combatskill >= SKILL_ACE)
//DeadCode CSB 29/03/99						||	(ai.morale < MORALE_GOOD)
//DeadCode CSB 29/03/99					)
//DeadCode CSB 29/03/99			)
//DeadCode CSB 29/03/99			SimpleTurnWithPitchChange(HdgIntercept, FALSE, target);
//DeadCode CSB 29/03/99		else
//DeadCode CSB 29/03/99			SimpleTurnWithPitchChange(HdgIntercept, TRUE, target);
//DeadCode CSB 29/03/99	}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
void	AirStruc::SimpleTurnWithPitchChange(ANGLES	HdgIntercept, Bool sitbehindtarget,COORDS3D	target)
{
	if (CombatFactor() > 200)
	{
		if (classtype->phrasename == PHRASE_MIGS)
//DEADCODE RDH 10/05/99 		if (classtype->visible == MIG15)
			SimpleTurnFightSub(HdgIntercept, sitbehindtarget, ANGLES_5Deg, target);	
		else if (World.Y > FT_10000)
			SimpleTurnFightSub(HdgIntercept, sitbehindtarget, ANGLES_0Deg, target);	
		else
			SimpleTurnFightSub(HdgIntercept, sitbehindtarget, ANGLES_0Deg, target);	

	}else
		SimpleTurnFightSub(HdgIntercept, sitbehindtarget, ANGLES_0Deg, target);

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		TurnFightSub
//Author		R. Hyde 
//Date			Fri 13 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleTurnFightSub(ANGLES	HdgIntercept, Bool sitbehindtarget, ANGLES	deltapitch,COORDS3D	target)
{
	ACMMODEL& ACM = *fly.pAcmModel;
	MODEL& MOD = *fly.pModel;
	AirStrucPtr	unfriendly = *ai.unfriendly;

	SLong temp_range;
	temp_range = Range; 	

	SWord	reqdeltahdg,reqdeltahdg_sign;
	UWord	reqdeltahdg_pos;
	SWord	reqdeltapitch,reqdeltapitch_sign;
	UWord	reqdeltapitch_pos;
	FP desiredturn;
	SLong		time;

	reqdeltahdg = 	hdg - HdgIntercept;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust (ThrustfromFF());
			SetManoeuvreTime (1000);
			ACM.maxdeltaforclosework = 0.0;
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			ACM.MinHeight (FT_2000);
 			Math_Lib.Pos (reqdeltahdg,reqdeltahdg_pos,reqdeltahdg_sign);
			reqdeltapitch = PitchIntercept - fly.cpitch;//	fly.cpitch - PitchIntercept;		//CHECK THIS !!!!
			Math_Lib.Pos (reqdeltapitch,reqdeltapitch_pos,reqdeltapitch_sign);

			ANGLES delta = unfriendly->hdg + ANGLES_180Deg + ANGLES_30Deg - hdg;

			{
				if (sitbehindtarget)
					SlowDown(reqdeltahdg_pos);
	
				SimpleTurnFightTargetNotOnNose(reqdeltahdg, reqdeltapitch, reqdeltapitch_sign, reqdeltapitch_pos, deltapitch);

				if  (		(!ManoeuvreTimeLeft())
					)
				{
					if	(ACM.maxdeltaforclosework <= 0.1)
						ai.manoeuvre = MANOEUVRE_SELECT;
					else
					{
						if (		(fly.leadflight)					
								&&	 (fly.leadflight->ai.manoeuvre == MANOEUVRE_TOPCOVER)
							)//force manoeuvre to allow disengage
						{
								ai.manoeuvre = MANOEUVRE_SELECT;
						}else
						{//on tail of target so keep in manoeuvre and up morale
							advantage = TRUE;
							disadvantage = FALSE;
							MoraleMod();
							SetManoeuvreTime (1000);
						}
					}
				}
			}
		}
	}

}
void	AirStruc::SimpleTurnFightTargetNotOnNose(SWord	reqdeltahdg, SWord	reqdeltapitch, SWord	reqdeltapitch_sign, UWord	reqdeltapitch_pos, ANGLES	deltapitch)
{

	SWord delta;
	SWord reqroll;
	SWord dpitch;

	SWord PI = PitchIntercept;
	if  (		(PI < 0)
			&&	(Range > WEAPONSRANGE)
		)
		PI = 0;
	delta = PI + deltapitch;
	if  (		(delta < ANGLES_0Deg)
			&&	(ai.manoeuvre != MANOEUVRE_DIVEANDZOOM)
			&&	(deltapitch << ANGLES_180Deg)				//if he really wants to go down then let him
			&&	(		(Range > WEAPONSRANGE)
					||	(classtype->phrasename == PHRASE_MIGS)
//DEADCODE RDH 10/05/99 					||	(classtype->visible == MIG15)
						
				)
		)	
		delta = ANGLES_0Deg;
	reqdeltapitch = 	delta - fly.cpitch;
	Math_Lib.Pos (reqdeltapitch,reqdeltapitch_pos,reqdeltapitch_sign);
	if (reqdeltapitch_pos > ANGLES_40Deg)
		dpitch = ANGLES_40Deg;
	else
		dpitch = reqdeltapitch_pos;
	if (reqdeltapitch_sign < 0)
		dpitch = - dpitch;

	
	SimpleMoveToHdg (HdgIntercept);	
	PitchIntercept = (Angles) dpitch;
	AutoCalcPitch();
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ChkAtDesiredHdg
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
Bool	AirStruc::ChkAtDesiredHdg (ANGLES desiredhdg,ANGLES target)

{
	SWord deltahdg;
	UWord	deltahdg_pos;
	SWord	deltahdg_sign;

	deltahdg = desiredhdg -	hdg;
	Math_Lib.Pos (((SWord)deltahdg),deltahdg_pos,deltahdg_sign);
	if (deltahdg_pos < target)
	{
		return (TRUE);
	}else
	{
		return (FALSE);
	}
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ChkAtDesiredHdg
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
Bool	AirStruc::ChkAtDesiredRoll (ANGLES desiredroll,ANGLES target)

{
	SWord deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign;

	deltaroll = desiredroll -	roll;
	Math_Lib.Pos (((SWord)deltaroll),deltaroll_pos,deltaroll_sign);
	if (deltaroll_pos < target)
	{
		return (TRUE);
	}else
	{
		return (FALSE);
	}
}
 
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		DesiredPitchChk
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
SWord	AirStruc::DesiredPitchChk (ANGLES desiredpitch,ANGLES target)

{
	SWord deltapitch;
	UWord	deltapitch_pos;
	SWord	deltapitch_sign;

	deltapitch = desiredpitch -	fly.cpitch;
	Math_Lib.Pos (((SWord)deltapitch),deltapitch_pos,deltapitch_sign);
	if (deltapitch_pos < target)
	{
		return (TRUE);
	}else
	{
		return (FALSE);
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		DesiredAcPitchChk
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
SWord	AirStruc::DesiredAcPitchChk (ANGLES desiredpitch,ANGLES target)

{
	SWord deltapitch;
	UWord	deltapitch_pos;
	SWord	deltapitch_sign;

	deltapitch = desiredpitch -	pitch;
	Math_Lib.Pos (((SWord)deltapitch),deltapitch_pos,deltapitch_sign);
	if (deltapitch_pos < target)
	{
		return (TRUE);
	}else
	{
		return (FALSE);
	}


}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ChkAtDesiredAcHdg
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
Bool	AirStruc::ChkAtDesiredAcHdg (ANGLES desiredhdg,ANGLES target)

{
	SWord deltahdg;
	UWord	deltahdg_pos;
	SWord	deltahdg_sign;

	deltahdg = desiredhdg -	hdg;
	Math_Lib.Pos (((SWord)deltahdg),deltahdg_pos,deltahdg_sign);
	if (deltahdg_pos < target)
	{
		return (TRUE);
	}else
	{
		return (FALSE);
	}
}

void  AirStruc::SimpleMakeForFriendly()
{
}
void  AirStruc::SimpleImmelmann()
{
}

void  AirStruc::SimpleGainSpeed()
{
	SWord 	deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SetManoeuvreTime (2000);
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			SimpleMoveToRoll (ANGLES_0Deg);
			if (World.Y < FT_1000)
				PitchIntercept =  ANGLES_0Deg;
			else
				PitchIntercept =  ANGLES_350Deg;
			AutoCalcPitch();

			if  (!ManoeuvreTimeLeft())
				ai.manoeuvre = MANOEUVRE_SELECT;

			break;
		}
	}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LeadUnFriendly
//Author		R. Hyde 
//Date			Fri 15 Mar 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLeadUnFriendly ()
{

	COORDS3D	target;
	SLong		t,actualrange;
	AirStrucPtr	unfriendly = *ai.unfriendly;

			target.X = ai.unfriendly->World.X;
			target.Y = ai.unfriendly->World.Y;
			target.Z = ai.unfriendly->World.Z;
			InterceptandRange (&target);
			actualrange = Range;

			//closing velocity
			SLong deltax = velx - unfriendly->velx;
			SLong deltay = vely - unfriendly->vely;
			SLong deltaz = velz - unfriendly->velz;
			Math_Lib.Intercept (deltax,deltay,deltaz,Range,HdgIntercept,PitchIntercept);
			//Range is a velocity in 10cm/s
			if (Range != 0)
			{
				t = actualrange / Range;
			}else
			{
				t = 0;
			}
			//when a long way off don't attempt to lead
			if (t >2000)
			t = 0;

			target.X = ai.unfriendly->World.X + (unfriendly->velx * t / 10);
			target.Y = ai.unfriendly->World.Y + (unfriendly->vely * t / 10);
			target.Z = ai.unfriendly->World.Z + (unfriendly->velz * t / 10);
			InterceptandRange (&target);
			AutoCalcPitch();
			SimpleMoveToHdg (HdgIntercept);

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		GatherSpeed
//Author		R. Hyde 
//Date			Fri 15 Mar 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleGatherSpeed ()

{
	COORDS3D	target;

		target.X = ai.unfriendly->World.X;
		target.Y = ai.unfriendly->World.Y;
		target.Z = ai.unfriendly->World.Z;
		InterceptandRange (&target);
		PitchIntercept = ANGLES_330Deg;
		AutoCalcPitch();
 		SimpleMoveToHdg (HdgIntercept);
}



//Simple acm routines											//RDH 11Oct96

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SimpleTopCover
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	The followers are supposed to go near to home line or
//				not depending on skill
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleTopCover ()

{
  if (formpos == 0)
  {//leader
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (500);
			ai.desiredroll = - (ANGLES)ANGLES_30Deg +
 						(Angles)(ANGLES_60Deg * Math_Lib.rnd(2)); 
			ai.ManStep = PHASE1;
		}


		case PHASE1:
		{
			SimpleTightClimbingTurn (ANGLES_5Deg);
		if (!ManoeuvreTimeLeft())
			{
				SetManoeuvreTime (500);
				ai.ManStep = PHASE2;
 			}

			break;
		}
		case PHASE2:
		{
			SimpleTightClimbingTurn (ANGLES_0Deg);

			if (!ManoeuvreTimeLeft())
			{
				ai.ManStep = PHASE3;
 			}

			break;
		}
		case PHASE3:
		{
			SimpleTightClimbingTurn (ANGLES_0Deg);

			if (AllOnTopCover ())								//JIM 25Nov96
			{													//JIM 25Nov96
				Art_Int.AllBreakOff(this);	//fly.leadflight);	//JIM 25Nov96
//TEMP			 	Persons_2.AddMessage(UserMsg::GOHOME,TEXT_SQUADRONREGROUPED,NULL);
			}													//JIM 25Nov96
			else												//JIM 25Nov96
			{
		    	ai.ManStep = PHASE1;
				SetManoeuvreTime (1000);
			}

			break;
		}
  	}
  }else
  {
  	   AirStruc* leader = FindGroupLeader();
	   if (leader == Persons2::PlayerGhostAC)
	   {
			Art_Int.BreakOff(this);
	   }else
	   {

			switch (ai.ManStep)
			{
				case PHASE0:
				{
 					SimpleMoveToThrust (100);
					SetManoeuvreTime (500);
					ai.desiredroll = -(ANGLES)ANGLES_30Deg +
 								(Angles)(ANGLES_60Deg * Math_Lib.rnd(2)); 
					ai.ManStep = PHASE1;
				}


				case PHASE1:
				{
		//DeadCode RDH 13Dec96 			SimpleTightClimbingTurn (ANGLES_10Deg);
					SimpleTightClimbingTurn (ANGLES_5Deg);				//RDH 13Dec96
					if (!ManoeuvreTimeLeft())
					{
						SetManoeuvreTime (1000);
						ai.ManStep = PHASE2;
 					}

					break;
				}
				case PHASE2:
				{
					if (fly.leadflight->World.Y > World.Y)
					{
					SimpleTightClimbingTurn (ANGLES_0Deg);
					}else
					{
		//DeadCode RDH 13Dec96 			SimpleTightClimbingTurn (ANGLES_8Deg);
					SimpleTightClimbingTurn (ANGLES_5Deg);
					}
					break;
				}

  			}
	   }
  }

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SimpleTightClimbingTurn
//Author		R. Hyde 
//Date			Sat 30 Mar 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleTightClimbingTurn (ANGLES desiredpitch)
{
 
 	SWord	deltapitch;
 
 	deltapitch = desiredpitch - fly.cpitch;
 	PitchIntercept = desiredpitch; 
 	SimpleTightTurnSubSub (deltapitch);
 
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SimpleLookAround
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleLookAround ()
{

	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (500);								//RDH 25/6/99
			ai.desiredroll = - (ANGLES)ANGLES_45Deg +
 						(Angles)(ANGLES_90Deg * Math_Lib.rnd(2)); 
			ai.ManStep = PHASE1;
		}


		case PHASE1:
		{
			if (World.Y > classtype->fightceiling)
				SimpleTightClimbingTurn (ANGLES_0Deg);
			else
				SimpleTightClimbingTurn (ANGLES_5Deg);			//RDH 14Nov96
			if (!ManoeuvreTimeLeft())
			{
		    	ai.ManStep = PHASE0;
				if (formpos == 0)
				{
					if (AllButLdrNotOnCombat ())				//RDH 26Nov96
					{											//RDH 26Nov96
						ai.manoeuvre=MANOEUVRE_TOPCOVER;		//RDH 26Nov96
					}else										//RDH 26Nov96
					{											//RDH 26Nov96
						ai.manoeuvre=MANOEUVRE_LOOKROUND;		//RDH 26Nov96
					}											//RDH 26Nov96
				}else
				{
					ai.manoeuvre=MANOEUVRE_TOPCOVER;
				}
 			}

			break;
		}
	}

}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		BarrelRoll
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleBarrelRollAttack ( )

{
	SimpleScissors ( );
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Scissors
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleScissors ( )

{
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (1500);
			ai.ManStep = PHASE1;
			ai.desiredhdg = hdg + ANGLES_120Deg; 
		}
		case PHASE1:
		{
			if (!ManoeuvreTimeLeft())
			{
				ai.ManStep = PHASE3;
			}
			PitchIntercept = ANGLES_2Deg;
			AutoCalcPitch();
			if (SimpleMoveToDesiredHdg (ai.desiredhdg,ANGLES_100Deg))
			{
				ai.ManStep = PHASE2;
				ai.desiredhdg = hdg - ANGLES_240Deg; 
			}
			break;
		}
		case PHASE2:
		{
			if (!ManoeuvreTimeLeft())
			{
				ai.ManStep = PHASE3;
			}
			PitchIntercept = ANGLES_2Deg;
			AutoCalcPitch();
			if (SimpleMoveToDesiredHdg (ai.desiredhdg,ANGLES_100Deg))
			{
				ai.ManStep = PHASE1;
				ai.desiredhdg = hdg + ANGLES_240Deg; 
			}
			break;
		}
		case PHASE3:
		{

			ai.manoeuvre = MANOEUVRE_SELECT;
			break;
		}
	}


}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MildScissors
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleMildScissors ( )

{
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (1500);
			ai.ManStep = PHASE1;
			ai.desiredhdg = hdg - ANGLES_15Deg; 
		}
		case PHASE1:
		{
			if (!ManoeuvreTimeLeft())
			{
				ai.ManStep = PHASE3;
			}
			PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();
			if (SimpleMoveToDesiredHdg (ai.desiredhdg,ANGLES_5Deg))
			{
				ai.ManStep = PHASE2;
				ai.desiredhdg = hdg + ANGLES_30Deg; 
			}
			break;
		}
		case PHASE2:
		{
			if (!ManoeuvreTimeLeft())
			{
				ai.ManStep = PHASE3;
			}
			PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();
			if (SimpleMoveToDesiredHdg (ai.desiredhdg,ANGLES_5Deg))
			{
				ai.ManStep = PHASE1;
				ai.desiredhdg = hdg - ANGLES_30Deg; 
			}
			break;
		}
		case PHASE3:
		{

			ai.manoeuvre = MANOEUVRE_SELECT;
			break;
		}
	}

}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveToDesiredHdg
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
Bool	AirStruc::SimpleMoveToDesiredHdg (ANGLES desiredhdg,ANGLES target)

{
	Bool	retval;

	SimpleMoveToHdg (desiredhdg);
	retval = ChkAtDesiredHdg (desiredhdg,target);
	return (retval);
}

 //컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
 //Procedure		MoveToHdg
 //Author		R. Hyde 
 //Date			Wed 13 Dec 1995
 //
 //Description	based on difference between desired and actual hdg, fix aileron
 //				to give a roll, morale and skill will fix maxroll				
 //
 //Inputs		
 //
 //Returns	
 //
 //------------------------------------------------------------------------------
void	AirStruc::SimpleMoveToHdg (ANGLES desiredhdg)
 
{
 	SWord	reqdeltahdg, reqdeltahdg_sign;
	UWord	reqdeltahdg_pos;
 	SWord	reqroll,deltaroll;
 
 	reqdeltahdg = 	hdg - desiredhdg;
 	Math_Lib.Pos ((SWord) reqdeltahdg, reqdeltahdg_pos,reqdeltahdg_sign);
 	reqroll = FindRequiredRoll (reqdeltahdg,CombatReqBankData);
 
//DeadCode RDH 30Mar96  	deltaroll = (SWord) (reqroll - fly.acroll);
	ai.desiredroll = (Angles) reqroll;
	SimpleMoveToRoll (reqroll);
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveToRoll
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleMoveToRoll (SWord desiredroll)

{	//RequiredDRollSub: parameter is desired roll, return val is delta roll

	roll += (Angles) (RequiredDRollSub (desiredroll));
}



//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveToDesiredPitch
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
SWord	AirStruc::SimpleMoveToDesiredPitch (ANGLES desiredpitch,ANGLES target)

{
	SWord	retval;


	PitchIntercept = desiredpitch;
	AutoCalcPitch();
	retval = DesiredPitchChk (desiredpitch,target);
	return (retval);


}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		StraightandLevel
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleStraightandLevel ( )

{
	SWord 	deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SetManoeuvreTime (2000);
//DEADCODE RDH 20/06/99  			SimpleMoveToThrust (80);
			ai.ManStep = PHASE1;
	 		ai.desiredroll = ANGLES_0Deg;
		}
		case PHASE1:
		{
			//this manoeuvre is only done by poor pilots so we can reduce speed so that
			//other can catch up and attack							  //RDH 20/06/99
			if (vel < MPH250)
				SimpleMoveToThrust (80);
			else
				SimpleMoveToThrust (0);

			if (World.Y < FT_1000)
				PitchIntercept = ANGLES_4Deg;
			else
				PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();

			deltaroll = (SWord) (ai.desiredroll - roll);
			SimpleMoveToRoll (ai.desiredroll);
			Math_Lib.Pos (deltaroll,deltaroll_pos,deltaroll_sign);
			if (deltaroll_pos <ANGLES_2Deg)
			{
				ai.ManStep = PHASE2;
			}

			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;

		}
		case PHASE2:
		{
			//this manoeuvre is only done by poor pilots so we can reduce speed so that
			//other can catch up and attack							  //RDH 20/06/99
			if (vel < MPH250)
				SimpleMoveToThrust (80);
			else
				SimpleMoveToThrust (0);

			if (World.Y < FT_1000)
				PitchIntercept = ANGLES_10Deg;
			else
				PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();
			SimpleMoveToRoll (ai.desiredroll);
			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;

		}

	}

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		AcmSideSlip
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleAcmSideSlip ( )

{
	
	SimpleSustainedTurn ();
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SpinRecovery
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleSpinRecovery ()
{
	SimpleSustainedTurn ();

}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SpinOut
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleSpinOut ( )

{
	SWord	deltaroll;
	UWord	deltayaw_pos;
	SWord	deltayaw_sign;


	
	SLong groundlevel = _Collide.LowestSafeAlt(this,			//RDH 16Dec96
											this->pitch,		//RDH 16Dec96
											this->roll);		//RDH 16Dec96


	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (1);
			ai.ManStep = PHASE1;
			SetManoeuvreTime (600);
			ai.desiredroll = ANGLES_45Deg;
		}
		case PHASE1:
		{
			if ((World.Y - groundlevel) < FT_1000)				//RDH 16Dec96
				PitchIntercept =  ANGLES_8Deg;					//RDH 16Dec96
			else												//RDH 16Dec96
				PitchIntercept =  ANGLES_305Deg;

			AutoCalcPitch();
			SimpleMoveToRoll (ai.desiredroll);
			if (!ManoeuvreTimeLeft())
			{
				ai.desiredroll = ANGLES_0Deg;
				ai.ManStep = PHASE2;
				SetManoeuvreTime (400);
			}

			break;
		}
		case PHASE2:
		{
			if ((World.Y - groundlevel) < FT_1000)				//RDH 16Dec96
				PitchIntercept =  ANGLES_8Deg;					//RDH 16Dec96
			else												//RDH 16Dec96
			PitchIntercept =  ANGLES_340Deg;
			AutoCalcPitch();
			SimpleMoveToRoll (ai.desiredroll);
			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
	 			SimpleMoveToThrust (100);

			}else
			{
				
			}
			break;
		}

	}
		SetControlSurfaces ();

}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		DiveforHome
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleDiveforHome ( )

{
	SLong	temp_range;

	temp_range = Range; 	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
			InterceptandRange (&ai.homebase->World);
			ai.desiredhdg = HdgIntercept; 
			SetManoeuvreTime(0);
		}
		case PHASE1:
		{
			if(!TimeLeft())
			{
				Range = temp_range; 	
				if (Range < DANGERRANGE)
				{
					ai.manoeuvre = MANOEUVRE_SELECT;
				}

				if (World.Y < FT_1000)
					PitchIntercept = ANGLES_0Deg;
				else
					PitchIntercept = ANGLES_10Deg;
				Bool level = SimpleMoveToDesiredHdg (ai.desiredhdg, ANGLES_1Deg);
				if((AutoCalcPitch()) && (level))
					SetManoeuvreTime (1000);
				Range = temp_range; 	
				if (Range > ENGAGERANGE)
				{
					ai.manoeuvre = MANOEUVRE_SELECT;
				}
			}
		}
	}
}


//DeadCode CSB 28/03/99	{
//DeadCode CSB 28/03/99		SLong	temp_range;
//DeadCode CSB 28/03/99		temp_range = Range; 	
//DeadCode CSB 28/03/99	
//DeadCode CSB 28/03/99		switch (ai.ManStep)
//DeadCode CSB 28/03/99		{
//DeadCode CSB 28/03/99			case PHASE0:
//DeadCode CSB 28/03/99			{
//DeadCode CSB 28/03/99				if (World.Y < FT_1000)
//DeadCode CSB 28/03/99				{
//DeadCode CSB 28/03/99					PitchIntercept =  ANGLES_0Deg;
//DeadCode CSB 28/03/99					AutoCalcPitch();
//DeadCode CSB 28/03/99				}else
//DeadCode CSB 28/03/99				{
//DeadCode CSB 28/03/99					PitchIntercept =  ANGLES_340Deg;
//DeadCode CSB 28/03/99					AutoCalcPitch();
//DeadCode CSB 28/03/99					InterceptandRange (&ai.homebase->World);
//DeadCode CSB 28/03/99					SimpleMoveToHdg (HdgIntercept);
//DeadCode CSB 28/03/99				}
//DeadCode CSB 28/03/99				if (Range < MILES01)
//DeadCode CSB 28/03/99				{//within mile of home
//DeadCode CSB 28/03/99					ai.manoeuvre = MANOEUVRE_SELECT;
//DeadCode CSB 28/03/99				}
//DeadCode CSB 28/03/99				Range = temp_range; 	
//DeadCode CSB 28/03/99				if (Range > MILES01)
//DeadCode CSB 28/03/99				{//mile from action, reconsider
//DeadCode CSB 28/03/99					ai.manoeuvre = MANOEUVRE_SELECT;
//DeadCode CSB 28/03/99				}
//DeadCode CSB 28/03/99			}
//DeadCode CSB 28/03/99		}
//DeadCode CSB 28/03/99	}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ClimbforHome
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleClimbforHome ( )

{
	SLong	temp_range;

	temp_range = Range; 	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
			InterceptandRange (&ai.homebase->World);
			ai.desiredhdg = HdgIntercept; 
			SetManoeuvreTime(0);
		}
		case PHASE1:
		{
			if(!TimeLeft())
			{
				Range = temp_range; 	
				if (Range < DANGERRANGE)
				{
					ai.manoeuvre = MANOEUVRE_SELECT;
				}
				PitchIntercept =  ANGLES_10Deg;
				Bool level = SimpleMoveToDesiredHdg (ai.desiredhdg, ANGLES_1Deg);
				if((AutoCalcPitch()) && (level))
					SetManoeuvreTime (1000);
				Range = temp_range; 	
				if (Range > ENGAGERANGE)
				{
					ai.manoeuvre = MANOEUVRE_SELECT;
				}
			}
		}
	}
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		StraightDive
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleStraightDive ( )

{
	SWord 	deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SetManoeuvreTime (2000);
 			SimpleMoveToThrust (90);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			SimpleMoveToRoll (ANGLES_0Deg);
			PitchIntercept =  ANGLES_340Deg;
			AutoCalcPitch();
			if (World.Y < FT_1000)
				ai.manoeuvre = MANOEUVRE_SELECT;
			if  (!ManoeuvreTimeLeft())
				ai.manoeuvre = MANOEUVRE_SELECT;

			break;
		}
	}

}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		StandOnTail
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleStandOnTail ( )

{
	COORDS3D	target;
	SLong		time;
	AirStrucPtr	unfriendly = *ai.unfriendly;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (800);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			target.X = ai.unfriendly->World.X;
			target.Y = ai.unfriendly->World.Y;
			target.Z = ai.unfriendly->World.Z;
			InterceptandRange (&target);

			//time for bullets to arrive at current unfriendly position
			time = Range / MUZZELVEL;
			//calc lead position using this time
			target.X = ai.unfriendly->World.X + (unfriendly->velx * time );
			target.Y = ai.unfriendly->World.Y + (unfriendly->vely * time );
			target.Z = ai.unfriendly->World.Z + (unfriendly->velz * time );
			InterceptandRange (&target);


			SimpleMoveToHdg (HdgIntercept);
			if (vel < MPH55)
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}else
			{
				if ((SWord) fly.cpitch < (SWord) PitchIntercept)
				{
					AutoCalcPitch();
				}else
				{
					AutoCalcPitch();
					FireABullet(10);
				}
			}
			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}

	}
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ShootToFrighten
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleShootToFrighten ( )

{
	AirStrucPtr	unfriendly = *ai.unfriendly;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (200);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			if (!ManoeuvreTimeLeft())
			{
			   	ai.ManStep = PHASE0;
				ai.manoeuvre=MANOEUVRE_SUSTAINEDTURN;
			}
			SimpleTightTurn (FT_1000);
			FireABullet(10);
			break;
		}
	}
}





//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		GoHome
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleGoHome ( )

{
	
//DeadCode RDH 20Aug96 	WayPointPtr	lastwaypoint;
	SLong	temp_range;

	temp_range = Range; 	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (1000);
			ai.ManStep = PHASE1;
			InterceptandRange (&ai.homebase->World);
			ai.desiredhdg = HdgIntercept; 
			Range = temp_range;
//jink a bit possibly
			if	(		TargetOnSubjectTail(ANGLES_10Deg)
					&&	(Range < 4 * WEAPONSRANGE)
					&&	(ai.combatskill >= SKILL_REGULAR)
				)
				ai.desiredhdg = HdgIntercept + ANGLES_20Deg; 
		}
		case PHASE1:
		{
			if (!ManoeuvreTimeLeft())
			{
				SetManoeuvreTime (500);
				ai.ManStep = PHASE2;
				InterceptandRange (&ai.homebase->World);
				ai.desiredhdg = HdgIntercept; 
				Range = temp_range;
				if	(		TargetOnSubjectTail(ANGLES_10Deg)
						&&	(Range < 4 * WEAPONSRANGE)
						&&	(ai.combatskill >= SKILL_REGULAR)
					)
					ai.desiredhdg = HdgIntercept + ANGLES_340Deg; 
			}
			if (Range < WEAPONSRANGE)
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			PitchIntercept =  ANGLES_0Deg;
			AutoCalcPitch();
			SimpleMoveToHdg (ai.desiredhdg);
			if (Range > COMBATRANGE +METRES1400)
//DeadCode RDH 25Oct96 			if (Range > COMBATRANGE + METRES1400)
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}
		case PHASE2:
		{
			if (!ManoeuvreTimeLeft())
			{
				ai.ManStep = PHASE1;
				SetManoeuvreTime (500);
				InterceptandRange (&ai.homebase->World);


				ai.desiredhdg = HdgIntercept; 
				Range = temp_range;
				if	(		TargetOnSubjectTail(ANGLES_10Deg)
						&&	(Range < 4 * WEAPONSRANGE)
						&&	(ai.combatskill >= SKILL_REGULAR)
					)
					ai.desiredhdg = HdgIntercept + ANGLES_20Deg; 
			}
			if (Range < WEAPONSRANGE)
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			PitchIntercept =  ANGLES_0Deg;
			AutoCalcPitch();
			SimpleMoveToHdg (ai.desiredhdg);
//DeadCode RDH 25Oct96 			if (Range > COMBATRANGE + METRES1400)
			if (Range > COMBATRANGE+METRES1400)
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}
	}
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveAway
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleMoveAway ( )

{
	UWord	roll_pos;
	SWord	roll_sign;
	SWord	desiredaoa;
	SWord deltahdg;
	UWord	deltahdg_pos;
	SWord	deltahdg_sign;
	UWord	elevator_pos;
	SWord	elevator_sign;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (2000);
			ai.ManStep = PHASE1;
			break;
		}
		case PHASE1:
		{
			if  		(!ManoeuvreTimeLeft())
				
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}

			PitchIntercept =  ANGLES_0Deg;
			AutoCalcPitch();
			SimpleMoveToHdg (HdgIntercept+ANGLES_180Deg);	//get aidesiredroll

			deltahdg = hdg - (HdgIntercept+ANGLES_180Deg);
			Math_Lib.Pos (((SWord)deltahdg),deltahdg_pos,deltahdg_sign);

			if (deltahdg_pos <ANGLES_5Deg)
			{
				ai.ManStep = PHASE2;
				SetManoeuvreTime (1500);
			}
			break;
		}

		case PHASE2:
		{
			SimpleMoveToHdg (HdgIntercept+ANGLES_180Deg);	//get aidesiredroll
			PitchIntercept =  ANGLES_4Deg;
			AutoCalcPitch();
			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}
	}

}	

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		AccidentalSpinOut
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	similar to SpinOut, but longer recovery
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleAccidentalSpinOut ( )

{
	SimpleSpinOut ( );
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		TurningFight
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	attempt to lead, find Hdgintercept and delta, set roll
//				roll should not give alt lose
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
//void AirStruc::SimpleTurningFight ()
//
//{
//	SWord	deltapitch;
//	COORDS3D	target;
//	SLong		time;
//	AirStrucPtr	unfriendly = *ai.unfriendly;
//	UWord	acroll_pos,aoa_pos;
//	SWord	acroll_sign,reqroll,aoa_sign;
//	
//	switch (ai.ManStep)
//	{
//		case PHASE0:
//		{
//			SimpleMoveToThrust (100);
//			SetManoeuvreTime (1500);
//			ai.ManStep = PHASE1;
//			break;
//		}
//		case PHASE1:
//		{
//			if (!ManoeuvreTimeLeft())
//			{
//				ai.manoeuvre = MANOEUVRE_SELECT;
//			}
//			if (	(TargetFacingSubject(ANGLES_10Deg))
//				&&	SubjectFacingTarget(ANGLES_10Deg)
//				&&	(Range < METRES150)
//				)
//				{
//					HdgIntercept = HdgIntercept + ANGLES_90Deg;
//					PitchIntercept = PitchIntercept + ANGLES_10Deg;
//				}
//			SimpleTightTurn (FT_1000);
//			break;
//
//		}
//	}
//
//}		
	
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LowGYoYo
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleLowGYoYo ( )

{
	UWord	dhdg_pos;
	SWord	dhdg_sign;
	SWord dhdg;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SetManoeuvreTime (1000);
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			dhdg = (SWord) (hdg - HdgIntercept);
			Math_Lib.Pos (dhdg,dhdg_pos,dhdg_sign);
			if (dhdg_pos < ANGLES_45Deg)
			{
				dhdg_pos = 5*dhdg_pos/4;  
			}
			dhdg = Math_Lib.UseSign (dhdg_pos,dhdg_sign);

		 	ai.desiredroll = (Angles) FindRequiredRoll (dhdg,CombatReqBankData);
			SimpleMoveToRoll (ai.desiredroll);
			if (	(SimpleMoveToDesiredPitch (ANGLES_340Deg,ANGLES_15Deg))
				||	(!ManoeuvreTimeLeft())
				)
			{
				ai.ManStep = PHASE2;
				SetManoeuvreTime (500);

			}
			break;
		}
		case PHASE2:
		{
			dhdg = (SWord) (hdg - HdgIntercept);
			Math_Lib.Pos (dhdg,dhdg_pos,dhdg_sign);
			if (dhdg_pos < ANGLES_45Deg)
			{
				dhdg_pos = 5*dhdg_pos/4;  
			}
			dhdg = Math_Lib.UseSign (dhdg_pos,dhdg_sign);

		 	ai.desiredroll = (Angles) FindRequiredRoll (dhdg,CombatReqBankData);
			SimpleMoveToRoll (ai.desiredroll);
			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		VerticalLoop
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleVerticalLoop ( )

{

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SetManoeuvreTime (1000);
 			SimpleMoveToThrust (100);
			ai.desiredroll = - (ANGLES)ANGLES_45Deg +
 						(Angles)(ANGLES_90Deg * Math_Lib.rnd(2)); 
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
//DeadCode RDH 13Dec96 			SimpleTightClimbingTurn (ANGLES_15Deg);
			SimpleTightClimbingTurn (ANGLES_7Deg);				//RDH 13Dec96
			if  (!ManoeuvreTimeLeft())
			{
 				ai.manoeuvre = MANOEUVRE_SELECT;

			}
			break;
		}
	}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ImmelmannTurn
//Author		R. Hyde 
//Date			Wed 13 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleImmelmannTurn ( )

{
	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
//DeadCode RDH 13Dec96 			SimpleTightClimbingTurn (ANGLES_15Deg);
			SimpleTightClimbingTurn (ANGLES_7Deg);
			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
		}
	}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		HeadOnOffset
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleHeadOnOffset ( )

{
	AirStrucPtr	unfriendly = *ai.unfriendly;

	SLong temp_range;
	temp_range = Range; 	
	COORDS3D	target;
	SWord c,s;
	SWord newhdg;
	UByte	FF = FlyingFactor();

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			InterceptandRange(ai.unfriendly);
			SimpleMoveToThrust (ThrustfromFF());
			SetManoeuvreTime (2000);
			SWord dhdg = HdgIntercept - hdg;
			if ((dhdg) > 0)
				ai.desiredhdg = ANGLES_90Deg;
			else
				ai.desiredhdg = ANGLES_270Deg;
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			InterceptandRange(ai.unfriendly);
			ANGLES bca = BestClimbAngle();
			if (PitchIntercept > bca) PitchIntercept = bca;
			AutoCalcPitch();

			ANGLES desiredhdg = ai.desiredhdg;
			ANGLES unfHdgIntercept = HdgIntercept;
			SLong	unfRange = Range;
			newhdg = unfriendly->hdg + ai.desiredhdg;

			Math_Lib.high_sin_cos((Angles)newhdg,s,c);
			target.X = unfriendly->World.X + ((BREAKTURNRADIUS * s) >> ANGLES_SHIFT);
			target.Y = unfriendly->World.Y;
			target.Z = unfriendly->World.Z + ((BREAKTURNRADIUS * c) >> ANGLES_SHIFT);
			InterceptandRange (&target);
			desiredhdg = HdgIntercept;

			SWord	reqdeltahdg = hdg - desiredhdg;
		 	ANGLES reqroll = (Angles)FindRequiredRoll (reqdeltahdg,CombatReqBankData);

			int decisionpoint = Math_Lib.rnd(2);

			if  (		(Range < (2 * BREAKTURNRADIUS ))
					&&	(FF < 200)
				)
			{
				if (decisionpoint)
				{
					if (FF < 90)
					{
						ai.manoeuvre = MANOEUVRE_PANICTURN;
						ai.ManStep = PHASE0;
					}else  
					{
						ai.manoeuvre = MANOEUVRE_BREAKTURN;
						ai.ManStep = PHASE0;
					}
				}else
				{
					ai.ManStep = PHASE2;
				}

			}else if	(		(Range < BREAKTURNRADIUS)
							
						)
			{
				if (decisionpoint)
				{
					ai.manoeuvre = MANOEUVRE_BREAKTURN;
					ai.ManStep = PHASE0;
				}else
				{
					ai.desiredhdg = ai.desiredhdg  + ANGLES_180Deg;
					ai.ManStep = PHASE2;
				}
			}

			if  (!ManoeuvreTimeLeft())
				ai.manoeuvre = MANOEUVRE_SELECT;
			break;
		}
		case PHASE2:
		{
			InterceptandRange(ai.unfriendly);
			ANGLES bca = BestClimbAngle();
			if (PitchIntercept > bca ) PitchIntercept = bca;
			AutoCalcPitch();

			ANGLES desiredhdg = ai.desiredhdg;
			ANGLES unfHdgIntercept = HdgIntercept;
			SLong	unfRange = Range;
			newhdg = unfriendly->hdg + ai.desiredhdg;

			Math_Lib.high_sin_cos((Angles)newhdg,s,c);
			target.X = unfriendly->World.X + ((METRES30 * s) >> ANGLES_SHIFT);
			target.Y = unfriendly->World.Y;
			target.Z = unfriendly->World.Z + ((METRES30 * c) >> ANGLES_SHIFT);
			InterceptandRange (&target);
			desiredhdg = HdgIntercept;

			SWord	reqdeltahdg = hdg - ai.desiredhdg;
		 	SWord reqroll = FindRequiredRoll (reqdeltahdg,CombatReqBankData);
			SimpleMoveToRoll((Angles)reqroll);

			if  (!ManoeuvreTimeLeft())
				ai.manoeuvre = MANOEUVRE_SELECT;
			if	(SubjectOnTargetTail(ANGLES_80Deg))
			{
				if	(		(ai.unfriendly == Manual_Pilot.ControlledAC2)
						&&	(((AirStruc*)ai.unfriendly)->Follower())
					)
					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_HEADON, MSG_HEADON, ((AirStruc*)ai.unfriendly)->Follower(), this, ((AirStruc*)ai.unfriendly)));

				int startpoint = Math_Lib.rnd(2);
				ai.ManStep = PHASE0;
				if (FF < 90)
				{
					if (startpoint)
						ai.manoeuvre = MANOEUVRE_JINK;
					else
						ai.manoeuvre = MANOEUVRE_MILDSCISSORS;
				}else if (FF < 200)
				{
					if (startpoint)
							ai.manoeuvre = MANOEUVRE_STRAIGHTANDLEVEL;
					else
							ai.manoeuvre = MANOEUVRE_EXTENSION;
				}else 
				{
					if (startpoint)
							ai.manoeuvre = MANOEUVRE_TURNINGFIGHT;
					else
							ai.manoeuvre = MANOEUVRE_BREAKTURN;
				}
			}
		}
	}
	
//DeadCode CSB 05/04/99		UWord	roll_pos,turn_pos;
//DeadCode CSB 05/04/99		SWord	turn_sign,roll_sign;
//DeadCode CSB 05/04/99		
//DeadCode CSB 05/04/99		switch (ai.ManStep)
//DeadCode CSB 05/04/99		{
//DeadCode CSB 05/04/99			case PHASE0:
//DeadCode CSB 05/04/99			{
//DeadCode CSB 05/04/99				SimpleMoveToThrust (100);
//DeadCode CSB 05/04/99				ai.ManStep = PHASE1;
//DeadCode CSB 05/04/99				break;
//DeadCode CSB 05/04/99			}
//DeadCode CSB 05/04/99			case PHASE1:
//DeadCode CSB 05/04/99			{
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				if  (	(!SubjectFacingTarget(ANGLES_15Deg))
//DeadCode CSB 05/04/99					 || (!TargetFacingSubject(ANGLES_15Deg))
//DeadCode CSB 05/04/99					)
//DeadCode CSB 05/04/99				{
//DeadCode CSB 05/04/99					ai.manoeuvre = MANOEUVRE_SELECT;
//DeadCode CSB 05/04/99				}
//DeadCode CSB 05/04/99				AutoCalcPitch();
//DeadCode CSB 05/04/99				SimpleMoveToHdg (HdgIntercept);
//DeadCode CSB 05/04/99				if  (	(Range < METRES150)
//DeadCode CSB 05/04/99					&&	(ai.combatskill < SKILL_VETERAN)
//DeadCode CSB 05/04/99					&&	(ai.morale < MORALE_MEDIUM)
//DeadCode CSB 05/04/99					)
//DeadCode CSB 05/04/99				{
//DeadCode CSB 05/04/99					ai.desiredhdg = hdg + ANGLES_120Deg;
//DeadCode CSB 05/04/99					ai.ManStep = PHASE2;
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				}
//DeadCode CSB 05/04/99				if (Range < METRES80)
//DeadCode CSB 05/04/99				{
//DeadCode CSB 05/04/99					ai.desiredhdg = hdg + ANGLES_120Deg;
//DeadCode CSB 05/04/99					ai.ManStep = PHASE2;
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				}
//DeadCode CSB 05/04/99				break;
//DeadCode CSB 05/04/99			}
//DeadCode CSB 05/04/99			case PHASE2:
//DeadCode CSB 05/04/99			{
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				AutoCalcPitch();
//DeadCode CSB 05/04/99				SimpleMoveToHdg (ai.desiredhdg);
//DeadCode CSB 05/04/99				if  (!SubjectFacingTarget(ANGLES_85Deg))
//DeadCode CSB 05/04/99				{
//DeadCode CSB 05/04/99					ai.ManStep = PHASE3;
//DeadCode CSB 05/04/99					SetManoeuvreTime (1000);
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				}
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				break;
//DeadCode CSB 05/04/99			}
//DeadCode CSB 05/04/99			case PHASE3:
//DeadCode CSB 05/04/99			{
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				SimpleTightTurn (FT_500);
//DeadCode CSB 05/04/99				if 	(!ManoeuvreTimeLeft())
//DeadCode CSB 05/04/99				{
//DeadCode CSB 05/04/99					ai.manoeuvre = MANOEUVRE_SELECT;
//DeadCode CSB 05/04/99				}
//DeadCode CSB 05/04/99	
//DeadCode CSB 05/04/99				break;
//DeadCode CSB 05/04/99			}
//DeadCode CSB 05/04/99		}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		StayWithPrey
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	As target looses alt, stay until explosion or hits ground or alt <???
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleStayWithPrey ( )

{
	
	SWord	acroll_sign,reqroll;
	SWord	deltapitch;
	UWord	acroll_pos;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			SetManoeuvreTime (1500);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			if  (		(!ManoeuvreTimeLeft())
					||	(World.Y < FT_700)
				)
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			SimpleTightTurn (FT_500);


		}
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		GainHeight
//Author		R. Hyde 
//Date			Tue 12 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleGainHeight ( )

{
	
	AirStrucPtr		unfriendly;	
	SWord 	deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign,desiredaoa;
	
	switch (ai.ManStep)
	{
		case PHASE0:
		{
			unfriendly = *ai.unfriendly;
			if (	(unfriendly->ai.attacker == this)
				||	(World.Y > classtype->fightceiling)
				)
			{
				ai.manoeuvre=MANOEUVRE_SUSTAINEDTURN;
				SimpleSustainedTurn ();
				break;
			}
			SetManoeuvreTime (2000);
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
//##			if ((SWord) fly.turnrate > 0)
//			{
//		 		ai.desiredroll = ANGLES_20Deg;
//			}else
//			{
//		 		ai.desiredroll = ANGLES_340Deg;
//			}
		}
		case PHASE1:
		{
			PitchIntercept = ANGLES_10Deg;
			AutoCalcPitch();
			if(roll > 0)
				SimpleMoveToRoll(ANGLES_30Deg);
			else
				SimpleMoveToRoll(ANGLES_330Deg);

			if (!ManoeuvreTimeLeft())
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;

		}

	}

}



//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SplitS
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleSplitS ()

{
	SWord 	deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			int groundlevel = Land_Scape.GetGroundLevel(this);
			if(World.Y - groundlevel < 150000)	//5000 ft
			{
				if(Math_Lib.rnd(1))
					ai.manoeuvre = MANOEUVRE_SCISSORS;
				else
 					ai.manoeuvre = MANOEUVRE_BREAKTURN;
				SetManoeuvreTime(0);
			}
			else
			{
				ai.desiredroll = ANGLES_180Deg;
 				SimpleMoveToThrust (100);
				ai.ManStep = PHASE1;
				if((this != Persons2::PlayerSeenAC) && (this != Persons2::PlayerGhostAC))
					fly.elevator = 0;
			}
		}
		case PHASE1:
		{
			deltaroll = (SWord) (ai.desiredroll - roll);
			SimpleMoveToRoll (ai.desiredroll);
			Math_Lib.Pos (deltaroll,deltaroll_pos,deltaroll_sign);
			if (deltaroll_pos <ANGLES_10Deg)
			{
				ai.ManStep = PHASE2;
	 			SimpleMoveToThrust (0);
//rdh				ai.lastdeltapitch = ANGLES_0Deg;
				SetManoeuvreTime (1500);

			}
			break;
		}

		case PHASE2:
		{

			PitchIntercept = ANGLES_280Deg;
			AutoCalcPitch();
			if ( 		(!ManoeuvreTimeLeft())
					|| (    DesiredPitchChk (ANGLES_270Deg,ANGLES_20Deg)
						)										//RDH 24Jul96
				)
			{
	 			SimpleMoveToThrust (100);
				ai.manoeuvre = MANOEUVRE_SELECT;
			}


			break;
		}


	}

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ZoomandDrop
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleZoomandDrop ()

{
	SWord phase,deltaroll;
	UWord	deltaroll_pos;
	SWord	deltaroll_sign;

	phase = (SWord) ai.ManStep;
	switch (phase)
	{
		case PHASE0:
		{
			ai.desiredroll = ANGLES_0Deg;
 			SimpleMoveToThrust (0);
			SetManoeuvreTime (500);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			SimpleMoveToRoll (ai.desiredroll);
			Math_Lib.Pos (deltaroll,deltaroll_pos,deltaroll_sign);
			if (deltaroll_pos <ANGLES_20Deg)
			{
				SetManoeuvreTime (500);
				ai.ManStep = PHASE2;
			}
			break;
		}
		case PHASE2:
		{
			PitchIntercept = ANGLES_70Deg;
			AutoCalcPitch();
			if (	(SimpleMoveToDesiredPitch (ANGLES_70Deg,ANGLES_10Deg))
				||	(!ManoeuvreTimeLeft())
				)
			{
				ai.ManStep = PHASE3;
 				SimpleMoveToThrust (100);
			}
		break;
		}

		case PHASE3:
		{
			if (SimpleMoveToDesiredPitch (ANGLES_315Deg,ANGLES_10Deg))
			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}


	}

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		DiveAndZoom
//Author		R. Hyde 
//Date			Mon 18 Dec 1995
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleDiveAndZoom ()

{
	SWord	deltapitch_pos,deltapitch_sign;
	SLong	temp_range;
	COORDS3D	target;
	SLong		t,actualrange;
	AirStrucPtr	unfriendly = *ai.unfriendly;
 	SWord		deltaroll;										//RDH 14Mar96
	SWord		easestickforward;
	ANGLES		local_PI;


	local_PI  = PitchIntercept;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
 			SimpleMoveToThrust (100);
			ai.ManStep = PHASE1;
			SetManoeuvreTime (12000);
		}
		case PHASE1:
		{
 			if  (	(		(!ManoeuvreTimeLeft())
						||	((Range - (World.Y - ai.unfriendly->World.Y)) < ENGAGERANGE)
					)
					&&
					(SubjectFacingTarget(ANGLES_45Deg))
				 )
  			{
  				ai.ManStep = PHASE2;
				SetManoeuvreTime (5000);
  			}
   			if (local_PI << ANGLES_90Deg)
  			{
  				ai.manoeuvre = MANOEUVRE_SELECT;
  			}
 

			PitchIntercept = ANGLES_5Deg;
			AutoCalcPitch();
			SimpleMoveToHdg (HdgIntercept);
			break;
		}
		case PHASE2:
		{
			SimpleTurningFight();
			if  (			(!ManoeuvreTimeLeft())
					 ||
							(TargetOnSubjectTail(ANGLES_90Deg))
					 ||	
							(Range < METRES200)
				)
			{
			 	ai.ManStep = PHASE0;
				ai.manoeuvre=MANOEUVRE_ZOOM;
			}
 	
			break;
		}

	}

}

//DeadCode RDH 06Aug98 //컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//DeadCode RDH 06Aug98 //Procedure		TightTurn
//DeadCode RDH 06Aug98 //Author		R. Hyde 
//DeadCode RDH 06Aug98 //Date			//RDH 11Oct96
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Description	
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Inputs		
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Returns	
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //------------------------------------------------------------------------------
void AirStruc::SimpleTightTurn (SLong recovery_alt)
{
}
//DeadCode RDH 06Aug98 {
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	SWord	sinturn,costurn;
//DeadCode RDH 06Aug98 	SWord	turn;
//DeadCode RDH 06Aug98 	SLong	desiredroll,deltax,deltaz,delta; 
//DeadCode RDH 06Aug98 	SWord	deltapitch;
//DeadCode RDH 06Aug98 	COORDS3D	target;
//DeadCode RDH 06Aug98 	SLong		time;
//DeadCode RDH 06Aug98 	AirStrucPtr	unfriendly = *ai.unfriendly;
//DeadCode RDH 06Aug98 	UWord	acroll_pos,aoa_pos,dhdg_pos,desiredroll_pos;
//DeadCode RDH 06Aug98 	SWord	acroll_sign,reqroll,aoa_sign,dhdg_sign,desiredroll_sign;
//DeadCode RDH 06Aug98 	SWord dhdg;
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	target.X = ai.unfriendly->World.X;
//DeadCode RDH 06Aug98 	target.Y = ai.unfriendly->World.Y;
//DeadCode RDH 06Aug98 	target.Z = ai.unfriendly->World.Z;
//DeadCode RDH 06Aug98 	InterceptandRange (&target);
//DeadCode RDH 06Aug98 	SimpleTightTurnSub (recovery_alt,target);
//DeadCode RDH 06Aug98 }
//DeadCode RDH 06Aug98 

//DeadCode RDH 06Aug98 //컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//DeadCode RDH 06Aug98 //Procedure		TightTurnSub
//DeadCode RDH 06Aug98 //Author		R. Hyde 
//DeadCode RDH 06Aug98 //Date			//RDH 11Oct96
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Description	
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Inputs		
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Returns	
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //------------------------------------------------------------------------------
//DeadCode RDH 06Aug98 void AirStruc::SimpleTightTurnSub (SLong recovery_alt,COORDS3D	target)
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 {
//DeadCode RDH 06Aug98 	SLong		time;
//DeadCode RDH 06Aug98 	SWord	sinturn,costurn;
//DeadCode RDH 06Aug98 	SWord	turn;
//DeadCode RDH 06Aug98 	SLong	desiredroll,deltax,deltaz,delta; 
//DeadCode RDH 06Aug98 	AirStrucPtr	unfriendly = *ai.unfriendly;
//DeadCode RDH 06Aug98 	SWord	deltapitch,maxroll,minroll;
//DeadCode RDH 06Aug98 	UWord	acroll_pos,aoa_pos,dhdg_pos,desiredroll_pos,targetroll,olddesiredroll_pos;
//DeadCode RDH 06Aug98 	SWord	acroll_sign,reqroll,aoa_sign,dhdg_sign,desiredroll_sign;
//DeadCode RDH 06Aug98 	SWord dhdg;
//DeadCode RDH 06Aug98 	dhdg = (SWord) (hdg - HdgIntercept);
//DeadCode RDH 06Aug98 	Math_Lib.Pos (dhdg,dhdg_pos,dhdg_sign);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 		if (	(TargetFacingSubject(ANGLES_70Deg))
//DeadCode RDH 06Aug98 			&& 	(((SWord) PitchIntercept) > 0)
//DeadCode RDH 06Aug98 				)
//DeadCode RDH 06Aug98 		{
//DeadCode RDH 06Aug98 			PitchIntercept = ANGLES_0Deg;
//DeadCode RDH 06Aug98 		}	
//DeadCode RDH 06Aug98 		deltapitch = PitchIntercept - fly.cpitch;
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	if	(	(SubjectOnTargetTail(ANGLES_50Deg))
//DeadCode RDH 06Aug98 		&&	(SubjectFacingTarget(ANGLES_70Deg))
//DeadCode RDH 06Aug98 		&&	(Range < (WEAPONSRANGE-METRES20))
//DeadCode RDH 06Aug98 		)
//DeadCode RDH 06Aug98 	{
//DeadCode RDH 06Aug98 		time = dhdg_pos /400;
//DeadCode RDH 06Aug98 		if (time > 20)
//DeadCode RDH 06Aug98 			time =20;
//DeadCode RDH 06Aug98 //##		turn = time * unfriendly->fly.turnrate/10;	//in s
//DeadCode RDH 06Aug98 //		turn = unfriendly->hdg + turn;
//DeadCode RDH 06Aug98 		delta = unfriendly->vel * time;		//cm/s
//DeadCode RDH 06Aug98 		Math_Lib.high_sin_cos((Angles)turn,sinturn,costurn);
//DeadCode RDH 06Aug98 		deltax = MULSHSIN(sinturn,delta,ANGLES_SHIFT);
//DeadCode RDH 06Aug98 		deltaz = MULSHSIN(costurn,delta,ANGLES_SHIFT);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 		//calc lead position using this time
//DeadCode RDH 06Aug98 		target.X = ai.unfriendly->World.X + deltax;
//DeadCode RDH 06Aug98 		target.Y = ai.unfriendly->World.Y + (unfriendly->vely * time );
//DeadCode RDH 06Aug98 		target.Z = ai.unfriendly->World.Z + deltaz;
//DeadCode RDH 06Aug98 		InterceptandRange (&target);
//DeadCode RDH 06Aug98 		dhdg = (SWord) (hdg - HdgIntercept);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 		Math_Lib.Pos ((SWord)ai. desiredroll,olddesiredroll_pos,desiredroll_sign);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	 	ai.desiredroll = (Angles) FindRequiredRoll (dhdg,CombatReqBankData);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	}else
//DeadCode RDH 06Aug98 	{
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 		Math_Lib.Pos ((SWord)ai. desiredroll,olddesiredroll_pos,desiredroll_sign);
//DeadCode RDH 06Aug98 		dhdg = Math_Lib.UseSign (dhdg_pos,dhdg_sign);
//DeadCode RDH 06Aug98 	 	ai.desiredroll = (Angles) FindRequiredRoll (dhdg,CombatReqBankData);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	}
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	Math_Lib.Pos ((SWord)ai. desiredroll,desiredroll_pos,desiredroll_sign);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	if (	(deltapitch > ANGLES_2Deg)
//DeadCode RDH 06Aug98 		&&	(!(TargetFacingSubject(ANGLES_20Deg)))
//DeadCode RDH 06Aug98 			)
//DeadCode RDH 06Aug98 	{
//DeadCode RDH 06Aug98 		if (olddesiredroll_pos <= desiredroll_pos)
//DeadCode RDH 06Aug98 		{
//DeadCode RDH 06Aug98 			desiredroll_pos = olddesiredroll_pos - 64;
//DeadCode RDH 06Aug98 			if (desiredroll_pos > ANGLES_20Deg)
//DeadCode RDH 06Aug98 				ai.desiredroll = (Angles) Math_Lib.UseSign (desiredroll_pos,desiredroll_sign);
//DeadCode RDH 06Aug98 		}
//DeadCode RDH 06Aug98 	}
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	Math_Lib.Pos ((SWord)ai. desiredroll,desiredroll_pos,desiredroll_sign);
//DeadCode RDH 06Aug98 	if (	(deltapitch < ((SWord)ANGLES_350Deg))
//DeadCode RDH 06Aug98 		&& (desiredroll_pos >= MAXCOMBATROLL)
//DeadCode RDH 06Aug98 			)
//DeadCode RDH 06Aug98 			{
//DeadCode RDH 06Aug98 				desiredroll_pos = ANGLES_180Deg - (ANGLES)MAXCOMBATROLL;
//DeadCode RDH 06Aug98 				ai.desiredroll = (Angles) Math_Lib.UseSign (desiredroll_pos,desiredroll_sign);
//DeadCode RDH 06Aug98 			}
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 	if ( target.Y >= recovery_alt)
//DeadCode RDH 06Aug98 	{
//DeadCode RDH 06Aug98 		deltapitch = PitchIntercept - fly.cpitch;
//DeadCode RDH 06Aug98 		SimpleTightTurnSubSub(deltapitch);
//DeadCode RDH 06Aug98 	}else
//DeadCode RDH 06Aug98 	{
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 		SLong groundlevel = _Collide.LowestSafeAlt(this,
//DeadCode RDH 06Aug98 											this->pitch,
//DeadCode RDH 06Aug98 											this->roll);//RDH 21Oct96
//DeadCode RDH 06Aug98 		if ((World.Y - groundlevel) < FT_100)
//DeadCode RDH 06Aug98 		{
//DeadCode RDH 06Aug98 			//## attempt to disengage
//DeadCode RDH 06Aug98 			SimpleMoveToThrust (100);
//DeadCode RDH 06Aug98 			PitchIntercept = ANGLES_5Deg;
//DeadCode RDH 06Aug98 			AutoCalcPitch();
//DeadCode RDH 06Aug98 			SimpleMoveToRoll (ANGLES_0Deg);
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 		}
//DeadCode RDH 06Aug98 	}
//DeadCode RDH 06Aug98 }
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 
//DeadCode RDH 06Aug98 //컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//DeadCode RDH 06Aug98 //Procedure		SimpleTightTurnSubSub
//DeadCode RDH 06Aug98 //Author		R. Hyde 
//DeadCode RDH 06Aug98 //Date			//RDH 11Oct96
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Description	
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Inputs		
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //Returns	
//DeadCode RDH 06Aug98 //
//DeadCode RDH 06Aug98 //------------------------------------------------------------------------------
void AirStruc::SimpleTightTurnSubSub (SWord deltapitch)
{
	SLong	desiredroll; 
	UWord	acroll_pos,aoa_pos,dhdg_pos,desiredroll_pos;
	SWord	acroll_sign,reqroll,aoa_sign,dhdg_sign,desiredroll_sign;
		//now do pitch
		PitchIntercept = (Angles)deltapitch + fly.cpitch;
		{
			if (	(deltapitch > 0)
				&&	((SWord) fly.cpitch < ANGLES_15Deg)	//sustainable max climbrate
				)
			{
				AutoCalcPitch();
		
			}else
			{
				if (	(deltapitch <= 0)
					)
				{//can push down
					AutoCalcPitch();
				}else
				{
					PitchIntercept = ANGLES_0Deg;
					AutoCalcPitch();
				}
			}
			Math_Lib.Pos ((SWord)ai.desiredroll,desiredroll_pos,desiredroll_sign);
			
			if (	(fly.cpitch > ANGLES_5Deg)
				&& (desiredroll_pos > ANGLES_20Deg)
				&& (deltapitch > 0)
					)
			
			{
				if (fly.cpitch > ANGLES_15Deg)
				{
						desiredroll_pos = ANGLES_20Deg;
				}else
				{
					desiredroll = ANGLES_20Deg +  (desiredroll_pos - ANGLES_20Deg) *
								(ANGLES_15Deg - fly.cpitch) / ANGLES_15Deg;
					desiredroll_pos = (Angles) desiredroll;
				}
				ai.desiredroll = (Angles) Math_Lib.UseSign (desiredroll_pos,desiredroll_sign);
			}
			SimpleMoveToRoll (ai.desiredroll);
		}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SimpleCalcVel
//Author		R. Hyde 
//Date			Fri 25 Oct 1996
//
//Description	rewritten to use the accel calculated via Ps 3/9/98
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleCalcVel ()
{

   		Float	accel, fvel;
		bool positive = true;
		SWord		tvel;
		accel = CalcSimpleAccel ();
		fvel	=  accel * Timer_Code.FRAMETIME * 0.1;
		if (fvel < 0)
		{
			positive = false;
			fvel = -fvel;
		}
		tvel = fvel;
		fvel = 65536 * (fvel - tvel);
		int rnd = Math_Lib.rnd();
		if (fvel > rnd)
			tvel++;
		if (!positive)
			tvel = -tvel;
		vel = vel + tvel;



//	SWord thrust;
//	SLong reqvel;
//
//	thrust = fly.thrustpercent;
//	if (thrust > 10)
//	{
//		reqvel = classtype->maxvel*thrust*thrust/10000;
//	}else
//	{
//DeadCode RDH 27Oct96 		reqvel = classtype->bestclimbvel;
//		reqvel = MPH30;
//	}
//	if (fly.cpitch >> ANGLES_90Deg)
//	{
//		reqvel = classtype->bestclimbvel + (classtype->maxvel - classtype->bestclimbvel) *
//									(SWord) (classtype->bestclimbpitch - pitch)/(ANGLES)classtype->bestclimbpitch;
//	}
//	CalcVelSub (reqvel);
//	//if we want to go slow then have a double decell
//	if (	(reqvel == MPH30)
//		&&	(Range <METRES70)
//		)
//		CalcVelSub (reqvel);
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SimpleMoveToThrust
//Author		R. Hyde 
//Date			//RDH 11Oct96
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleMoveToThrust (SWord thrust)

{
	fly.thrustpercent =  thrust * thrust / 100;;

//FC code:
//	SLong	reqvel;
//	SLong	accel;
//	SWord	deltavel,deltavel_sign;	
//	UWord	deltavel_pos;
//	SLong	climb_vel;
//
//	if  (thrust == -100)
//	{
//		if (fly.thrustpercent ==0)
//			fly.thrustpercent =  MAXAUTOTHRUST;
//	}
//	elseif  (thrust == -1)
//	{
//		if (fly.thrustpercent ==MAXAUTOTHRUST)
//			fly.thrustpercent =  0;
//	}
//	else
//		fly.thrustpercent =  thrust;
//	if (fly.thrustpercent > MAXAUTOTHRUST)
//		fly.thrustpercent = MAXAUTOTHRUST;	

}




//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		WeldedWingMan
//Author		R. Hyde							Craig Beeston
//Date			Mon 2 Mar 1998					Tue 27 Apr 1999
//
//Description	Formation flag not set so a/c can spot
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleWeldedWingMan()
{
	
//	stick to the leaders tail, range about 1000ft and weave
//	However for low morale bring wingman closer
//	For very low morale it should appear as if he is trying to stay in formation

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			fly.thrustpercent = ThrustfromFF();
			ai.ManStep = PHASE1;
			SetManoeuvreTime (3000);

		}
		case PHASE1:
		{
			if (WeldedWingSixOK())
			{
				if (		(fly.leadflight)					
						&&	 (fly.leadflight->ai.manoeuvre == MANOEUVRE_TOPCOVER)
					)//force manoeuvre to allow disengage
						ai.manoeuvre = MANOEUVRE_SELECT;
				else if (!leader)
					ai.manoeuvre = MANOEUVRE_SELECT;
				else
				{
					Bool PullHard = FALSE;
					COORDS3D	target;
					SWord	reqdeltahdg,reqdeltahdg_sign;
					UWord	reqdeltahdg_pos;
					SWord	reqdeltapitch,reqdeltapitch_sign;
					UWord	reqdeltapitch_pos;

					target = leader->World; 									//JIM 02Aug96
					target.Y +=	60 * ai.morale;//FT_500;		Top Moral gives 500 ft

					InterceptandRange(&target);


					reqdeltahdg   =	HdgIntercept - hdg;
					reqdeltapitch =	PitchIntercept - pitch;
					Math_Lib.Pos (reqdeltahdg,reqdeltahdg_pos,reqdeltahdg_sign);
					Math_Lib.Pos (reqdeltapitch,reqdeltapitch_pos,reqdeltapitch_sign);
					
					SWord DesRoll	= 0;
					SWord IdealRoll = 0;
					if(reqdeltapitch != 0)
						IdealRoll = Math_Lib.arctan(reqdeltahdg, reqdeltapitch);
					else
					{
						if(reqdeltahdg > 0) IdealRoll += ANGLES_90Deg;
						if(reqdeltahdg < 0) IdealRoll -= ANGLES_90Deg;
					}

					if((reqdeltahdg_pos > ANGLES_10Deg) || (reqdeltapitch_pos > ANGLES_10Deg))
					{
						DesRoll = IdealRoll;
						PullHard = TRUE;
					}
					else
					{
						if((leader->roll > MAXCOMBATROLL) || (-leader->roll > MAXCOMBATROLL))
						{
							DesRoll = IdealRoll;
							PullHard = TRUE;
						}
						else
						{
							DesRoll = leader->roll;
							DesRoll += 4 * reqdeltahdg;
							MODLIMIT(DesRoll, ANGLES_90Deg);
							PullHard = FALSE;
						}
					}
					SimpleMoveToRoll(DesRoll);

					if(PullHard)
					{
						SWord RollError = (SWord)roll - IdealRoll;
						if((RollError < ANGLES_90Deg) && (-RollError < ANGLES_90Deg))
						{
							SWord sin_roll, cos_roll;
							Math_Lib.high_sin_cos(roll, sin_roll, cos_roll);
	//DeadCode CSB 01/06/99							FP MaxDeltaPitch = (FP)classtype->maxpitchrate * (FP)Timer_Code.FRAMETIME / 100.0 / 32768.0;
							FP MaxDeltaPitch = (FP)CalcMaxPitchRate() * (FP)Timer_Code.FRAMETIME / (100.0 * 32768.0);
							FP MaxDeltaYaw   = MaxDeltaPitch * sin_roll;
							MaxDeltaPitch	 = MaxDeltaPitch * cos_roll;

							SWord	maxdeltapitch_sign, maxdeltayaw_sign;
							UWord	maxdeltapitch_pos,  maxdeltayaw_pos;
							Math_Lib.Pos((SWord)MaxDeltaPitch, maxdeltapitch_pos, maxdeltapitch_sign);
							Math_Lib.Pos((SWord)MaxDeltaYaw  , maxdeltayaw_pos,   maxdeltayaw_sign);
							FP factor1 = 1, factor2 = 1;
							if(maxdeltapitch_pos > reqdeltapitch_pos) factor1 = (FP)reqdeltapitch_pos / (FP)maxdeltapitch_pos;
							if(maxdeltayaw_pos   > reqdeltahdg_pos)   factor2 = (FP)reqdeltahdg_pos / (FP)maxdeltayaw_pos;
							if(factor1 > factor2) factor1 = factor2;
							pitch += SWord((FP)MaxDeltaPitch * (FP)factor1);
							hdg   += SWord((FP)MaxDeltaYaw   * (FP)factor1);
						}
						fly.cpitch = pitch;

						hdg -= (Angles) CalcHdg ();	//This means that all the heading change is done inside this routine
					}
					else
						AutoCalcPitch();

					UWord DesRange = 256 * ai.morale;	// up to 650 metres
					if(DesRange < WEAPONSRANGE) DesRange = WEAPONSRANGE;

					FP DeltaVel = (Range - DesRange) * 0.025;
					MODLIMIT(DeltaVel, 200);
					UWord DesVel = leader->vel + DeltaVel;
					if(vel > DesVel)
					{
						fly.thrustpercent = 0;
						if(vel > leader->vel + 200) vel = leader->vel + 200;
					}
					else
					{
						fly.thrustpercent = 200;
						if(vel < leader->vel - 200) vel = leader->vel - 200;
					}
					
	//#define PRINT_WWM_DATA
	#ifdef PRINT_WWM_DATA
	PrintVar(0, 1, "D_Range %.1f ", (FP)(DesRange / 100.0));
	PrintVar(0, 2, "Range   %.1f ", (FP)(Range / 100.0));
	PrintVar(0, 3, "Pitch   %.1f ", (FP)((SWord)pitch / 182.04));
	PrintVar(0, 4, "Pitch_I %.1f ", (FP)((SWord)PitchIntercept / 182.04));
	PrintVar(0, 5, "Headg   %.1f ", (FP)((SWord)hdg / 182.04));
	PrintVar(0, 6, "Headg_I %.1f ", (FP)((SWord)HdgIntercept / 182.04));
	PrintVar(0, 8, "D_Pitch %.1f ", (FP)(reqdeltapitch / 182.04));
	PrintVar(0, 9, "D_Headg %.1f ", (FP)(reqdeltahdg / 182.04));
	PrintVar(0,10, "DesRoll %.1f ", (FP)(DesRoll / 182.04));

	if(PullHard) PrintString(0, 12, "PULL HARD");
	else		 PrintString(0, 12, "         ");

	PrintVar(0,14, "L_Vel   %.1f ", (FP)(leader->vel / 10));
	PrintVar(0,15, "DesVel  %.1f ", (FP)(DesVel / 10));
	PrintVar(0,16, "Vel     %.1f ", (FP)(vel / 10));

	#endif
				}
			}
		}
	}
	
}				
/*	switch (ai.ManStep)
	{
		case PHASE0:
		{
			fly.thrustpercent = ThrustfromFF();
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			if (!leader)
				ai.manoeuvre = MANOEUVRE_SELECT;
			else
			{
				COORDS3D target;
				target = leader->World; 									//JIM 02Aug96
				target.Y +=	FT_500;
				InterceptandRange(&target);

				SWord reqdeltahdg, reqdeltahdg_sign;
				UWord reqdeltahdg_pos;
				reqdeltahdg = HdgIntercept - hdg;
				Math_Lib.Pos(reqdeltahdg, reqdeltahdg_pos, reqdeltahdg_sign);

				SWord reqdeltapitch, reqdeltapitch_sign;
				UWord reqdeltapitch_pos;
				reqdeltapitch = (SWord)PitchIntercept - (SWord)pitch;
				Math_Lib.Pos (reqdeltapitch, reqdeltapitch_pos, reqdeltapitch_sign);

				
				hdg -= (Angles) CalcHdg ();

				if((reqdeltahdg_pos < ANGLES_1Deg) && (reqdeltapitch_pos < ANGLES_1Deg))
					SimpleMoveToRoll(ANGLES_0Deg);
				else
				{
					if(reqdeltahdg_pos < ANGLES_180Deg)
					{
						SWord DesRoll = 0;
						if(reqdeltapitch != 0)
							DesRoll = Math_Lib.arctan(reqdeltahdg, reqdeltapitch);
						else
						{
							if(reqdeltahdg > 0) DesRoll += ANGLES_90Deg;
							if(reqdeltahdg < 0) DesRoll -= ANGLES_90Deg;
						}
						SimpleMoveToRoll(DesRoll);

#define PRINT_WWM_DATA
#ifdef PRINT_WWM_DATA
PrintVar(0, 2, "Range   %.0f ", (FP)(Range / 100.0));
PrintVar(0, 3, "Pitch   %.0f ", (FP)((SWord)pitch / 182.04));
PrintVar(0, 4, "Pitch_I %.0f ", (FP)((SWord)PitchIntercept / 182.04));
PrintVar(0, 5, "Headg   %.0f ", (FP)((SWord)hdg / 182.04));
PrintVar(0, 6, "Headg_I %.0f ", (FP)((SWord)HdgIntercept / 182.04));
PrintVar(0, 8, "D_Pitch %.0f ", (FP)(reqdeltapitch / 182.04));
PrintVar(0, 9, "D_Headg %.0f ", (FP)(reqdeltahdg / 182.04));
PrintVar(0,10, "DesRoll %.0f ", (FP)(DesRoll / 182.04));
#endif
						SWord rollerror = DesRoll - (SWord)roll;
						if((rollerror < ANGLES_45Deg) && (-rollerror < ANGLES_45Deg))
						{
							SWord sin_roll, cos_roll;
							Math_Lib.high_sin_cos(roll, sin_roll, cos_roll);
							FP MaxDeltaPitch = (FP)classtype->maxpitchrate * (FP)Timer_Code.FRAMETIME / 100.0 / 32768.0;
							FP MaxDeltaYaw   = MaxDeltaPitch * sin_roll;
							MaxDeltaPitch	 = MaxDeltaPitch * cos_roll;

	//DeadCode CSB 28/04/99							SWord	maxdeltapitch_sign, maxdeltayaw_sign;
	//DeadCode CSB 28/04/99							UWord	maxdeltapitch_pos,  maxdeltayaw_pos;
	//DeadCode CSB 28/04/99							Math_Lib.Pos((SWord)MaxDeltaPitch, maxdeltapitch_pos, maxdeltapitch_sign);
	//DeadCode CSB 28/04/99							Math_Lib.Pos((SWord)MaxDeltaYaw  , maxdeltayaw_pos,   maxdeltayaw_sign);
							FP factor1 = 1, factor2 = 1;
	//DeadCode CSB 28/04/99							if(maxdeltapitch_pos > reqdeltapitch_pos) factor1 = reqdeltapitch_pos / maxdeltapitch_pos;
	//DeadCode CSB 28/04/99							if(maxdeltayaw_pos   > reqdeltahdg_pos)   factor2 = reqdeltahdg_pos / maxdeltayaw_pos;
	//DeadCode CSB 28/04/99							if(factor1 > factor2) factor1 = factor2;
							pitch += SWord((FP)MaxDeltaPitch * (FP)factor1);
							hdg   += SWord((FP)MaxDeltaYaw   * (FP)factor1);
							fly.cpitch = pitch;
						}
					}
					else 
					{
						AutoCalcPitch();				
						SimpleMoveToHdg(HdgIntercept);
					}
				}				
//DeadCode CSB 28/04/99					InterceptandRange (&leader->World);
				vel = leader->vel;
				if (Range < DANGERRANGE)
					vel -= 100;//CalcVelAlt(0);
				else
					vel += 100;//CalcVelAlt(32000);
			}

		}
	}


}
*/
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		BailOut				
//Author		R. Hyde 			CSB
//Date			Tue 3 Mar 1998		Sun 28 Mar 1999
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleBailOut()
{
//pilot ejects or bails out depending on ac type
//ac in slow turning dive

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust (0);	 
			SimpleMoveToRoll (0);
			SimpleMoveToDesiredPitch (ANGLES_0Deg, ANGLES_0Deg);
			SetManoeuvreTime(100);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			SimpleMoveToThrust (0);	 
			SimpleMoveToRoll (0);
			if(!TimeLeft())
			{
				Trans_Obj.LaunchCanopyEject(this,*mobileitem::currworld);
				AirStrucPtr	unfriendly = *ai.unfriendly;
				unfriendly->ai.unfriendly = NULL;
				ai.ManStep = PHASE2;
				SetManoeuvreTime(100);
			}
			break;
		}
		case PHASE2:
		{
			SimpleMoveToRoll (0);
			SimpleMoveToDesiredPitch (ANGLES_0Deg, ANGLES_0Deg);
			if(!TimeLeft())
			{
				Trans_Obj.LaunchParachute(this,*mobileitem::currworld);
				movecode = AUTO_DEATHSEQUENCE;
			}
		}
	}

//DeadCode CSB 30/03/99		switch (ai.ManStep)
//DeadCode CSB 30/03/99		{
//DeadCode CSB 30/03/99			case PHASE0:
//DeadCode CSB 30/03/99			{
//DeadCode CSB 30/03/99				SimpleMoveToThrust (0);
//DeadCode CSB 30/03/99				SetManoeuvreTime (300);
//DeadCode CSB 30/03/99				ai.ManStep = PHASE1;
//DeadCode CSB 30/03/99			}
//DeadCode CSB 30/03/99			case PHASE1:
//DeadCode CSB 30/03/99			{
//DeadCode CSB 30/03/99				SimpleMoveToRoll (0);
//DeadCode CSB 30/03/99				SimpleMoveToDesiredPitch (ANGLES_0Deg, ANGLES_0Deg);
//DeadCode CSB 30/03/99				if  (!ManoeuvreTimeLeft())
//DeadCode CSB 30/03/99				{
//DeadCode CSB 30/03/99					Trans_Obj.LaunchParachute(this,*mobileitem::currworld);
//DeadCode CSB 30/03/99					ai.ManStep = PHASE2;
//DeadCode CSB 30/03/99					ai.desiredroll = - (ANGLES)ANGLES_45Deg +
//DeadCode CSB 30/03/99	 								(Angles)(ANGLES_90Deg * Math_Lib.rnd(2)); 
//DeadCode CSB 30/03/99					AirStrucPtr	unfriendly = *ai.unfriendly;
//DeadCode CSB 30/03/99					unfriendly->ai.unfriendly = NULL;
//DeadCode CSB 30/03/99					movecode = AUTO_DEATHSEQUENCE;
//DeadCode CSB 30/03/99				}
//DeadCode CSB 30/03/99			}
//DeadCode CSB 30/03/99		}
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LineAbreast
//Author		R. Hyde 
//Date			Tue 17 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLineAbreast()
{
	//only followers go line abreast. We don't link flights

	if (!leader) 
	{//leader, do same as roundabout, get height, dive and then zoom
	 //at end of zoom, choose manoeuvre based on firsttactic
			ai.ManStep = PHASE0;
			ai.manoeuvre = MANOEUVRE_DIVEANDZOOM;			
	}else
	{//follower
		if (		(Range < WEAPONSRANGE)
				&&	(information)
			)
		{
			FireABullet(30);
		}
		{
			switch (ai.ManStep)
			{
				case PHASE0:
				{
		 			SimpleMoveToThrust (100);
					ai.ManStep = PHASE1;
				}
				case PHASE1:
				{

					
					if (!leader)
						ai.manoeuvre = MANOEUVRE_SELECT;					//if no leader break manoeuvre
					else
					{
						AutoFollowWpWing();

					}

				}
			}
		}
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Scatter
//Author		R. Hyde 
//Date			Mon 30 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleScatter()
{
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Pincer
//Author		R. Hyde 
//Date			Thu 5 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimplePincer()
{
  	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust (ThrustfromFF());
			SetManoeuvreTime (2000);
			ai.ManStep = PHASE1;

		}
		case PHASE1:
		{
			AirStrucPtr	unfriendly = *ai.unfriendly;
			SWord	sinbearing,cosbearing;
			SLong	range = METRES1200;
			ANGLES brg;

			brg = (Angles)((SWord)ai.desiredhdg + (SWord)unfriendly->hdg);
			Math_Lib.high_sin_cos(brg,sinbearing,cosbearing);

			despos = unfriendly->World; 			
			despos.X += MULDIVSIN (sinbearing,range,ANGLES_FRACT);
			despos.Z += MULDIVSIN (cosbearing,range,ANGLES_FRACT);
			despos.Y += FT_500;
			InterceptandRange (&despos);

			PitchIntercept = PitchIntercept;
			AutoCalcPitch();
			SimpleMoveToHdg (HdgIntercept);

			if	(	(		(!ManoeuvreTimeLeft())
						&&	(Range < COMBATRANGE)
					)
					||
					(!SubjectFacingTarget(ANGLES_80Deg))
				 )

			{
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
			break;
		}
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MultiWave
//Author		R. Hyde 
//Date			Wed 18 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleMultiWave()
{
	ACMMODEL& ACM = *fly.pAcmModel;
	MODEL& MOD = *fly.pModel;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			MOD.Rudder = 0;
			ACM.Thrust (ThrustfromFF());
			SetManoeuvreTime (3000);
			ai.ManStep = PHASE1;
   		}
		case PHASE1:
		{
			PitchIntercept = ANGLES_10Deg;
			AutoCalcPitch();
			SimpleMoveToHdg (HdgIntercept);

			if  (		(!ManoeuvreTimeLeft())
					&&	(Range < ENGAGERANGE)
				)
					ai.manoeuvre = MANOEUVRE_SELECT;
		}
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LeadPursuit
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	1.63
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLeadPursuit()
{
//	ifcombatakill high
//		turn in parallel plane so that target still visible
//
//	calc ideal lead
//		could base calc on maintaining collision course(hdg-hdgintercept?)
//	actual lead depends also on skill
//
// end after x secs or within DANGERRANGE
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LagPursuit
//Author		R. Hyde 
//Date			Tue 10 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLagPursuit()
{
//
//aim for a point off target's tail
//
//
//	acutal lag depends also on skill
//
//
//
// end after x secs or when go behind traget's tail
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Circumvent
//Author		R. Hyde 
//Date			Mon 30 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleCircumvent()
{
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		DecoyActive
//Author		R. Hyde 
//Date			Mon 30 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
//rdhvoid	AirStruc::SimpleDecoyActive()
//{
//}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		DecoyPassive
//Author		R. Hyde 
//Date			Mon 30 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
//rdhvoid	AirStruc::SimpleDecoyPassive()
//{
//}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		RoundAbout
//Author		R. Hyde 
//Date			Mon 30 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleRoundAbout()
{
	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust(ThrustfromFF());
			SetManoeuvreTime (1500);
 			ai.desiredroll = -(ANGLES)ANGLES_60Deg +
					(Angles)(ANGLES_120Deg * Math_Lib.rnd(2)); 


			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			SWord desroll = ai.desiredroll;
			UWord desroll_pos;
			SWord desroll_sign;
			Math_Lib.Pos (desroll,desroll_pos,desroll_sign);
			if	(SubjectFacingTarget(ANGLES_45Deg))
			//when facing unfriendly unroll to move towards it
				desroll_pos = desroll_pos/2;
			else
				desroll_pos = desroll_pos;
			if (desroll_sign==MathLib::NEGATIVE)
				desroll_pos = -desroll_pos;
			desroll = desroll_pos;
			SimpleMoveToRoll ((Angles)desroll);

			PitchIntercept = BestClimbAngle();
			AutoCalcPitch ();
 			if  (		((Range - (World.Y - ai.unfriendly->World.Y)) < ENGAGERANGE)
					&&	((World.Y - ai.unfriendly->World.Y) > FT_3000)
				)
			{
 		    	ai.ManStep = PHASE0;
				ai.manoeuvre = MANOEUVRE_DIVEANDZOOM;
			}

		}
	}

}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SplitManoeuvre
//Author		R. Hyde 
//Date			Tue 17 Mar 1998
//
//Description	
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleSplitManoeuvre()
{


	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust (ThrustfromFF());
			SetManoeuvreTime (1500);
			ai.ManStep = PHASE1;

		}
		case PHASE1:
		{
			PitchIntercept = ai.desiredpitch;
			AutoCalcPitch();
			SimpleMoveToHdg (ai.desiredhdg);
			if  (		(ChkAtDesiredHdg (ai.desiredhdg,ANGLES_40Deg))
					||	(!ManoeuvreTimeLeft())
				)
			{
				RndVal	rndnum = Math_Lib.rnd();
				if (		(rndnum > RND50PC)
						&&	(ai.morale < MORALE_MEDIUM)
					)
				{
					ai.ManStep = PHASE0;
					ai.manoeuvre = MANOEUVRE_EXTENSION;
				}else
					ai.manoeuvre = MANOEUVRE_SELECT;
			}

		}
	}


}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		HeadOn
//Author		R. Hyde 
//Date			Tue 3 Mar 1998
//
//Description	Theoretically no offset, 
//				However use a little to avoid collision
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleHeadOn ( )
{
//use desiredpitch and desiredhdg to determine offset
// distance = 2 WINGSPANS
	//just pass to the side
	//poor pilots may turn too early
	//good pilots may turn early
	//on passing pilot skill will dtermine action

	AirStrucPtr	unfriendly = *ai.unfriendly;

	SLong temp_range;
	temp_range = Range; 	
	COORDS3D	target;
	SWord c,s;
	SWord newhdg;
	UByte	FF = FlyingFactor();

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			InterceptandRange(ai.unfriendly);
			SimpleMoveToThrust (ThrustfromFF());
			SetManoeuvreTime (2000);
			SWord dhdg = HdgIntercept - hdg;
			if ((dhdg) > 0)
				ai.desiredhdg = ANGLES_90Deg;
			else
				ai.desiredhdg = ANGLES_270Deg;
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			InterceptandRange(ai.unfriendly);
			ANGLES bca = BestClimbAngle();
			if (PitchIntercept > bca) PitchIntercept = bca;
			AutoCalcPitch();

			ANGLES desiredhdg = ai.desiredhdg;
			ANGLES unfHdgIntercept = HdgIntercept;
			SLong	unfRange = Range;
			newhdg = unfriendly->hdg + ai.desiredhdg;

			Math_Lib.high_sin_cos((Angles)newhdg,s,c);
			target.X = unfriendly->World.X + ((METRES30 * s) >> ANGLES_SHIFT);
			target.Y = unfriendly->World.Y;
			target.Z = unfriendly->World.Z + ((METRES30 * c) >> ANGLES_SHIFT);
			InterceptandRange (&target);
			desiredhdg = HdgIntercept;

			SWord	reqdeltahdg = hdg - desiredhdg;
		 	ANGLES reqroll = (Angles)FindRequiredRoll (reqdeltahdg,CombatReqBankData);

			int decisionpoint = Math_Lib.rnd(2);

			if  (		(Range < (2 * BREAKTURNRADIUS ))
					&&	(FF < 200)
				)
			{
				if (decisionpoint)
				{
					if (FF < 90)
					{
						ai.manoeuvre = MANOEUVRE_PANICTURN;
						ai.ManStep = PHASE0;
					}else  
					{
						ai.manoeuvre = MANOEUVRE_BREAKTURN;
						ai.ManStep = PHASE0;
					}
				}else
				{
					ai.ManStep = PHASE2;
				}

			}else if	(		(Range < BREAKTURNRADIUS)
							
						)
			{
				if (decisionpoint)
				{
					ai.manoeuvre = MANOEUVRE_BREAKTURN;
					ai.ManStep = PHASE0;
				}else
				{
					ai.desiredhdg = ai.desiredhdg  + ANGLES_180Deg;
					ai.ManStep = PHASE2;
				}
			}

			if  (!ManoeuvreTimeLeft())
				ai.manoeuvre = MANOEUVRE_SELECT;
			break;
		}
		case PHASE2:
		{
			InterceptandRange(ai.unfriendly);
			ANGLES bca = BestClimbAngle();
			if (PitchIntercept > bca ) PitchIntercept = bca;
			AutoCalcPitch();

			ANGLES desiredhdg = ai.desiredhdg;
			ANGLES unfHdgIntercept = HdgIntercept;
			SLong	unfRange = Range;
			newhdg = unfriendly->hdg + ai.desiredhdg;

			Math_Lib.high_sin_cos((Angles)newhdg,s,c);
			target.X = unfriendly->World.X + ((METRES30 * s) >> ANGLES_SHIFT);
			target.Y = unfriendly->World.Y;
			target.Z = unfriendly->World.Z + ((METRES30 * c) >> ANGLES_SHIFT);
			InterceptandRange (&target);
			desiredhdg = HdgIntercept;

			SWord	reqdeltahdg = hdg - ai.desiredhdg;
		 	SWord reqroll = FindRequiredRoll (reqdeltahdg,CombatReqBankData);
			SimpleMoveToRoll((Angles)reqroll);

			if  (!ManoeuvreTimeLeft())
				ai.manoeuvre = MANOEUVRE_SELECT;
			if	(SubjectOnTargetTail(ANGLES_80Deg))
			{
				if	(		(ai.unfriendly == Manual_Pilot.ControlledAC2)
						&&	(((AirStruc*)ai.unfriendly)->Follower())
					)
					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_HEADON, MSG_HEADON, ((AirStruc*)ai.unfriendly)->Follower(), this, ((AirStruc*)ai.unfriendly)));

				int startpoint = Math_Lib.rnd(2);
				ai.ManStep = PHASE0;
				if (FF < 90)
				{
					if (startpoint)
						ai.manoeuvre = MANOEUVRE_JINK;
					else
						ai.manoeuvre = MANOEUVRE_MILDSCISSORS;
				}else if (FF < 200)
				{
					if (startpoint)
							ai.manoeuvre = MANOEUVRE_STRAIGHTANDLEVEL;
					else
							ai.manoeuvre = MANOEUVRE_EXTENSION;
				}else 
				{
					if (startpoint)
							ai.manoeuvre = MANOEUVRE_TURNINGFIGHT;
					else
							ai.manoeuvre = MANOEUVRE_BREAKTURN;
				}
			}
		}
	}
	
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LineAstern
//Author		R. Hyde 
//Date			Tue 17 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLineAstern()
{
//
//use leader's unfriendly
//also use leader's phase
//Phase 0: stay out of way until separation exceeds say 400m
//
//phase 1: if leader 
//				highest speed dive down to target
//			else
//				follow using formation data
//phase 2: when leader in DANGERRANGE
//				do highest speed dive down to target
//			AutoShoot() will do firing	(avoid firing at buddy)
//phase 2:	at 200 yards, zoom climb
//				success depends on flyingskill
//				will break out of manoeuvre if attacked, damaged, winchester, bingo
//phase 3: establish circle around target, go to phase 0				
//
	if (!leader) 
	{//leader, do same as roundabout, get height, dive and then zoom
	 //at end of zoom, choose manoeuvre based on firsttactic
			ai.ManStep = PHASE0;
			ai.manoeuvre = MANOEUVRE_ROUNDABOUT;			
	}else
	{//follower
		if (Range < 2 * WEAPONSRANGE)
		{
			ai.manoeuvre = MANOEUVRE_DIVEANDZOOM;
			ai.ManStep = PHASE0;
		}
		else
		{
			switch (ai.ManStep)
			{
				case PHASE0:
				{
//DeadCode CSB 05/07/99						MOD.Rudder = 0;
					SimpleMoveToThrust (ThrustfromFF());
					ai.ManStep = PHASE1;
//DeadCode CSB 05/07/99						ACM.maxdeltaforclosework = 0.0;
				}
				case PHASE1:
				{
					if (!leader)
						ai.manoeuvre = MANOEUVRE_SELECT;					//if no leader break manoeuvre
					else
					{
 						COORDS3D	target;
						AirStrucPtr MyLeader = Leader();
						for(;;)
						{
							AirStrucPtr Foll = MyLeader->Follower();
							if(Foll == this)
								break;			 
							MyLeader = Foll;
						}
 						target = MyLeader->World; 
						target.X += 10 * MyLeader->velx;
						target.Y += 10 * MyLeader->vely;
						target.Z += 10 * MyLeader->velz;
 						InterceptandRange(&target);

						AutoCalcPitch();
						SWord DeltaHdg = HdgIntercept - hdg;
						if((DeltaHdg > ANGLES_0Deg) && (DeltaHdg < ANGLES_HalfDeg))
							HdgIntercept = hdg + ANGLES_HalfDeg;
						if((-DeltaHdg > ANGLES_0Deg) && (-DeltaHdg < ANGLES_HalfDeg))
							HdgIntercept = hdg - ANGLES_HalfDeg;

						CalcHdgRoll();
//DeadCode CSB 05/07/99							if( roll > ANGLES_60Deg) roll = ANGLES_60Deg;
//DeadCode CSB 05/07/99							if(-roll > ANGLES_60Deg) {roll = ANGLES_60Deg; roll = -roll;}
//DeadCode CSB 05/07/99							Float fpitch, fhdg;
//DeadCode CSB 05/07/99							SWord reqroll = GetRequiredRoll(*(rotitem*)this, (COORDS3D)target, fpitch, fhdg );
//DeadCode CSB 05/07/99							if  (		(	(fpitch < ACM.maxdeltaforclosework)
//DeadCode CSB 05/07/99										&&	(fpitch > -ACM.maxdeltaforclosework)
//DeadCode CSB 05/07/99										&&	(fhdg < ACM.maxdeltaforclosework)
//DeadCode CSB 05/07/99										&&	(fhdg > -ACM.maxdeltaforclosework)
//DeadCode CSB 05/07/99										)
//DeadCode CSB 05/07/99								)
//DeadCode CSB 05/07/99							{
//DeadCode CSB 05/07/99								TurnFightPhase1(HdgIntercept, TRUE, ANGLES_0Deg, target);
//DeadCode CSB 05/07/99							}else
//DeadCode CSB 05/07/99							{
//DeadCode CSB 05/07/99								ANGLES rroll;
//DeadCode CSB 05/07/99								rroll = (Angles)(reqroll + roll);
//DeadCode CSB 05/07/99								ACM.Roll (rroll);
//DeadCode CSB 05/07/99								ACM.ZeroSpecificPower();
//DeadCode CSB 05/07/99							}
						SLong DesVel = MyLeader->vel;
						Range -= MyLeader->vel * 10;
						if (Range < METRES200)
							DesVel -= 100;
 						else
						{
							SLong DeltaVel = (Range - METRES400) / 10;
							MODLIMIT(DeltaVel, 100);
							DesVel += DeltaVel;
						}
						if(DesVel > vel) SimpleMoveToThrust(200);
						else			 SimpleMoveToThrust(0);	

					}
				}
			}
		}
	}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ShootTooEarly
//Author		R. Hyde 
//Date			Mon 30 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::SimpleShootTooEarly()
{
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LagRoll
//Author		R. Hyde 
//Date			Tue 3 Mar 1998
//
//Description	Ref1.67..	
//				manoeuvre incorporates barrel roll attack(1.69..)
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLagRoll()
{
//maintain throttle
//roll towards unfriendly for 10-20deg hdg (f(AOT))
//go upside down
//climb to loose speed to corner speed
//exit manoeuvre when unfriendly goes behind or time out

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ExtensionManoeuvre
//Author		R. Hyde 
//Date			Thu 5 Mar 1998
//
//Description	1.23
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleExtensionManoeuvre()
{
//attempt to get to extend range to ENGAGERANGE safely
//this is to try to escape
//or get distance to try again
//if at same level
//	good mig pilots should go up
//	good us pilots should go down
//if higher than bogey
//	
//
//ref1.70
//if turning good pilots will initially unload and
//retain roll to disguise manoeuvre 
//
//initially aircraft could be closing

}
void	AirStruc::SimpleDivingSpin()
{
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ReverseTurn
//Author		R. Hyde 
//Date			Thu 5 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleReverseTurn()
{
//	DesiredRoll = opposite of existing
//	Move to desired and hold for 3 secs
//	if (good)
//		out of plane
//	Close brakes if open at end of manoeuvre(used in selfasbait())
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SelfAsBait
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	Ref19
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleSelfAsBait()
{
//
//let bandit out turn you
//just as he get lead, pop brakes
//goto RevereTurn
//
//this could be where bandit is on your tail, wait for him to get close 
//before turning
//keep buddy in combat spread so that you can sandwich him
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Jink
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleJink()
{
//
//fast 90 deg roll
//hard push or pull on stick
//hold for 2-3 secs
//
//flyingability determines rollrate and stick push pull
//combatablity determines length of manoeuvre
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		BreakTurn
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	
//				Maximum performance turn towards unfriendly
//				Aim for corner speed (see summary at top of acmman.cpp)
//				When quoted as IAS, corner speed is practically a constant
//				cornerspeed should be part of the planetype
//				corner speed occurs at max g allowed for ac
//				max g should be part of the planetype 
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleBreakTurn()
{
//	roll = cos -1 (1/maxg)
//ability to reach roll = f(flyingskill)
//ability to hold corner vel = f(flyingskill)
//	option to nose down to keep speed up and nose up to reduce speed
//hold for 4-5 secs
//
//low morale gives a very lazy turn
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LazyTurn
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLazyTurn()
{
//
//	Choose a roll 10-40deg
//		= f(morale)
//	continue on turn for 10s
//	if (ai.morale > MORALE_GOOD)
//		if (Range < WEAPONS)
//			end manoeuvre
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Break180
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	1.26
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleBreak180()
{
//roll 180
//pull hard until pitch 45 deg down
//slowly unroll for x secs
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		BreakLow
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleBreakLow()
{
//decide which way to roll to go down
//roll 90
//pull hard until pitch 45 deg down
//slowly unroll for x secs
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		BreakHigh
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleBreakHigh()
{
//decide which way to roll to go up
//roll 90
//pull hard until pitch 45 deg up
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Break90
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	1.27
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleBreak90()
{
//	if (classtype->aerobaticfactor == AEROBATIC_MIG)
//		BreakHigh();
//	else if (	(ai.morale > MORALE_MEDIUM)
//			&&	(ClosureLessThan(MPH50))
//			)
//				BreakHigh();
//			else
//				BreakLow();
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		HiGBarrelRoll
//Author		R. Hyde						Craig Beeston
//Date			Mon 9 Mar 1998				Sun 13 Jun 1999
//
//Description	1.30	
//				Defensive manoeuvre
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleHiGBarrelRoll()
{
	switch(ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust(0);
			ai.ManStep = PHASE1;
		}
		case PHASE1:
		{
			PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();
			SWord DesRoll = ANGLES_45Deg;	DesRoll = -DesRoll;
			SimpleMoveToRoll(DesRoll);

			if(((SWord)pitch == (SWord)PitchIntercept) && ((SWord)roll == DesRoll))
				ai.ManStep = PHASE2;
		
			break;
		}
		case PHASE2:
		{
			PitchIntercept = ANGLES_45Deg;
			AutoCalcPitch();
			SWord DesRoll = 2 * SWord(pitch);
			SimpleMoveToRoll(DesRoll);

			if(((SWord)pitch == (SWord)PitchIntercept) && ((SWord)roll == DesRoll))
				ai.ManStep = PHASE3;
		
			break;
		}
		case PHASE3:
		{
			PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();
			SWord DesRoll = ANGLES_180Deg - 2 * SWord(pitch);
			SimpleMoveToRoll(DesRoll);

			SWord dHdg = CalcHdg();
			hdg -= 2 * dHdg;

			if(((SWord)pitch == (SWord)PitchIntercept) && ((SWord)roll == DesRoll))
				ai.ManStep = PHASE4;
		
			break;
		}
		case PHASE4:
		{
			PitchIntercept = ANGLES_45Deg;	PitchIntercept = -PitchIntercept;
			AutoCalcPitch();
			SWord DesRoll =  -2 * SWord(pitch) - ANGLES_180Deg;
			SimpleMoveToRoll(DesRoll);

			SWord dHdg = CalcHdg();
			hdg -= 2 * dHdg;

			if(((SWord)pitch == (SWord)PitchIntercept) && ((SWord)roll == DesRoll))
				ai.ManStep = PHASE5;
		
			break;
		}
		case PHASE5:
		{
			PitchIntercept = ANGLES_0Deg;
			AutoCalcPitch();
			SWord DesRoll =  2 * SWord(pitch);
			SimpleMoveToRoll(DesRoll);

			if(((SWord)pitch == (SWord)PitchIntercept) && ((SWord)roll == DesRoll))
			{
				SimpleMoveToThrust(ThrustfromFF());
				ai.manoeuvre = MANOEUVRE_SELECT;
			}
		
			break;
		}
	}
		
//DeadCode CSB 13/06/99			case PHASE1:
//DeadCode CSB 13/06/99			{
//DeadCode CSB 13/06/99				SimpleMoveToRoll(-ANGLES_45Deg);
//DeadCode CSB 13/06/99				PitchIntercept = ANGLES_0Deg;
//DeadCode CSB 13/06/99				AutoCalcPitch();
//DeadCode CSB 13/06/99				if(-(SWord)roll == ANGLES_45Deg)
//DeadCode CSB 13/06/99					ai.ManStep = PHASE2;
//DeadCode CSB 13/06/99				break;
//DeadCode CSB 13/06/99			}
//DeadCode CSB 13/06/99			case PHASE2:
//DeadCode CSB 13/06/99			{
//DeadCode CSB 13/06/99				SimpleMoveToRoll(roll + ANGLES_90Deg);
//DeadCode CSB 13/06/99				SWord sin, cos;
//DeadCode CSB 13/06/99				Math_Lib.high_sin_cos(roll, sin, cos);
//DeadCode CSB 13/06/99	
//DeadCode CSB 13/06/99				PitchIntercept = (Angles)(sin / 2);
//DeadCode CSB 13/06/99	 			AutoCalcPitch();
//DeadCode CSB 13/06/99	
//DeadCode CSB 13/06/99				if(-(SWord)roll > ANGLES_90Deg)
//DeadCode CSB 13/06/99					ai.ManStep = PHASE3;
//DeadCode CSB 13/06/99				break;
//DeadCode CSB 13/06/99			}
//DeadCode CSB 13/06/99			case PHASE3:
//DeadCode CSB 13/06/99			{
//DeadCode CSB 13/06/99				SimpleMoveToRoll(roll + ANGLES_90Deg);
//DeadCode CSB 13/06/99				SWord sin, cos;
//DeadCode CSB 13/06/99				Math_Lib.high_sin_cos(roll, sin, cos);
//DeadCode CSB 13/06/99	
//DeadCode CSB 13/06/99				PitchIntercept = (Angles)(sin / 2);
//DeadCode CSB 13/06/99	  			AutoCalcPitch();
//DeadCode CSB 13/06/99	
//DeadCode CSB 13/06/99				if((SWord)roll > ANGLES_0Deg)
//DeadCode CSB 13/06/99				{
//DeadCode CSB 13/06/99					SimpleMoveToThrust(ThrustfromFF());
//DeadCode CSB 13/06/99					ai.manoeuvre = MANOEUVRE_SELECT;
//DeadCode CSB 13/06/99				}
//DeadCode CSB 13/06/99				break;
//DeadCode CSB 13/06/99			}
//DeadCode CSB 13/06/99		}
	
//max g, roll in one direction
//nose low or high
//unbalanced flight, power reduction, increased drag for better pilots
//
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		PanicTurn
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	1.30
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimplePanicTurn()
{
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		UnBalancedFlight
//Author		R. Hyde 
//Date			Mon 9 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleUnBalancedFlight()
{
//skid and slip while making turns
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		TrackingShot
//Author		R. Hyde 
//Date			Tue 10 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleTrackingShot()
{
//aim for gun solution
//actually modify pitch and heading within limits on top of changes produced by manoeuvre
//just enough lead
//modify speed(rpm and airbrakes) to stay on tail
//
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SnapShot
//Author		R. Hyde 
//Date			Tue 10 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleSnapShot()
{
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		StayOnTail
//Author		R. Hyde 
//Date			Fri 20 Mar 1998
//
//Description	This is not a good idea, very aggressive
//				Stay at corner speed or less
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleStayOnTail()
{
//	if necessayr shed energy:
//		turn
//		brakes
//		throttle
//	to stay on tail

}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		TurnToHdgAndPitch
//Author		R. Hyde 
//Date			Tue 17 Mar 1998
//
//Description	move to within 5 deg of desiredpitch and desiredhdg 
//				break or sustained turn depending on circumstances
//				if break then nose down if <corner vel
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
Bool	AirStruc::SimpleTurnToHdgAndPitch()
{
	//return true on success
	return(TRUE);
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LagFight
//Author		R. Hyde 
//Date			Fri 13 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleHotSideLagPursuitFight()
{
//1	trgItem is behind target
//	TurningFightSub()
}

//------------------------------------------------------------------------------
Bool	AirStruc::SimpleSandwichManoeuvres()
{
	return(TRUE);
}
//void	AirStruc::SimpleTurningFight()
//{
//}
//void	AirStruc::SimpleTightTurnSubtSub(ANGLES	HdgIntercept)
//{
//}
//
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		SustainedTurn
//Author		R. Hyde 
//Date			Fri 13 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleSustainedTurn()
{
//1	GetRequiredRoll(srcItem,trgItem)
//2	pull on elevators so that Drag = Thrust 
//			this is the max sustainable turn, in-plane
//turn to minimise the difference between hdgintercept and hdg, go shortest way
//
//
//	roll = f(morale, flyingskill)
//	drag = f(combatskill)
//
//
//	if (skill high)
//			check lead/lag to avoid turning into target
//
//	turn at best sustained turnrate  (how near = f(skill), maintain altitude if possible
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		HighYoYo
//Author		R. Hyde 
//Date			Wed 11 Mar 1998
//
//Description	Ref1.70
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleHighYoYo()
{
//approach from 30-60 AOT, cospeed
//
//roll wings level
//pull up
//when in rear hemisphere (say near 6 o'clock)
//	roll towards target then lead or lag pursuit
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		Zoom
//Author		R. Hyde 
//Date			Wed 11 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleZoom()
{
//roll wings nearly level
//pull up then zoom climb for x secs or near stall
//roll 180 and pull level
//
//
//	ACM.Step = ai.ManStep;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SetManoeuvreTime (2000);

			SimpleMoveToThrust (100);			//rdh
//			ACM.ClimbRate (0);
//			ACM.KeepWingsLevel ();

			ai.ManStep = PHASE1;
			break;
		}
		case PHASE1:
		{
				SimpleMoveToRoll (ANGLES_0Deg);
				PitchIntercept = ANGLES_40Deg;
				AutoCalcPitch ();
			

			if  (		(!ManoeuvreTimeLeft ())
					||
						(vel <= CornerSpeed())
					||
						((World.Y - ai.unfriendly->World.Y)	>	FT_4000)
				)
			{
 				if (ai.firsttactic == TACTIC_ROUNDABOUT)
 					ai.manoeuvre = MANOEUVRE_ROUNDABOUT;
 				else if (ai.firsttactic == TACTIC_LINEABREAST)
 					ai.manoeuvre = MANOEUVRE_LINEABREAST;
 				else if (ai.firsttactic == TACTIC_LINEASTERN)
 					ai.manoeuvre = MANOEUVRE_LINEASTERN;
 				else if (ai.firsttactic == TACTIC_DIVEANDZOOM)
 					ai.manoeuvre = MANOEUVRE_DIVEANDZOOM;
				else
					ai.manoeuvre = MANOEUVRE_SELECT;

			}
			break;
		}
	}
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		LowYoYo
//Author		R. Hyde 
//Date			Tue 3 Mar 1998
//
//Description	Ref1.73	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLowYoYo ( )
{
//used when closure is insufficient
//
//in turning fight, attacker increases horizontal component of turn rate by nose down
//lead target
//level wings and pull up
//
//Note may have to write so that the ac doesn't try to get advantage in one 
//manoeuvre (see btm 1.73)
//
//
//
//Note can also be used when in tailchase to pick up speed to get just inside gun range
//
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		InterceptHigh
//Author		R. Hyde 
//Date			Wed 11 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleInterceptHigh()
{
//intercept but stay at alt
//
}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ClimbAtSustainedTurnSpeed
//Author		R. Hyde 
//Date			Fri 20 Mar 1998
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleClimbAtSustainedTurnSpeed()
{
//climb but don't go below sustained turn speed
//aim towards unfriendly plus offset = f(desiredrange,desiredhdg)

}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		
//Author		R. Hyde 
//Date			 //RDH 25/05/99
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::SimpleLowAlt()
{

	ANGLES dpitch;

	switch (ai.ManStep)
	{
		case PHASE0:
		{
			SimpleMoveToThrust (ThrustfromFF());
			SetManoeuvreTime (800);
			if (TargetFacingSubject(ANGLES_45Deg))
				ai.desiredpitch = ANGLES_5Deg;
			else
				ai.desiredpitch = ANGLES_10Deg;
			ai.ManStep = PHASE1;

		}
		case PHASE1:
		{
//DEADCODE RDH 18/06/99 			if (TargetFacingSubject(ANGLES_45Deg))
//DEADCODE RDH 18/06/99 				dpitch = ANGLES_5Deg;
//DEADCODE RDH 18/06/99 			else
//DEADCODE RDH 18/06/99 				dpitch = ANGLES_10Deg;

			PitchIntercept = ai.desiredpitch;
			AutoCalcPitch();
			SimpleMoveToHdg (HdgIntercept);

			if  (!ManoeuvreTimeLeft())
							ai.manoeuvre = MANOEUVRE_SELECT;

		}
	}


}
