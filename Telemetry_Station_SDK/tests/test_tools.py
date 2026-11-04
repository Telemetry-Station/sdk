import copy,hashlib,importlib.util,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def module(name):
 s=importlib.util.spec_from_file_location(name,ROOT/'tools'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
build=module('build_f169');production=module('production_record')
class Tests(unittest.TestCase):
 def test_order(self):
  import json
  o=json.loads((ROOT/'orders/bench.json').read_text());build.validate(o)
  for key,value in [('receive_mt',1),('topic_id',12),('schema_id','bad'),('line_capacity',255),('order_id','../x')]:
   bad=copy.deepcopy(o);bad[key]=value
   with self.assertRaises(ValueError):build.validate(bad)
 def test_production(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);(root/'measurement').write_text('real evidence fixture');h=hashlib.sha256((root/'measurement').read_bytes()).hexdigest()
   p={'approved_by':'fixture','approval_id':'test','image_sha256':'a'*64,'required_final_state':'off','required_tests':[{'id':'rail','minimum':3.5,'maximum':4.5,'unit':'V'}]}
   r={k:'fixture' for k in ['board_serial','board_revision','station_id','operator','timestamp_utc']};r.update(verified_image_sha256='a'*64,final_state='off',tests=[{'id':'rail','status':'PASS','value':4.0,'unit':'V','evidence':'measurement','evidence_sha256':h}])
   self.assertEqual(production.evaluate(p,r,root)['decision'],'RECORD_ACCEPTED')
   for k,v in [('status','NOT RUN'),('value',4.6),('value',float('nan')),('unit','mV'),('evidence','../missing'),('evidence_sha256','b'*64)]:
    bad=copy.deepcopy(r);bad['tests'][0][k]=v;self.assertEqual(production.evaluate(p,bad,root)['decision'],'HOLD')
   r['tests']=[];self.assertEqual(production.evaluate(p,r,root)['decision'],'HOLD')
if __name__=='__main__':unittest.main()
