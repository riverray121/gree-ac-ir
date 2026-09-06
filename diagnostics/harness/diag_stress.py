# Network load on the blaster: a UDP packet stream the ESP's lwIP stack has to
# receive and drop, plus repeated TCP connects to its API port. Interrupt and
# task activity during a transmission is what disturbs software-timed pulses.
import socket,sys,time,threading
HOST=sys.argv[1]; SECS=float(sys.argv[2]); PPS=int(sys.argv[3]) if len(sys.argv)>3 else 1500
end=time.time()+SECS
def udp():
    s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); pkt=b'x'*512; n=0
    while time.time()<end:
        s.sendto(pkt,(HOST,9)); n+=1; time.sleep(1.0/PPS)
    print("udp packets",n)
def tcp():
    n=0
    while time.time()<end:
        try:
            c=socket.create_connection((HOST,6053),timeout=1); c.close(); n+=1
        except OSError: pass
        time.sleep(0.05)
    print("tcp connects",n)
t=[threading.Thread(target=udp),threading.Thread(target=tcp)]; [x.start() for x in t]; [x.join() for x in t]
