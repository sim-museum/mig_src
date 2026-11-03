/* ACL-ENGINE story 2 (PO 11-02): the ACL runtime inside MA. A hero pilot's doctrine and manoeuvres, written in the
   Shaw air-combat language (~/acl, the Python reference engine), are read at run time and re-decided every frame.
   This header has no MA types: bodies are metres, z up, so the same code is unit-tested outside the game
   (tools/acl_cpp_test.sh). MA_ACL.CPP's adapter turns MA's AirStruc into AclBody and the output into ACM controls. */
#ifndef MA_ACL_H
#define MA_ACL_H
#include <string>
#include <vector>

struct AclBody { double pos[3], vel[3]; bool dead; double sustained_n; int id, target_id; };	/* sustained_n: level sustained load factor now; target_id: whom he attacks (-1) */

struct AclNode;	/* an expression tree (lang.py Expr) */

struct AclPhase
{	std::string name, aim; std::vector<double> args; std::string argname;	/* aim lead(bandit) / climb(89 deg) */
	double gain; int gmode; double g;	/* gmode 0 = none, 1 = max, 2 = sustained, 3 = fixed g */
	double throttle; bool hold, brake; AclNode* until; int line; };

struct AclManoeuvre { std::string name; std::vector<AclPhase> phases; AclNode* fail; };

struct AclRule { AclNode* cond; std::string man, target; int line; };	/* cond NULL = otherwise */

struct AclProgram
{	std::vector<AclManoeuvre> mans; std::vector<std::pair<std::string, std::vector<AclRule> > > doctrines;
	std::string error;	/* empty when the parse succeeded */
	bool parse(const std::string& text);
	const AclManoeuvre* find(const std::string& name) const;
	const std::vector<AclRule>* doctrine(const std::string& name) const; };

struct AclOut { double aim[3]; double nmax; bool force; double throttle; bool brake; bool nose; std::string label; };	/* nose: aim the gun line (body axis), not the flight path */

class AclPilot
{public:
	AclPilot() : prog(0), rules(0), rule(-1), man(0), phase(0), tp(0.0), v0(-1.0), t(0.0), tgt(0), tgt_id(-1) {}
	bool init(const AclProgram* p, const std::string& doctrine);
	/* one decision + control step: me, the enemies (target = the nearest live one), dt (s). false = nothing to fly */
	bool step(const AclBody& me, const std::vector<const AclBody*>& enemies, double dt, AclOut& out, const AclBody* partner = 0);
	std::string event;	/* the last rule/phase change, for the trace */
	const AclProgram* prog; const std::vector<AclRule>* rules; int rule; const AclManoeuvre* man; int phase; double tp, v0, t;
	const AclBody* tgt; int tgt_id; };	/* tgt: this step's body of the target (re-found by tgt_id each step) */

/* Lift direction and load factor that turn the velocity toward `aim` (prim.py lift_for_direction): mu is the bank
   from the vertical lift plane (rad, + right), n the load factor. */
void acl_lift_for_direction(const double vel[3], const double aim[3], double gain, double nmax, double& mu, double& n);
#endif
