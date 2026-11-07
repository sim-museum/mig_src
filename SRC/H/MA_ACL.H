/* ACL-ENGINE story 2 (PO 11-02): the ACL runtime inside MA. A hero pilot's doctrine and manoeuvres, written in the
   Shaw air-combat language (~/acl, the Python reference engine), are read at run time and re-decided every frame.
   This header has no MA types: bodies are metres, z up, so the same code is unit-tested outside the game
   (tools/acl_cpp_test.sh). MA_ACL.CPP's adapter turns MA's AirStruc into AclBody and the output into ACM controls. */
#ifndef MA_ACL_H
#define MA_ACL_H
#include <string>
#include <vector>
#include <map>

struct AclBody { double pos[3], vel[3]; bool dead; double sustained_n; int id, target_id; bool damaged, bingo; int allies;
	bool big, crosser, has_home, cover; double yalu_km, limit_km, home[3];
	int sk_eyes = -1, sk_flying = -1, sk_gunnery = -1, sk_nerve = -1; double fear = -1.0; };	/* fear: PILOT-PANIC-1, the inner world's (0..255; -1 none) */	/* PILOT-SUBSKILL-1: this pilot's own (only `me` carries them; -1 = not set) */	/* DOCTRINE-1 (PO 11-04 period constraints): damaged = critical damage (smoking/seized engine); bingo = fuel for home only;
   allies = his side within 5 km, himself included; big = a bomber (seen from twice as far); crosser = this pilot would
   follow a MiG over the Yalu (Sabre) / south of the Pyongyang-Wonsan line (MiG); yalu_km = km on the Korean side of the
   Yalu (< 0 over Manchuria), limit_km = km north of the Pyongyang-Wonsan line (1e9 = unknown); home = his base;
   cover = a MiG of the group's cover flights (Soviet gruppa prikrytiya: holds height, comes down on a Sabre attacking a MiG) */	/* sustained_n: level sustained load factor now; target_id: whom he attacks (-1) */

struct AclNode;	/* an expression tree (lang.py Expr) */

struct AclPhase
{	std::string name, aim; std::vector<double> args; std::string argname;	/* aim lead(bandit) / climb(89 deg) */
	double gain; int gmode; double g;	/* gmode 0 = none, 1 = max, 2 = sustained, 3 = fixed g */
	double throttle; bool hold, brake, eject = false; AclNode* until; int line; };   /* eject: PILOT-PANIC-1 */

struct AclManoeuvre { std::string name; std::vector<AclPhase> phases; AclNode* fail; int needs; };	/* needs: least MA skill (0 novice .. 15 hero) that can fly it -- "manoeuvre m(bandit) needs veteran:" */

/* ACL-CHOICE-1: one weighted option of a "do one of:" rule -- "<w> [when <cond>] [needs <skill>]: m(target)" */
struct AclOption { double w; AclNode* cond; std::string man, target; int needs, line; };
struct AclRule { AclNode* cond; std::string man, target; int line; bool choice = false; std::vector<AclOption> opts; };	/* cond NULL = otherwise; choice: man/target come from a drawn option */

struct AclProgram
{	std::vector<AclManoeuvre> mans; std::vector<std::pair<std::string, std::vector<AclRule> > > doctrines;
	std::string error;	/* empty when the parse succeeded */
	bool parse(const std::string& text);
	const AclManoeuvre* find(const std::string& name) const;
	const std::vector<AclRule>* doctrine(const std::string& name) const; };

struct AclOut { double aim[3]; double nmax; bool force; double throttle; bool brake; bool nose; std::string label; bool eject = false; };	/* nose: aim the gun line (body axis), not the flight path */

class AclPilot
{public:
	AclPilot() : prog(0), rules(0), rule(-1), man(0), phase(0), tp(0.0), v0(-1.0), t(0.0), tgt(0), tgt_id(-1), skill(15), think_dt(0.0), next_think(0.0), rear_eye(0.0), gcap(0.0), can_force(true), front_eye(6000.0), mem_s(10.0), blind_deg(0.0), fog(false), gci(false), assigned(false) {}
	bool init(const AclProgram* p, const std::string& doctrine);
	/* one decision + control step: me, the enemies (target = the nearest live one), dt (s). false = nothing to fly */
	bool step(const AclBody& me, const std::vector<const AclBody*>& enemies, double dt, AclOut& out, const AclBody* partner = 0);
	std::string event;	/* the last rule/phase change, for the trace */
	const AclProgram* prog; const std::vector<AclRule>* rules; int rule; const AclManoeuvre* man; int phase; double tp, v0, t;
	const AclBody* tgt; int tgt_id;
	/* PO 11-04 "make the shaw engine fly to each pilot's skill": set from ai.combatskill by the adapter. skill gates the
	   manoeuvres (needs); think_dt = seconds between re-decisions (0 = every step); rear_eye = range (m) at which he sees an
	   enemy behind his 3-9 line (0 = all); gcap = the most g he will pull (0 = the limit); can_force = "g max" may hit the lift limit */
	int skill; double think_dt, next_think, rear_eye, gcap; bool can_force;
	/* FOG-1 (PO 11-04 "model the fog of war"): he knows an enemy he sees (ahead within front_eye, behind his 3-9 line within
	   rear_eye, never inside blind_deg of dead astern), one his partner within 3 km sees (the call-out), and his GCI target
	   (gci: the MiG's controller vectors him onto the bombers). Out of sight, he flies on where he last saw him, moved on
	   by the velocity he had then, for mem_s seconds. fog = false: he knows every enemy's true position (before 11-04). */
	double front_eye, mem_s, blind_deg; bool fog, gci;
	struct Seen { int id; double ts; AclBody last; }; std::vector<Seen> seen; std::vector<AclBody> ghosts;
	bool escort_logged = false;	/* TUNE-ESCORT-1: the cover MiG's [aclescort] line is printed once */
	/* THREAT-1: a threat integrator. Per enemy he knows of, a level that grows with how squarely the enemy points at
	   him, how close and how far behind him he is and his energy edge, and decays (half-life ~1.4 s). threat_level()
	   is the highest; the defend rules fire when it passes react(), which falls with eyes and nerve. */
	struct Threat { int id; double level; }; std::vector<Threat> threats; double tl_max = 0.0; int tl_id = -1;
	unsigned rng_spot = 0;   /* FOG-2: its own stream, so the choice draws (rng) are not shifted */
	double gate_t = -1.0, gate_clear_t = -1.0;   /* for the [threat] reaction-time trace: when the old 1 km / 20 deg gate first held */
	unsigned rng = 0;	/* ACL-CHOICE-1: his own xorshift32 stream, seeded from (aircraft id, census start) so a fight's draws replay */
	bool assigned;	/* "target" = me.target_id (the game's own choice, e.g. a MiG's bomber) while he lives, else the nearest */ };	/* tgt: this step's body of the target (re-found by tgt_id each step) */

/* Lift direction and load factor that turn the velocity toward `aim` (prim.py lift_for_direction): mu is the bank
   from the vertical lift plane (rad, + right), n the load factor. */
void acl_lift_for_direction(const double vel[3], const double aim[3], double gain, double nmax, double& mu, double& n);
#endif
