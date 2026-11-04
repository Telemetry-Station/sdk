#!/usr/bin/env python3
"""Fail-closed release decision from independent approved requirements and evidence.
This validates records, not the truth of measurements or regulatory approval.
"""
import argparse,hashlib,json,math,re,sys
from pathlib import Path

def evaluate(policy,record,root):
    reasons=[]
    for k in ['board_serial','board_revision','station_id','operator','timestamp_utc']:
        if not isinstance(record.get(k),str) or not record[k].strip(): reasons.append('missing '+k)
    if not policy.get('approved_by') or not policy.get('approval_id'): reasons.append('policy not approved')
    expected=policy.get('image_sha256','')
    if not re.fullmatch('[0-9a-f]{64}',expected) or record.get('verified_image_sha256')!=expected: reasons.append('image not independently verified')
    if not policy.get('required_tests'): reasons.append('empty required test plan')
    required=policy.get('required_tests',[])
    tests=record.get('tests',[]);ids=[t.get('id') for t in tests]
    if len(ids)!=len(set(ids)): reasons.append('duplicate test ids')
    for spec in required:
        matches=[t for t in tests if t.get('id')==spec.get('id')]
        if len(matches)!=1: reasons.append('missing '+str(spec.get('id')));continue
        t=matches[0]
        if t.get('status')!='PASS': reasons.append(str(spec['id'])+' not PASS')
        if 'minimum' in spec or 'maximum' in spec:
            value=t.get('value');lo=spec.get('minimum');hi=spec.get('maximum')
            if any(type(v) not in [int,float] or not math.isfinite(v) for v in [value,lo,hi]) or not lo<=value<=hi or t.get('unit')!=spec.get('unit'): reasons.append(str(spec['id'])+' limits/unit invalid')
        try:
            evidence=(root/t['evidence']).resolve()
            if not evidence.is_relative_to(root.resolve()) or hashlib.sha256(evidence.read_bytes()).hexdigest()!=t['evidence_sha256']: raise ValueError()
        except (KeyError,OSError,ValueError,TypeError): reasons.append(str(spec['id'])+' evidence missing/mismatched')
    if record.get('final_state')!=policy.get('required_final_state') or not policy.get('required_final_state'): reasons.append('final state unverified')
    return {'decision':'HOLD' if reasons else 'RECORD_ACCEPTED','reasons':reasons,'scope':'Evidence record consistency only; manufacturing release approval is separate'}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('policy',type=Path);p.add_argument('record',type=Path);p.add_argument('--evidence-root',type=Path,required=True);a=p.parse_args()
    try: r=evaluate(json.loads(a.policy.read_text()),json.loads(a.record.read_text()),a.evidence_root)
    except (ValueError,TypeError,KeyError,OSError) as e:r={'decision':'HOLD','reasons':[str(e)]}
    print(json.dumps(r,indent=2));sys.exit(r['decision']!='RECORD_ACCEPTED')
