#!/usr/bin/env python3
import sys, re
REG={'eax':'a','ax':'a','al':'a','ebx':'b','bx':'b','bl':'b','ecx':'c','cx':'c','cl':'c',
     'edx':'d','dx':'d','dl':'d','esi':'S','si':'S','edi':'D','di':'D'}
CALLER={'a':'eax','c':'ecx','d':'edx'}
pat=re.compile(r'(inline\s+([\w:]+\s*\*?)\s+(\w+)\s*\(([^)]*)\)\s*\{)([^{}]*?)_asm\s*\{?([^{}]*?)\}([^{}]*?\})', re.S)
def parmtypes(params):
    # name -> 'ref' (Type&) | 'val' (everything else)
    d={}
    for p in params.split(','):
        p=p.strip()
        if not p: continue
        mm=re.match(r'(.+?)([A-Za-z_]\w*)\s*$', p)
        if mm:
            typ, nm = mm.group(1), mm.group(2)
            d[nm] = 'ref' if '&' in typ else 'val'
    return d
def conv(f):
    s=open(f,encoding='latin-1').read()
    def rep(m):
        head,ret,name,params,pre,body,tail=m.groups()
        cm=re.search(r'call\s+(XASM_\w+)',body)
        if not cm: return m.group(0)
        call=cm.group(1); pt=parmtypes(params)
        inputs=[]
        for mm in re.finditer(r'(?:movzx|mov)\s+(\w+),(\w+)',body):
            r,v=mm.group(1),mm.group(2)
            if r in REG and v not in ('esp','ss') and v not in REG:
                expr = f'&{v}' if pt.get(v)=='ref' else f'(long)({v})'
                inputs.append((REG[r], expr))
        after=body[cm.end():]
        extra='; mov %%edx, %%eax' if re.search(r'mov\s+eax,edx',after) else ''
        ret=ret.strip()
        used=set(c for c,_ in inputs)
        if ret!='void': used.add('a')
        clob=[f'"{CALLER[k]}"' for k in CALLER if k not in used]+['"cc"','"memory"']
        inlist=','.join(f'"{c}"({e})' for c,e in inputs)
        asmstr=f'__asm__ __volatile__("call {call}{extra}"'
        if ret=='void':
            return head+pre+f'{asmstr} : : {inlist} : {",".join(clob)});'+tail
        return head+pre+f'{ret} _r; {asmstr} : "=a"(_r) : {inlist} : {",".join(clob)}); return ({ret})_r;'+'\n}'
    s2,n=pat.subn(rep,s)
    open(f,'w',encoding='latin-1').write(s2); return n
for f in sys.argv[1:]: print(f, "converted:", conv(f))
