import csv,sys,statistics as st
from diag_irdecode import dumps_from_log
from trial import decode
f=sys.argv[1]; rows=list(csv.DictReader(open(f)))
ok=sum(1 for r in rows if r['success']=='True')
print(f"{f}: {len(rows)} trials, {ok} success, {len(rows)-ok} fail | tx missing {sum(1 for r in rows if r['tx_ok']!='1')} | halves!=4 {sum(1 for r in rows if r['halves']!='4')} | trials with a corrupted half {sum(1 for r in rows if r['chk_all']!='True')}")
for r in rows:
    if r['success']!='True': print('   FAIL',r['trial'],r['utc'],r['cmd'],'halves',r['halves'],'chk',r['chk_all'],'rx_power',r['rx_power'],'green',r['green'])
m=[int(r['mark']) for r in rows if r['mark']]; s0=[int(r['s0']) for r in rows if r['s0']]; s1=[int(r['s1']) for r in rows if r['s1']]
mn=[int(r['mark_min']) for r in rows if r['mark_min']]; s1n=[int(r['s1_min']) for r in rows if r['s1_min']]
print(f"   timing: mark {st.mean(m):.0f} s0 {st.mean(s0):.0f} s1 {st.mean(s1):.0f} | worst mark_min {min(mn)} worst s1_min {min(s1n)}")
