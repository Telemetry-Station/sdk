#!/usr/bin/env python3
"""Build a frozen-order F169 bench image, never flash. Requires TI MSP430 CGT."""
import argparse, hashlib, json, os, re, subprocess, sys, tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def validate(order):
    expected = {'order_id','topic_id','receive_mt','gnss','console','watchdog','payload_max','line_capacity','report_interval_ms','schema_id'}
    if set(order) != expected: raise ValueError('Order keys differ from the documented schema')
    if not isinstance(order['order_id'],str) or not re.fullmatch(r'[a-z0-9][a-z0-9_-]{0,47}',order['order_id']): raise ValueError('Invalid order_id')
    for k in ['receive_mt','gnss','console','watchdog']:
        if type(order[k]) is not bool: raise ValueError(k+' must be a JSON boolean')
    for k,lo,hi in [('topic_id',0,65535),('payload_max',4,1024),('line_capacity',256,4096),('report_interval_ms',1,2147483647)]:
        if type(order[k]) is not int or not lo<=order[k]<=hi: raise ValueError(k+' outside limits')
    if 0 < order['topic_id'] < 64: raise ValueError('Topic must be zero (disabled) or >=64')
    if 4*((order['payload_max']+4)//3)+160 > order['line_capacity']: raise ValueError('Line too small for payload')
    schema=json.loads((ROOT/'examples/common/generated/schema.json').read_text())
    if order['schema_id'] != schema['schemaId']: raise ValueError('Order/codec schema mismatch')
    return order

def header(o):
    defs={'TS_TOPIC_ID':o['topic_id'],'TS_ENABLE_MT':int(o['receive_mt']),'TS_ENABLE_GNSS':int(o['gnss']),
          'TS_ENABLE_CONSOLE':int(o['console']),'TS_F169_WATCHDOG':int(o['watchdog']),
          'TS_MAX_PAYLOAD':o['payload_max'],'TS_LINE_CAP':o['line_capacity'],'REPORT_INTERVAL_MS':o['report_interval_ms']}
    return '#ifndef TS_COMPILED_ORDER_H\n#define TS_COMPILED_ORDER_H\n'+''.join('#define %s %su\n'%kv for kv in defs.items())+'#endif\n'

STACK_RESERVED=512;STACK_MARGIN=64
def stack_worst_case(xml_text):
    """Static worst case from the ofd430 call graph; callbacks are assumed to reach any port/main function."""
    funcs={}
    for f in ET.fromstring(xml_text).iter('function'):
        n=f.findtext('name')
        if n: funcs[n]=(int(f.findtext('stack_size'),16),[c.text for c in f.findall('callee') if c.text],f.findtext('makes_indirect_call')=='true',f.findtext('file') or '')
    isrs=[n for n in funcs if n.endswith('_isr')]
    targets=[n for n,v in funcs.items() if v[3].endswith(('ts_f169_port.c','msp430f169/main.c')) and n!='main' and n not in isrs]
    def depth(n,path):
        if n in path: raise ValueError('Recursion in call graph: '+' > '.join(path+(n,)))
        if n not in funcs: return 0,[n]
        size,callees,indirect,_=funcs[n]
        if indirect and n!='main' and not n.startswith('__TI_'): callees=callees+targets
        best=(0,[])
        for c in callees:
            v=depth(c,path+(n,))
            if v[0]>best[0]: best=v
        return size+best[0],[n]+best[1]
    if 'main' not in funcs or not isrs: raise ValueError('Call graph is missing main or ISRs')
    main_bytes,main_path=depth('main',())
    isr_bytes,isr_path=max(depth(n,()) for n in isrs)
    worst=main_bytes+isr_bytes+4
    return {'worst_case':worst,'main':main_bytes,'main_path':' > '.join(main_path),'isr':isr_bytes+4,'isr_path':' > '.join(isr_path),'headroom':STACK_RESERVED-worst}

def portable(text,out,ccs):
    """Replace machine-specific paths so published manifests do not reveal local directories."""
    for path,name in [(out,'<OUTPUT>'),(ccs,'<CCS_ROOT>'),(ROOT,'<SDK>')]: text=str(text).replace(str(path),name)
    return text.replace(os.sep,'/') if os.name=='nt' else text

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--order',type=Path,default=ROOT/'orders/bench.json');p.add_argument('--ccs-root',type=Path,default=os.getenv('CCS_ROOT'));p.add_argument('--output-parent',type=Path,default=ROOT/'build');a=p.parse_args()
    o=validate(json.loads(a.order.read_text()))
    candidates=[a.ccs_root] if a.ccs_root else [*Path.home().glob('ti/ccs*/ccs'),*Path('/Applications/ti').glob('ccs*/ccs'),*Path('C:/ti').glob('ccs*/ccs')]
    ccs=next((x.resolve() for x in candidates if x and (x/'tools/compiler').is_dir()),None)
    if not ccs: raise ValueError('Set CCS_ROOT to a CCS installation with MSP430 CGT')
    exe='cl430.exe' if os.name=='nt' else 'cl430'
    compilers=sorted((ccs/'tools/compiler').glob('ti-cgt-msp430_*/bin/'+exe))
    if not compilers: raise ValueError('MSP430 CGT not installed')
    compiler=compilers[-1];cgt=compiler.parent.parent
    a.output_parent.mkdir(parents=True,exist_ok=True)
    out=Path(tempfile.mkdtemp(prefix=o['order_id']+'-',dir=a.output_parent.resolve()))
    (out/'compiled_order.h').write_text(header(o));(out/'order.json').write_text(json.dumps(o,indent=2)+'\n')
    flags=['-vmsp','--abi=eabi','--use_hw_mpy=16','--c99','--opt_level=2','--printf_support=minimal','-g','--display_error_number','--diag_warning=225','--emit_warnings_as_errors','--define=__MSP430F169__=1','--preinclude='+str(out/'compiled_order.h')]
    flags+=['--include_path='+str(x) for x in [ROOT/'include',ROOT/'ports/msp430f169',ROOT/'examples/common',ccs/'ccs_base/msp430/include',cgt/'include']]
    sources=sorted((ROOT/'src').glob('*.c'))+sorted((ROOT/'ports/msp430f169').glob('*.c'))+[ROOT/'examples/common/app_payload.c',ROOT/'examples/common/bear_checkout.c',ROOT/'examples/common/generated/telemetry_payload.c',ROOT/'examples/msp430f169/main.c']
    image=out/'firmware.out';mapfile=out/'firmware.map'
    linker=ccs/'ccs_base/msp430/include/lnk_msp430f169.cmd'
    with (out/'build.log').open('w') as log:
        def run(cmd):
            result=subprocess.run([str(v) for v in cmd],cwd=out,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            log.write(result.stdout);log.flush()
            if result.returncode: raise RuntimeError(str(out)+'\n'+result.stdout)
        objs=[]
        for i,src in enumerate(sources):
            obj=out/(str(i)+'.obj');run([compiler,*flags,'--compile_only','--output_file='+str(obj),src]);objs.append(obj)
        run([compiler,*flags,'--run_linker','--rom_model','--heap_size=0','--stack_size=%d'%STACK_RESERVED,'--search_path='+str(cgt/'lib'),'--search_path='+str(linker.parent),'--output_file='+str(image),'--map_file='+str(mapfile),'--xml_link_info='+str(out/'link.xml'),*objs,linker,'--library=libc.a'])
    txt=mapfile.read_text();memory={}
    if not re.search(r'ENTRY POINT SYMBOL: "_c_int00',txt) or not re.search(r'RESET\s+0000fffe\s+00000002\s+00000002',txt): raise ValueError('Missing startup/reset vector')
    for region in ['RAM','FLASH']:
        m=re.search(r'^\s*'+region+r'\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)',txt,re.M)
        if not m: raise ValueError('Missing memory region '+region)
        memory[region]={k:int(m[i],16) for k,i in [('capacity',2),('used',3),('free',4)]}
    ofd=compiler.parent/('ofd430.exe' if os.name=='nt' else 'ofd430')
    graph=subprocess.run([str(ofd),'--call_graph',str(image)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if graph.returncode: raise RuntimeError(graph.stdout)
    (out/'call_graph.xml').write_text(graph.stdout);stack=stack_worst_case(graph.stdout)
    if stack['headroom']<STACK_MARGIN: raise ValueError('Stack worst case %d B leaves %d B of %d B (need %d): %s'%(stack['worst_case'],stack['headroom'],STACK_RESERVED,STACK_MARGIN,stack['main_path']))
    inputs=sorted(set(sources+list((ROOT/'include').glob('*.h'))+list((ROOT/'ports/msp430f169').glob('*.h'))+list((ROOT/'examples/common').rglob('*.h'))+[ROOT/'tools/build_f169.py',ROOT/'examples/common/generated/schema.json']))
    manifest={'status':'PASS','scope':'compile/link only; bench example, not production release','order':o,'order_sha256':digest(out/'order.json'),'compiler':'<CCS_ROOT>/'+compiler.relative_to(ccs).as_posix(),'compiler_sha256':digest(compiler),'linker_sha256':digest(linker),'flags':[portable(f,out,ccs) for f in flags],'memory':memory,'stack_reserved':STACK_RESERVED,'stack_static':stack,'stack_high_water':'NOT RUN','image_sha256':digest(image),'source_sha256':{str(x.relative_to(ROOT)):digest(x) for x in inputs}}
    (out/'build_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({'output':str(out),'memory':memory,'stack':{k:stack[k] for k in ('worst_case','headroom')}}))
if __name__=='__main__':
    try: main()
    except (ValueError,OSError,RuntimeError) as e: sys.exit(str(e))
