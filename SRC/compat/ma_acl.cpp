/* ACL-ENGINE story 2 (PO 11-02): the ACL runtime inside MA -- see SRC/H/MA_ACL.H. This file mirrors the Python
   reference engine (~/acl: lang.py Expr/parse, engine.py UserPrim/think, prim.py lift_for_direction) for the part
   a hero pilot needs: doctrine rules ("when <cond>: do m(target)", "otherwise: do m(target)"), manoeuvres written
   in ACL (phase ... aim ... g ... until ...), and the expressions they test. No MA types here.
   No iostreams: MA builds with -fpack-struct=1, and a std::stringstream instantiated under it does not match the
   libstdc++ library's layout (stack smashing in the game, fine natively). std::string/std::vector are pointer-only. */
#include "MA_ACL.H"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>

/* ---- expressions (lang.py Expr) ------------------------------------------------------------------------------ */
struct AclNode { int k; double v; std::string s; std::vector<AclNode*> a; };	/* k: see AclK */
enum AclK { K_NUM, K_NAME, K_CALL, K_NOT, K_AND, K_OR, K_LT, K_GT, K_LE, K_GE, K_EQ, K_NE, K_ADD, K_SUB };

struct AclTok { int kind; double v; std::string s; };	/* kind 0 num, 1 op, 2 id */

static double acl_unit(const std::string& u)
{	if (u == "km") return 1000.0; if (u == "ft") return 0.3048; if (u == "min") return 60.0; if (u == "kt") return 0.514444;
	return 1.0; }	/* m, s, deg, m/s, deg/s, g, mach: as written */

static bool acl_tokenize(const std::string& s, std::vector<AclTok>& out)
{	static const char* units[] = { "m/s", "deg/s", "mach", "km", "ft", "kt", "deg", "min", "s", "m", "g", 0 };
	size_t i = 0;
	while (i < s.size())
	{	if (isspace((unsigned char)s[i])) { i++; continue; }
		AclTok t; t.v = 0.0;
		if (isdigit((unsigned char)s[i]) || (s[i] == '.' && i + 1 < s.size() && isdigit((unsigned char)s[i + 1])))
		{	size_t j = i; while (j < s.size() && (isdigit((unsigned char)s[j]) || s[j] == '.')) j++;
			t.kind = 0; t.v = atof(s.substr(i, j - i).c_str()); i = j;
			size_t k = i; while (k < s.size() && s[k] == ' ') k++;
			for (int u = 0; units[u]; u++)
			{	const size_t n = strlen(units[u]);
				if (s.compare(k, n, units[u]) == 0 && (k + n == s.size() || !(isalnum((unsigned char)s[k + n]) || s[k + n] == '_')))
				{	t.v *= acl_unit(units[u]); i = k + n; break; } }
			out.push_back(t); continue; }
		if (isalpha((unsigned char)s[i]) || s[i] == '_')
		{	size_t j = i; while (j < s.size() && (isalnum((unsigned char)s[j]) || s[j] == '_' || s[j] == '.')) j++;
			t.kind = 2; t.s = s.substr(i, j - i); i = j; out.push_back(t); continue; }
		static const char* ops2[] = { "<=", ">=", "==", "!=", 0 };
		bool two = false;
		for (int o = 0; ops2[o]; o++) if (s.compare(i, 2, ops2[o]) == 0) { t.kind = 1; t.s = ops2[o]; i += 2; two = true; break; }
		if (two) { out.push_back(t); continue; }
		if (strchr("<>()+-*/,:", s[i])) { t.kind = 1; t.s = std::string(1, s[i]); i++; out.push_back(t); continue; }
		return false; }
	return true; }

