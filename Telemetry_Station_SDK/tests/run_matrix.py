#!/usr/bin/env python3
"""Run every MT/GNSS/IMEI-on-ready host combination under sanitizers with warnings as errors.

--target also builds every MT/GNSS order for the MSP430F169 with TI cl430 (console on), plus a
headless (console off) image; build_f169.py gates RAM, flash and worst-case stack for each."""
import argparse, itertools, json, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--target',action='store_true');p.add_argument('--output-parent',type=Path,default=Path(tempfile.gettempdir()));a=p.parse_args()
a.output_parent.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix='ts-matrix-',dir=a.output_parent));results=[]
def target_build(name,**fields):
    o=json.loads((ROOT/'orders/bench.json').read_text());o.update(order_id=name,topic_id=1234,**fields)
    order=out/(name+'.json');order.write_text(json.dumps(o))
    return [sys.executable,str(ROOT/'tools/build_f169.py'),'--order',str(order),'--output-parent',str(out/'target')]
variants=[]
for mt,gnss,imei in itertools.product([False,True],repeat=3):
    name=f'mt{int(mt)}-gnss{int(gnss)}-imei{int(imei)}';b=out/name
    commands=[['cmake','-S',str(ROOT),'-B',str(b),'-DTS_WERROR=ON','-DTS_SANITIZE=ON','-DTS_ENABLE_MT='+('ON' if mt else 'OFF'),'-DTS_ENABLE_GNSS='+('ON' if gnss else 'OFF'),'-DTS_QUERY_IMEI_ON_READY='+('ON' if imei else 'OFF')],['cmake','--build',str(b),'-j8'],['ctest','--test-dir',str(b),'-j8','--output-on-failure','--output-junit',str(b/'tests.xml')]]
    if a.target and imei: commands.append(target_build(name,receive_mt=mt,gnss=gnss,console=True))
    variants.append((name,commands))
if a.target: variants.append(('target-headless',[target_build('headless',receive_mt=True,gnss=False,console=False)]))
for name,commands in variants:
    ok=True
    with (out/(name+'.log')).open('w') as log:
        for cmd in commands:
            result=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
            if result.returncode: ok=False;break
    results.append({'variant':name,'status':'PASS' if ok else 'FAIL','log':name+'.log'})
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(out);print(json.dumps(results));sys.exit(any(r['status']!='PASS' for r in results))
