import subprocess, sys
ROI=(350,200,210,100)   # x, y, w, h of the AC display area, lights-off framing
def capture(path):
    subprocess.run(["ffmpeg","-hide_banner","-loglevel","error","-f","avfoundation","-framerate","30","-video_size","1280x720","-i","0:none","-frames:v","1","-y",path],check=True,timeout=20)
def green_count(path):
    x,y,w,h=ROI
    raw=subprocess.run(["ffmpeg","-hide_banner","-loglevel","error","-i",path,"-vf",f"crop={w}:{h}:{x}:{y}","-f","rawvideo","-pix_fmt","rgb24","-"],capture_output=True,check=True,timeout=20).stdout
    n=0
    for i in range(0,len(raw)-2,3):
        r,g,b=raw[i],raw[i+1],raw[i+2]
        if g>90 and g>r+30 and g>b+30: n+=1
    return n
if __name__=="__main__":
    p=sys.argv[1] if len(sys.argv)>1 else "cam_now.jpg"
    if len(sys.argv)<2: capture(p)
    print("green pixels in display ROI:", green_count(p))
