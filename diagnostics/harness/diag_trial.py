import json,os,re,sys,time,subprocess,statistics as st,urllib.request,csv
from diag_cam import capture,green_count
from diag_irdecode import dumps_from_log
TOK=[l.split('=',1)[1].strip() for l in open(os.path.expanduser('~/.config/home-brain/secrets.env')) if l.startswith('HA_TOKEN=')][0]
HA="http://192.168.8.2:8123"; ENT="climate.ram_s_bedroom_ac"; TX="sensor.rams_bedroom_ram_s_bedroom_ac_ir_tx_log"
LOG="session_rx.log"; GREEN_ON=40
def ha(path,body=None):
    r=urllib.request.Request(HA+path,data=json.dumps(body).encode() if body else None,headers={"Authorization":"Bearer "+TOK,"Content-Type":"application/json"})
    return json.load(urllib.request.urlopen(r,timeout=15))
def txn():
    m=re.match(r'#(\d+)',ha(f"/api/states/{TX}")['state']); return int(m.group(1)) if m else 0
def decode(raw):
    bits=[1 if -raw[i+1]>=1000 else 0 for i in range(2,len(raw)-1,2) if -raw[i+1]<3000]
    if len(bits)<67: return None
    def byte(b): return sum(bit<<i for i,bit in enumerate(b))
    cmd=[byte(bits[i*8:i*8+8]) for i in range(4)]; foot=bits[32:35]; data=[byte(bits[35+i*8:35+i*8+8]) for i in range(4)]
    blk=cmd+data
    chk=(10+sum(x&0xF for x in blk[:4])+sum(x>>4 for x in blk[4:7]))&0xF
    marks=[raw[i] for i in range(2,len(raw)-1,2) if -raw[i+1]<3000]; sp=[-raw[i+1] for i in range(2,len(raw)-1,2) if -raw[i+1]<3000]
    s0=[s for s in sp if s<1000]; s1=[s for s in sp if s>=1000]
    return dict(hex=''.join(f'{x:02X}' for x in blk),foot=''.join(map(str,foot)),chk_ok=(blk[7]>>4)==chk,power=(blk[0]>>3)&1,mode=blk[0]&7,temp=16+(blk[1]&0xF),
                mark=round(st.mean(marks)),mark_sd=round(st.pstdev(marks)),mark_min=min(marks),mark_max=max(marks),s0=round(st.mean(s0)),s0_min=min(s0),s0_max=max(s0),s1=round(st.mean(s1)) if s1 else 0,s1_min=min(s1) if s1 else 0,s1_max=max(s1) if s1 else 0,hdr=f"{raw[0]}/{-raw[1]}")
def new_dumps(offset):
    txt=open(LOG).read()[offset:]
    tmp="_slice.log"; open(tmp,'w').write(txt); return [d for d in dumps_from_log(tmp) if len(d)>100]
def run(n,out):
    w=csv.writer(open(out,'a')); 
    if os.path.getsize(out)==0: w.writerow(["trial","utc","cmd","expect_on","tx_ok","halves","chk_all","rx_power","rx_mode","rx_temp","green","display_on","success","mark","mark_sd","mark_min","mark_max","s0","s0_min","s0_max","s1","s1_min","s1_max","hdr","hex"])
    seq=[("off",None),("cool",20),("off",None),("cool",24),("off",None),("cool",22)]
    for t in range(n):
        mode,temp=seq[t%len(seq)]; expect_on=mode!="off"
        before=txn(); off=os.path.getsize(LOG); ts=time.strftime("%H:%M:%S",time.gmtime())
        if mode=="off": ha("/api/services/climate/set_hvac_mode",{"entity_id":ENT,"hvac_mode":"off"})
        else: ha("/api/services/climate/set_temperature",{"entity_id":ENT,"hvac_mode":"cool","temperature":temp})
        time.sleep(4.5)
        img=f"shots/t{t:03d}.jpg"; os.makedirs("shots",exist_ok=True)
        g=-1
        for attempt in range(3):
            try: capture(img); g=green_count(img); break
            except Exception: time.sleep(1)
        after=txn(); dumps=new_dumps(off); dec=[decode(d) for d in dumps]; dec=[d for d in dec if d]
        first=dec[0] if dec else {}
        display_on=g>=GREEN_ON; success=(display_on==expect_on)
        row=[t,ts,f"{mode}{'' if temp is None else temp}",expect_on,after-before,len(dumps),all(d['chk_ok'] for d in dec) if dec else False,first.get('power'),first.get('mode'),first.get('temp'),g,display_on,success]+[first.get(k) for k in ("mark","mark_sd","mark_min","mark_max","s0","s0_min","s0_max","s1","s1_min","s1_max","hdr","hex")]
        w.writerow(row); print(*row[:13],sep=' | ',flush=True)
        time.sleep(1.5)
if __name__=="__main__": run(int(sys.argv[1]),sys.argv[2])
