import re,sys,statistics as st
def dumps_from_log(path):
    txt=open(path).read().splitlines(); out=[]; cur=None
    for l in txt:
        m=re.search(r'\[remote\.raw:\d+\]: (.*)$',l)
        if not m: continue
        body=m.group(1)
        if body.startswith('Received Raw:'):
            if cur: out.append(cur)
            cur=[]; body=body[len('Received Raw:'):]
        if cur is None: continue
        cur+= [int(v) for v in re.findall(r'-?\d+',body)]
    if cur: out.append(cur)
    return out
def pronto_to_us(s):
    w=[int(x,16) for x in s.split()]; unit=1e6/(w[1]*0.241246) ; unit=1/unit*1e6
    vals=w[4:]; out=[]
    for i,v in enumerate(vals): us=round(v*unit); out.append(us if i%2==0 else -us)
    return out
def analyze(raw,label):
    marks=[abs(v) for v in raw[2::2] if 0<abs(v)<3000]
    sp=[abs(v) for v in raw[3::2] if abs(v)<3000]
    s0=[v for v in sp if v<1000]; s1=[v for v in sp if v>=1000]
    bits=''.join('1' if abs(v)>=1000 else '0' for v in raw[3::2] if abs(v)<3000)
    gaps=[abs(v) for v in raw[1::2] if abs(v)>=3000]
    print(f"{label}: header {raw[0]}/{raw[1]} | mark mean {st.mean(marks):.0f} sd {st.pstdev(marks):.0f} min {min(marks)} max {max(marks)} | space0 mean {st.mean(s0):.0f} (min {min(s0)} max {max(s0)}) | space1 mean {st.mean(s1):.0f} (min {min(s1)} max {max(s1)}) | gaps {gaps} | bits {len(bits)}")
    return bits
if __name__=='__main__':
    d=dumps_from_log(sys.argv[1]); print(len(d),'dumps, lengths',[len(x) for x in d])
    d=[x for x in d if len(x)>50]; bits=[analyze(x,f"dump{i}") for i,x in enumerate(d)]
    for i,b in enumerate(bits): print(f" bits{i}: {b}")