struct AclParser
{	std::vector<AclTok> t; size_t i; bool bad;
	AclParser(const std::string& s) : i(0), bad(false) { if (!acl_tokenize(s, t)) bad = true; }
	bool is(int kind, const char* s) const { return i < t.size() && t[i].kind == kind && t[i].s == s; }
	static AclNode* mk(int k, AclNode* a = 0, AclNode* b = 0) { AclNode* n = new AclNode; n->k = k; n->v = 0.0; if (a) n->a.push_back(a); if (b) n->a.push_back(b); return n; }
	AclNode* p_or() { AclNode* a = p_and(); while (is(2, "or")) { i++; a = mk(K_OR, a, p_and()); } return a; }
	AclNode* p_and() { AclNode* a = p_not(); while (is(2, "and")) { i++; a = mk(K_AND, a, p_not()); } return a; }
	AclNode* p_not() { if (is(2, "not")) { i++; return mk(K_NOT, p_not()); } return p_cmp(); }
	AclNode* p_cmp()
	{	AclNode* a = p_sum();
		static const char* c[] = { "<", ">", "<=", ">=", "==", "!=" }; static const int k[] = { K_LT, K_GT, K_LE, K_GE, K_EQ, K_NE };
		for (int j = 0; j < 6; j++) if (is(1, c[j])) { i++; return mk(k[j], a, p_sum()); }
		return a; }
	AclNode* p_sum() { AclNode* a = p_atom(); while (is(1, "+") || is(1, "-")) { const int k = t[i].s == "+" ? K_ADD : K_SUB; i++; a = mk(k, a, p_atom()); } return a; }
	AclNode* p_atom()
	{	if (i >= t.size()) { bad = true; return mk(K_NUM); }
		if (t[i].kind == 0) { AclNode* n = mk(K_NUM); n->v = t[i++].v; return n; }
		if (is(1, "(")) { i++; AclNode* a = p_or(); if (is(1, ")")) i++; else bad = true; return a; }
		if (t[i].kind == 2)
		{	AclNode* n = mk(K_NAME); n->s = t[i++].s;
			if (is(1, "("))
			{	i++; n->k = K_CALL;
				if (!is(1, ")")) { n->a.push_back(p_or()); while (is(1, ",")) { i++; n->a.push_back(p_or()); } }
				if (is(1, ")")) i++; else bad = true; }
			return n; }
		bad = true; i++; return mk(K_NUM); } };

static AclNode* acl_expr(const std::string& s, std::string& err)
{	AclParser p(s); AclNode* n = p.p_or();
	if (p.bad || p.i != p.t.size()) { err = "cannot read condition '" + s + "'"; return 0; }
	return n; }

