#!/usr/bin/env python3
"""Line and branch coverage for the SDK core, the BEAR checkout and the MSP430F169 port/firmware.

Two configurations are built (IMEI requested by the driver at READY, and by the application) and
every object is run through gcov. Counts are merged per source path, so files compiled into several
tests or configurations (or #included, like the firmware main.c) are judged on their combined coverage.
Branches that cannot be reached on the host are listed with a reason in branch_waivers.json.
Never delete caller directories or hide zero-hit files."""
import argparse,json,os,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCES=sorted(str(x.relative_to(ROOT)) for x in (ROOT/'src').glob('*.c'))+['examples/common/bear_checkout.c','examples/msp430f169/main.c','ports/msp430f169/ts_f169_console.c','ports/msp430f169/ts_f169_port.c']
p=argparse.ArgumentParser();p.add_argument('--minimum',type=float,default=float(os.getenv('TS_COVERAGE_MIN','100')));p.add_argument('--branch-minimum',type=float,default=float(os.getenv('TS_BRANCH_COVERAGE_MIN','100')));p.add_argument('cmake_args',nargs='*');a=p.parse_args()
if not (0<=a.minimum<=100 and 0<=a.branch_minimum<=100): sys.exit('minimums must be 0..100')
waivers=json.loads((ROOT/'tests/branch_waivers.json').read_text())
CONFIGS=[[],['-DTS_QUERY_IMEI_ON_READY=OFF']]
out=Path(tempfile.mkdtemp(prefix='ts-coverage-'));work=out/'gcov';work.mkdir();builds=[out/('build%d'%i) for i in range(len(CONFIGS))]
def run(cmd,cwd=None): return subprocess.run(cmd,cwd=cwd,check=True,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT).stdout
def parse(path,lines,branches):
    """Merge one .gcov file: lines[n]=hits, branches[(n,i)]=taken (both summed)."""
    current=None;index=0
    for line in path.read_text(errors='replace').splitlines():
        if line.startswith('branch '):
            parts=line.split()
            if current is not None:
                taken=int(parts[3]) if parts[2]=='taken' and parts[3].isdigit() else 0
                branches[(current,index)]=branches.get((current,index),0)+taken;index+=1
            continue
        fields=line.split(':',2)
        if len(fields)!=3 or not fields[1].strip().isdigit(): continue
        number=int(fields[1]);count=fields[0].strip().rstrip('*')
        if number==0 or count=='-': current=None;continue
        current=number;index=0
        lines[number]=lines.get(number,0)+(int(count) if count.isdigit() else 0)
try:
    log=''
    for b,config in zip(builds,CONFIGS):
        log+=run(['cmake','-S',str(ROOT),'-B',str(b),'-DTS_COVERAGE=ON','-DTS_ENABLE_GNSS=ON',*config,*a.cmake_args]);log+=run(['cmake','--build',str(b),'-j8']);log+=run(['ctest','--test-dir',str(b),'-j8','--output-on-failure'])
    (out/'tests.log').write_text(log)
    compiler=''.join(x.read_text() for x in builds[0].glob('CMakeFiles/*/CMakeCCompiler.cmake'))
    gcov=(['xcrun','llvm-cov','gcov'] if shutil.which('xcrun') else ['llvm-cov','gcov']) if 'Clang' in compiler else ['gcov']
    merged={s:({},{}) for s in SOURCES}
    for i,note in enumerate(sorted(n for b in builds for n in b.rglob('*.gcno'))):
        d=work/str(i);d.mkdir();run([*gcov,'-b','-c','-p',str(note)],d)
        for g in d.glob('*.gcov'):
            first=g.read_text(errors='replace').split('\n',1)[0]
            src=first.split('Source:',1)[1].strip() if 'Source:' in first else ''
            src=os.path.relpath(Path(src).resolve(),ROOT) if src else ''
            if src in merged: parse(g,*merged[src])
    rows=[]
    for src,(lines,branches) in merged.items():
        if not lines: raise ValueError('Missing instrumentation: '+src)
        waived={int(k):v for k,v in waivers.get(src,{}).items()}
        missed=sorted(n for n,h in lines.items() if h==0)
        untaken=sorted({n for (n,_),t in branches.items() if t==0})
        open_branches=[n for n in untaken if n not in waived]
        stale=sorted(n for n in waived if n not in untaken)
        counted=[k for k in branches if k[0] not in waived or branches[k]>0]
        taken=sum(1 for k in counted if branches[k]>0)
        rows.append({'source':src,'lines':len(lines),'hit':len(lines)-len(missed),'percent':100*(len(lines)-len(missed))/len(lines),'missed':missed,
                     'branches':len(counted),'taken':taken,'branch_percent':100*taken/len(counted) if counted else 100.0,'untaken_lines':open_branches,'waived_lines':sorted(waived),'stale_waivers':stale})
    ok=all(r['percent']>=a.minimum and r['branch_percent']>=a.branch_minimum and not r['stale_waivers'] for r in rows)
    result={'scope':'host line+branch coverage, GNSS+MT enabled, IMEI-on-ready on and off; core, checkout, F169 port, console and firmware main; excludes generated codec','minimum':a.minimum,'branch_minimum':a.branch_minimum,'files':rows,'status':'PASS' if ok else 'FAIL'}
    (out/'coverage.json').write_text(json.dumps(result,indent=2)+'\n');print(out)
    for r in rows: print('%-6s %-38s lines %5.1f%% (%d)  branches %5.1f%% (%d)%s%s'%('PASS' if r['percent']>=a.minimum and r['branch_percent']>=a.branch_minimum and not r['stale_waivers'] else 'FAIL',r['source'],r['percent'],r['lines'],r['branch_percent'],r['branches'],'  missed '+str(r['missed']) if r['missed'] else '','  untaken '+str(r['untaken_lines'])+(' stale '+str(r['stale_waivers']) if r['stale_waivers'] else '') if r['untaken_lines'] or r['stale_waivers'] else ''))
    print('COVERAGE',result['status']);sys.exit(result['status']!='PASS')
except (subprocess.CalledProcessError,ValueError) as e:
    print(getattr(e,'stdout',str(e)));print(out);sys.exit(1)