/* ---- geometry (model.py) ---------------------------------------------------------------------------------------- */
static double dot3(const double* a, const double* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static double norm3(const double* a) { return sqrt(dot3(a, a)); }
static double ang3(const double* a, const double* b)
{	const double n = norm3(a) * norm3(b); if (n < 1e-12) return 0.0; double c = dot3(a, b) / n; if (c > 1) c = 1; if (c < -1) c = -1; return acos(c); }
static double acl_rng(const AclBody& a, const AclBody& b) { double d[3] = { b.pos[0] - a.pos[0], b.pos[1] - a.pos[1], b.pos[2] - a.pos[2] }; return norm3(d); }
static double acl_aon(const AclBody& a, const AclBody& b)	/* b this far off a's nose (deg) */
{	double d[3] = { b.pos[0] - a.pos[0], b.pos[1] - a.pos[1], b.pos[2] - a.pos[2] }; return ang3(a.vel, d) * 57.29578; }
static double acl_aot(const AclBody& a, const AclBody& b)	/* a's angle off b's tail (deg) */
{	double d[3] = { a.pos[0] - b.pos[0], a.pos[1] - b.pos[1], a.pos[2] - b.pos[2] }; return 180.0 - ang3(b.vel, d) * 57.29578; }
static double acl_closure(const AclBody& a, const AclBody& b)
{	double d[3] = { b.pos[0] - a.pos[0], b.pos[1] - a.pos[1], b.pos[2] - a.pos[2] }; const double r = norm3(d); if (r < 1e-9) return 0.0;
	double rv[3] = { a.vel[0] - b.vel[0], a.vel[1] - b.vel[1], a.vel[2] - b.vel[2] }; return dot3(rv, d) / r; }

/* ---- evaluation (lang.py _ev, engine.py Ctx/_Phase) -------------------------------------------------------------- */
struct AclVal { int k; double v; const AclBody* b; };	/* k: 0 none, 1 number/bool, 2 body */
struct AclCtx { const AclBody* me; const AclBody* target; const std::vector<const AclBody*>* enemies; double t; std::string param; const AclBody* bound; const AclBody* partner; };

static AclVal vnum(double v) { AclVal r; r.k = 1; r.v = v; r.b = 0; return r; }
static AclVal vnone() { AclVal r; r.k = 0; r.v = 0; r.b = 0; return r; }
static AclVal vbody(const AclBody* b) { AclVal r; r.k = b ? 2 : 0; r.v = 0; r.b = b; return r; }
static bool truthy(const AclVal& v) { return v.k == 2 || (v.k == 1 && v.v != 0.0); }

static AclVal acl_ev(const AclNode* n, const AclCtx& c)
{	switch (n->k)
	{	case K_NUM: return vnum(n->v);
		case K_NAME:
			if (n->s == "t") return vnum(c.t);
			if (n->s == "me") return vbody(c.me);
			if (n->s == "target" || (!c.param.empty() && n->s == c.param)) return vbody(c.target);
			if (n->s == "threat")	/* engine.py: an enemy inside 1.5 km with me within 40 deg of his nose */
			{	const AclBody* best = 0; double br = 1e18;
				for (size_t i = 0; c.enemies && i < c.enemies->size(); i++)
				{	const AclBody* e = (*c.enemies)[i]; const double r = acl_rng(*c.me, *e);
					if (!e->dead && r < 1500.0 && acl_aon(*e, *c.me) < 40.0 && r < br) { br = r; best = e; } }
				return vbody(best); }
			if (n->s == "partner") return vbody(c.partner && !c.partner->dead ? c.partner : 0);	/* story 3: my element partner */
			if (n->s == "partner_target")	/* story 3: the enemy my partner is fighting (double attack's free fighter) */
			{	if (!c.partner || c.partner->dead || c.partner->target_id < 0) return vnone();
				for (size_t i = 0; c.enemies && i < c.enemies->size(); i++) if ((*c.enemies)[i]->id == c.partner->target_id && !(*c.enemies)[i]->dead) return vbody((*c.enemies)[i]);
				return vnone(); }
			if (n->s == "true") return vnum(1.0);
			if (n->s == "false") return vnum(0.0);
			return vnone();
		case K_CALL:
		{	std::vector<AclVal> a; for (size_t i = 0; i < n->a.size(); i++) a.push_back(acl_ev(n->a[i], c));
			const std::string& f = n->s;
			if (f == "dead") return vnum(!a.empty() && a[0].k == 2 && a[0].b->dead);
			if (f == "attacks") return vnum(a.size() == 2 && a[0].k == 2 && a[1].k == 2 && a[0].b->target_id >= 0 && a[0].b->target_id == a[1].b->id);
			for (size_t i = 0; i < a.size(); i++) if (a[i].k == 0) return vnone();
			const AclBody& me = *c.me; const AclBody* b = (!a.empty() && a[0].k == 2) ? a[0].b : 0;
			if (f == "speed") return vnum(norm3(b ? b->vel : me.vel));
			if (f == "alt") return vnum(b ? b->pos[2] : me.pos[2]);
			if (!b) return vnone();
			if (f == "range") return vnum(a.size() == 2 && a[1].k == 2 ? acl_rng(*b, *a[1].b) : acl_rng(me, *b));	/* range(x, y): between two others (fig 5-3) */
			if (f == "aot") return vnum(acl_aot(me, *b));
			if (f == "aon") return vnum(a.size() == 2 && a[1].k == 2 ? acl_aon(*b, *a[1].b) : acl_aon(me, *b));	/* aon(x, y): y off x's nose (fig 5-5) */
			if (f == "his_aon") return vnum(acl_aon(*b, me));
			if (f == "his_aot") return vnum(acl_aot(*b, me));
			if (f == "closure") return vnum(acl_closure(me, *b));
			if (f == "in_guns") return vnum(acl_rng(me, *b) < 650.0 && acl_aon(me, *b) < 20.0);
			return vnone(); }
		case K_NOT: return vnum(!truthy(acl_ev(n->a[0], c)));
		case K_AND: return vnum(truthy(acl_ev(n->a[0], c)) && truthy(acl_ev(n->a[1], c)));
		case K_OR: return vnum(truthy(acl_ev(n->a[0], c)) || truthy(acl_ev(n->a[1], c)));
		case K_ADD: case K_SUB:
		{	const AclVal x = acl_ev(n->a[0], c), y = acl_ev(n->a[1], c);
			if (x.k != 1 || y.k != 1) return vnone();
			return vnum(n->k == K_ADD ? x.v + y.v : x.v - y.v); }
		default:
		{	const AclVal x = acl_ev(n->a[0], c), y = acl_ev(n->a[1], c);
			if (x.k != 1 || y.k != 1) return vnum(0.0);	/* a query about someone who is not there is false */
			switch (n->k) { case K_LT: return vnum(x.v < y.v); case K_GT: return vnum(x.v > y.v); case K_LE: return vnum(x.v <= y.v);
				case K_GE: return vnum(x.v >= y.v); case K_EQ: return vnum(x.v == y.v); default: return vnum(x.v != y.v); } } } }

/* ---- program (lang.py parse, the doctrine/manoeuvre part) --------------------------------------------------------- */
static std::string trim(const std::string& s) { size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r"); return a == std::string::npos ? "" : s.substr(a, b - a + 1); }
static std::string uncomment(const std::string& s) { const size_t h = s.find('#'); return h == std::string::npos ? s : s.substr(0, h); }

static std::vector<std::string> split(const std::string& s, char sep)	/* sep ' ' = any run of blanks */
{	std::vector<std::string> out; std::string cur;
	for (size_t i = 0; i <= s.size(); i++)
	{	const bool end = i == s.size() || (sep == ' ' ? (s[i] == ' ' || s[i] == '\t') : s[i] == sep);
		if (end) { if (sep != ' ' || !cur.empty()) out.push_back(cur); cur.clear(); } else cur += s[i]; }
	return out; }
static std::string lineerr(int ln, const std::string& m) { char b[32]; snprintf(b, sizeof b, "line %d: ", ln); return b + m; }

static bool parse_do(const std::string& s, AclRule& r)
{	std::string x = trim(s); if (x.compare(0, 3, "do ") != 0) return false; x = trim(x.substr(3));
	const size_t p = x.find('('), q = x.rfind(')'); if (p == std::string::npos || q == std::string::npos || q < p) return false;
	r.man = trim(x.substr(0, p)); r.target = trim(x.substr(p + 1, q - p - 1)); return !r.man.empty(); }

static bool parse_phase(const std::string& body, AclPhase& ph, std::string& err)	/* "<name>: aim <dir>[(args)] opts [until cond]" */
{	const size_t colon = body.find(':'); if (colon == std::string::npos) { err = "phase without ':'"; return false; }
	ph.name = trim(body.substr(0, colon)); std::string rest = trim(body.substr(colon + 1));
	ph.gain = 1.6; ph.gmode = 0; ph.g = 0.0; ph.throttle = 1.0; ph.hold = ph.brake = false; ph.until = 0;
	std::string cond; const size_t u = rest.find(" until "); if (u != std::string::npos) { cond = trim(rest.substr(u + 7)); rest = trim(rest.substr(0, u)); }
	if (rest.compare(0, 4, "aim ") != 0) { err = "phase " + ph.name + ": expected 'aim'"; return false; }
	rest = trim(rest.substr(4));
	size_t e = 0; while (e < rest.size() && (isalpha((unsigned char)rest[e]) || rest[e] == '_')) e++;
	ph.aim = rest.substr(0, e); rest = trim(rest.substr(e));
	if (!rest.empty() && rest[0] == '(')
	{	const size_t q = rest.find(')'); if (q == std::string::npos) { err = "phase " + ph.name + ": ')' missing"; return false; }
		const std::vector<std::string> as = split(rest.substr(1, q - 1), ',');
		for (size_t j = 0; j < as.size(); j++)
		{	const std::string a = trim(as[j]); if (a.empty()) continue;
			if (isdigit((unsigned char)a[0]) || a[0] == '-' || a[0] == '.') ph.args.push_back(atof(a.c_str())); else ph.argname = a; }
		rest = trim(rest.substr(q + 1)); }
	const std::vector<std::string> ws = split(rest, ' ');
	for (size_t j = 0; j < ws.size(); j++)
	{	const std::string& w = ws[j]; const std::string v = j + 1 < ws.size() ? ws[j + 1] : "";
		if (w == "gain") { ph.gain = atof(v.c_str()); j++; }
		else if (w == "g") { if (v == "max") ph.gmode = 1; else if (v == "sustained") ph.gmode = 2; else { ph.gmode = 3; ph.g = atof(v.c_str()); } j++; }
		else if (w == "throttle") { ph.throttle = atof(v.c_str()); j++; }
		else if (w == "speed") { ph.hold = (v == "hold"); j++; }
		else if (w == "brake") ph.brake = true;
		else { err = "phase " + ph.name + ": unknown option '" + w + "'"; return false; } }
	static const char* aims[] = { "lead", "pure", "lag", "above", "below", "level", "dive", "climb", "away", "into", "right", "left", "guns", "wing", 0 };
	bool ok = false; for (int i = 0; aims[i]; i++) if (ph.aim == aims[i]) ok = true;
	if (!ok) { err = "phase " + ph.name + ": unknown aim '" + ph.aim + "'"; return false; }
	if (!cond.empty() && !(ph.until = acl_expr(cond, err))) return false;
	return true; }

bool AclProgram::parse(const std::string& text)
{	const std::vector<std::string> lines = split(text, '\n'); int ln = 0; int block = 0;	/* 0 none, 1 manoeuvre, 2 doctrine */
	for (size_t li = 0; li < lines.size(); li++)
	{	const std::string& raw = lines[li]; ln++; const std::string line = uncomment(raw); const std::string s = trim(line); if (s.empty()) continue;
		const bool indented = line[0] == ' ' || line[0] == '\t';
		if (!indented)
		{	block = 0;
			if (s.compare(0, 10, "manoeuvre ") == 0)
			{	AclManoeuvre m; m.fail = 0; const size_t p = s.find('('); m.name = trim(s.substr(10, p == std::string::npos ? std::string::npos : p - 10));
				mans.push_back(m); block = 1; }
			else if (s.compare(0, 9, "doctrine ") == 0)
			{	std::string nm = trim(s.substr(9)); if (!nm.empty() && nm[nm.size() - 1] == ':') nm = trim(nm.substr(0, nm.size() - 1));
				doctrines.push_back(std::make_pair(nm, std::vector<AclRule>())); block = 2; }
			continue; }	/* scenario, era, use, aircraft, pilot, run, expect: the game supplies those */
		if (block == 1)
		{	AclManoeuvre& m = mans.back();
			if (s.compare(0, 6, "phase ") == 0)
			{	AclPhase ph; ph.line = ln; std::string err;
				if (!parse_phase(s.substr(6), ph, err)) { error = lineerr(ln, err); return false; }
				m.phases.push_back(ph); }
			else if (s.compare(0, 5, "fail:") == 0) { std::string err; if (!(m.fail = acl_expr(s.substr(5), err))) { error = err; return false; } } }
		else if (block == 2)
		{	AclRule r; r.cond = 0; r.line = ln; std::string err;
			if (s.compare(0, 10, "otherwise:") == 0) { if (!parse_do(s.substr(10), r)) { error = lineerr(ln, "expected 'do m(target)'"); return false; } }
			else if (s.compare(0, 5, "when ") == 0)
			{	const size_t c = s.rfind(": do "); if (c == std::string::npos) { error = lineerr(ln, "expected ': do'"); return false; }
				if (!(r.cond = acl_expr(s.substr(5, c - 5), err))) { error = err; return false; }
				if (!parse_do(s.substr(c + 2), r)) { error = lineerr(ln, "expected 'do m(target)'"); return false; } }
			else continue;
			doctrines.back().second.push_back(r); } }
	for (size_t d = 0; d < doctrines.size(); d++)
		for (size_t i = 0; i < doctrines[d].second.size(); i++)
			if (!find(doctrines[d].second[i].man)) { error = lineerr(doctrines[d].second[i].line, "no manoeuvre " + doctrines[d].second[i].man + " (the runtime flies ACL manoeuvres only)"); return false; }
	return true; }

const AclManoeuvre* AclProgram::find(const std::string& name) const
{	for (size_t i = 0; i < mans.size(); i++) if (mans[i].name == name) return &mans[i]; return 0; }
const std::vector<AclRule>* AclProgram::doctrine(const std::string& name) const
{	for (size_t i = 0; i < doctrines.size(); i++) if (doctrines[i].first == name) return &doctrines[i].second; return 0; }

/* ---- the pilot (engine.py think + UserPrim.aim/control) ------------------------------------------------------------ */
bool AclPilot::init(const AclProgram* p, const std::string& d) { prog = p; rules = p ? p->doctrine(d) : 0; rule = -1; man = 0; return rules != 0; }

static void unit3(double* v) { const double n = norm3(v); if (n > 1e-12) { v[0] /= n; v[1] /= n; v[2] /= n; } }

static bool acl_aim(const AclPhase& ph, const AclBody& me, const AclBody* tb, double aim[3], const AclBody* partner)
{	const double hd = atan2(me.vel[0], me.vel[1]);	/* heading from north (y), x east */
	const double ang = (ph.args.empty() ? 30.0 : ph.args[0]) * 0.01745329;
	const std::string& a = ph.aim;
	if (a == "lead" || a == "pure" || a == "lag")
	{	if (!tb) return false;
		const double r = acl_rng(me, *tb), lt = std::min(r / std::max(norm3(me.vel), 50.0), 3.0);
		const double k = a == "lead" ? lt : a == "lag" ? -0.6 * lt : 0.0;
		for (int i = 0; i < 3; i++) aim[i] = tb->pos[i] + tb->vel[i] * k - me.pos[i];
		unit3(aim); return true; }
	if (a == "wing")	/* fig 5-1: the fighting-wing station -- d m behind my leader, 45 deg off his tail, my side; 1 s ahead */
	{	if (!partner || partner->dead) { if (!tb) return false; for (int i = 0; i < 3; i++) aim[i] = tb->pos[i] - me.pos[i]; unit3(aim); return true; }
		const double d = ph.args.empty() ? 300.0 : ph.args[0];
		double vh[3] = { partner->vel[0], partner->vel[1], partner->vel[2] }; unit3(vh);
		double rt[3] = { vh[1], -vh[0], 0.0 }; unit3(rt);	/* right of his track (x east, y north) */
		const double rel[3] = { me.pos[0] - partner->pos[0], me.pos[1] - partner->pos[1], me.pos[2] - partner->pos[2] };
		const double side = dot3(rel, rt) >= 0.0 ? 1.0 : -1.0;
		for (int i = 0; i < 3; i++) aim[i] = partner->pos[i] - vh[i] * 0.707 * d + rt[i] * side * 0.707 * d + partner->vel[i] - me.pos[i];
		unit3(aim); return true; }
	if (a == "guns")	/* Shaw ch.1 fig 1-1: his position plus his velocity times the bullet time of flight (~880 m/s) */
	{	if (!tb) return false; const double tof = acl_rng(me, *tb) / 880.0;
		for (int i = 0; i < 3; i++) aim[i] = tb->pos[i] + tb->vel[i] * tof - me.pos[i];
		unit3(aim); return true; }
	if (a == "above" || a == "below")
	{	if (!tb) return false; for (int i = 0; i < 3; i++) aim[i] = tb->pos[i] - me.pos[i]; unit3(aim);
		aim[2] += (a == "above" ? 1 : -1) * tan(ang); unit3(aim); return true; }
	if (a == "level") { aim[0] = sin(hd); aim[1] = cos(hd); aim[2] = 0.0; return true; }
	if (a == "right" || a == "left")
	{	const double h2 = hd + (a == "right" ? 1 : -1) * 1.5707963, dv = (ph.args.empty() ? 0.0 : ph.args[0]) * 0.01745329;
		aim[0] = cos(dv) * sin(h2); aim[1] = cos(dv) * cos(h2); aim[2] = -sin(dv); return true; }
	if (a == "dive" || a == "climb")
	{	const double s = a == "dive" ? -1 : 1; aim[0] = cos(ang) * sin(hd); aim[1] = cos(ang) * cos(hd); aim[2] = s * sin(ang); return true; }
	if (a == "away") { if (!tb) return false; for (int i = 0; i < 3; i++) aim[i] = me.pos[i] - tb->pos[i]; unit3(aim); return true; }
	if (a == "into") { if (!tb) return false; for (int i = 0; i < 3; i++) aim[i] = tb->pos[i] - me.pos[i]; unit3(aim); return true; }	/* lift on him: the adapter pulls toward it */
	return false; }

bool AclPilot::step(const AclBody& me, const std::vector<const AclBody*>& enemies, double dt, AclOut& out, const AclBody* partner)
{	if (!rules) return false;
	t += dt; event.clear();
	const AclBody* near = 0; double nr = 1e18;
	for (size_t i = 0; i < enemies.size(); i++) if (!enemies[i]->dead) { const double r = acl_rng(me, *enemies[i]); if (r < nr) { nr = r; near = enemies[i]; } }
	AclCtx c; c.me = &me; c.target = near; c.enemies = &enemies; c.t = t; c.bound = 0; c.partner = partner;
	/* think: rules in priority order; a running manoeuvre is kept unless a rule ABOVE it fires */
	for (int i = 0; i < int(rules->size()); i++)
	{	const AclRule& r = (*rules)[i];
		const bool done = man && phase >= int(man->phases.size());
		if (man && rule == i && !done) break;
		if (r.cond && !truthy(acl_ev(r.cond, c))) continue;
		const AclBody* rt = near;	/* "do m(partner_target)" / "do m(threat)": that enemy, else the nearest */
		if (!r.target.empty() && r.target != "target") { AclNode nm; nm.k = K_NAME; nm.v = 0.0; nm.s = r.target; const AclVal v = acl_ev(&nm, c); rt = v.k == 2 ? v.b : 0; }
		const AclManoeuvre* mm = prog->find(r.man); bool freeaim = mm && !mm->phases.empty();	/* a pull-up needs no enemy */
		for (size_t k = 0; mm && k < mm->phases.size(); k++) { const std::string& a = mm->phases[k].aim; if (a != "level" && a != "dive" && a != "climb" && a != "left" && a != "right") freeaim = false; }
		if (!rt && !freeaim) continue;
		man = mm; rule = i; phase = 0; tp = 0.0; v0 = -1.0; tgt_id = rt ? rt->id : -1;
		{ char b[48]; snprintf(b, sizeof b, "rule line %d -> ", r.line); event = b + r.man; }
		break; }
	tgt = 0;	/* the bodies are rebuilt every step: find this manoeuvre's target again by id */
	for (size_t i = 0; i < enemies.size(); i++) if (enemies[i]->id == tgt_id) tgt = enemies[i];
	if (man && !tgt && tgt_id >= 0) { man = 0; rule = -1; }	/* his target is gone: re-decide next step */
	if (!man || phase >= int(man->phases.size())) return false;
	if (tgt && tgt->dead) { man = 0; rule = -1; return false; }
	tp += dt;
	const AclPhase& ph = man->phases[phase];
	if (!acl_aim(ph, me, tgt, out.aim, c.partner)) return false;
	out.nmax = ph.gmode == 1 ? 99.0 : ph.gmode == 2 ? me.sustained_n : ph.gmode == 3 ? ph.g : 99.0;
	out.force = ph.gmode == 1 && ang3(me.vel, out.aim) * 57.29578 > 15.0;	/* fig 2-7: "g max" forced only while the nose is well off */
	out.throttle = ph.throttle; out.brake = ph.brake; out.nose = ph.aim == "guns";
	if (ph.aim == "wing" && c.partner && !c.partner->dead)	/* close on the station, then hold it (engine.py) */
	{	const double d = ph.args.empty() ? 300.0 : ph.args[0]; double vh[3] = { c.partner->vel[0], c.partner->vel[1], c.partner->vel[2] }; unit3(vh);
		double rt[3] = { vh[1], -vh[0], 0.0 }; unit3(rt);
		const double rel[3] = { me.pos[0] - c.partner->pos[0], me.pos[1] - c.partner->pos[1], me.pos[2] - c.partner->pos[2] };
		const double side = dot3(rel, rt) >= 0.0 ? 1.0 : -1.0; double to[3], mv[3] = { me.vel[0], me.vel[1], me.vel[2] }; unit3(mv);
		for (int i = 0; i < 3; i++) to[i] = c.partner->pos[i] - vh[i] * 0.707 * d + rt[i] * side * 0.707 * d - me.pos[i];
		const double ahead = dot3(to, mv); double th = 0.6 + 0.004 * ahead; out.throttle = th < 0 ? 0 : th > 1 ? 1 : th; out.brake = ahead < -150.0; }
	if (ph.hold) { if (v0 < 0.0) v0 = norm3(me.vel); double th = 0.5 + 0.2 * (v0 - norm3(me.vel)); out.throttle = th < 0 ? 0 : th > 1 ? 1 : th; }
	out.label = man->name + "/" + ph.name;
	AclCtx pc = c; pc.t = tp; pc.target = tgt; pc.param = "bandit";
	if (man->fail && truthy(acl_ev(man->fail, pc))) { event = man->name + " failed"; phase = int(man->phases.size()); return true; }
	if (ph.until && truthy(acl_ev(ph.until, pc)))
	{	phase++; tp = 0.0; v0 = -1.0;
		if (phase < int(man->phases.size())) event = man->name + " phase " + man->phases[phase].name; }
	return true; }

/* prim.py lift_for_direction without the feed-forward term: bank (rad, + right) and load factor */
void acl_lift_for_direction(const double vel[3], const double aim[3], double gain, double nmax, double& mu, double& n)
{	const double G0 = 9.80665; double vh[3] = { vel[0], vel[1], vel[2] }; const double V = std::max(norm3(vel), 30.0); unit3(vh);
	double up[3], right[3];	/* model.py frame(): right = vh x z (horizontal), up = right x vh */
	{	const double z[3] = { 0, 0, 1 }; right[0] = vh[1] * z[2] - vh[2] * z[1]; right[1] = vh[2] * z[0] - vh[0] * z[2]; right[2] = vh[0] * z[1] - vh[1] * z[0];
		if (norm3(right) < 1e-9) { right[0] = 1; right[1] = 0; right[2] = 0; } unit3(right);
		up[0] = right[1] * vh[2] - right[2] * vh[1]; up[1] = right[2] * vh[0] - right[0] * vh[2]; up[2] = right[0] * vh[1] - right[1] * vh[0]; }
	const double da = dot3(aim, vh); double err[3] = { aim[0] - vh[0] * da, aim[1] - vh[1] * da, aim[2] - vh[2] * da };
	const double a = ang3(vel, aim); double turn[3] = { 0, 0, 0 };
	if (norm3(err) > 1e-9) { unit3(err); for (int i = 0; i < 3; i++) turn[i] = err[i] * gain * a * V; }
	double gp[3] = { 0.0 - vh[0] * (-G0 * vh[2]), 0.0 - vh[1] * (-G0 * vh[2]), -G0 - vh[2] * (-G0 * vh[2]) };	/* gravity's normal part */
	double lift[3] = { turn[0] - gp[0], turn[1] - gp[1], turn[2] - gp[2] };
	n = norm3(lift) / G0;
	if (n > nmax)
	{	double g1[3] = { -gp[0], -gp[1], -gp[2] }; const double lim = nmax * G0;
		const double A = dot3(turn, turn), B = 2.0 * dot3(g1, turn), C = dot3(g1, g1) - lim * lim;
		if (C >= 0.0 || A < 1e-12) { if (norm3(g1) > 1e-9) { for (int i = 0; i < 3; i++) lift[i] = g1[i]; unit3(lift); for (int i = 0; i < 3; i++) lift[i] *= lim; } }
		else { double sc = (-B + sqrt(std::max(0.0, B * B - 4.0 * A * C))) / (2.0 * A); sc = sc < 0 ? 0 : sc > 1 ? 1 : sc; for (int i = 0; i < 3; i++) lift[i] = g1[i] + turn[i] * sc; }
		n = nmax; }
	mu = atan2(dot3(lift, right), dot3(lift, up)); }

#ifdef ACL_TEST
/* tools/acl_cpp_test.sh: parse an ACL file, print its doctrines/manoeuvres, and step one pilot of a named doctrine
   against a fixed head-on bogey (metres, z up) -- a smoke test of parse, rules, phases and aims outside the game. */
#include <fstream>
#include <sstream>
#include <string>
int main(int argc, char** argv)
{	if (argc < 3) { fprintf(stderr, "usage: %s file.acl doctrine\n", argv[0]); return 2; }
	std::ifstream f(argv[1]); std::stringstream ss; ss << f.rdbuf();
	AclProgram p; if (!p.parse(ss.str())) { printf("PARSE ERROR %s\n", p.error.c_str()); return 1; }
	printf("manoeuvres %d doctrines %d\n", int(p.mans.size()), int(p.doctrines.size()));
	AclPilot pl; if (!pl.init(&p, argv[2])) { printf("no doctrine %s\n", argv[2]); return 1; }
	if (argc > 3)	/* replay: ~/acl/tools/dump_states.py rows -- does the C++ pilot pick the Python engine's phases? */
	{	std::ifstream st(argv[3]); std::string line; int rows = 0, same = 0; std::string last;
		while (std::getline(st, line))
		{	std::istringstream in(line); double t; AclBody me; int n; in >> t; for (int i = 0; i < 3; i++) in >> me.pos[i]; for (int i = 0; i < 3; i++) in >> me.vel[i];
			me.dead = false; me.sustained_n = 2.5; me.id = 1; me.target_id = -1; in >> n;
			std::vector<AclBody> eb(n); std::vector<const AclBody*> en;
			for (int k = 0; k < n; k++) { for (int i = 0; i < 3; i++) in >> eb[k].pos[i]; for (int i = 0; i < 3; i++) in >> eb[k].vel[i]; eb[k].dead = false; eb[k].sustained_n = 2.5; eb[k].id = 100 + k; eb[k].target_id = 1; }
			for (int k = 0; k < n; k++) en.push_back(&eb[k]);
			std::string lab; in >> lab; AclOut o;
			const std::string mine = pl.step(me, en, 0.02, o) ? o.label : std::string("-");
			rows++; if (mine == lab) same++;
			if (mine != last) { printf("t=%.2f C++ %s (python %s)\n", t, mine.c_str(), lab.c_str()); last = mine; } }
		printf("replay: %d of %d steps on the Python engine's manoeuvre/phase\n", same, rows); return 0; }
	AclBody me = { { 0, 0, 5000 }, { 0, 230, 0 }, false, 2.5, 1, 2 }, him = { { 300, 3000, 5000 }, { 0, -230, 0 }, false, 2.5, 2, 1 };
	std::vector<const AclBody*> en(1, &him); AclOut o;
	for (int k = 0; k < 400; k++)
	{	if (!pl.step(me, en, 0.05, o)) { printf("t=%.2f nothing to fly\n", pl.t); break; }
		double mu, n; acl_lift_for_direction(me.vel, o.aim, 1.6, o.nmax, mu, n);
		if (!pl.event.empty() || k % 80 == 0) printf("t=%.2f %-28s aim=(%.2f %.2f %.2f) mu=%.0f n=%.2f %s\n", pl.t, o.label.c_str(), o.aim[0], o.aim[1], o.aim[2], mu * 57.3, n, pl.event.c_str());
		for (int i = 0; i < 3; i++) { me.pos[i] += me.vel[i] * 0.05; him.pos[i] += him.vel[i] * 0.05; } }
	return 0; }
#endif
