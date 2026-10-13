#!/usr/bin/env python3
"""The app-smoke check (roadmap M2, in the gate): sadx-app ran the guest for N frames and wrote a screenshot.

Checks the log (every frame line present, nothing left undrawn, the guest never stopped) and the picture:
640x480, not uniform, and — at frame 120 of the pinned profile's boot — the "PRESENTED BY SEGA" screen: a white
background (> 80 % of pixels) with the blue SEGA logo (> 2,000 blue pixels). These are properties of the frame
the game issued, not a pixel-exact golden (Metal filtering may differ between GPUs).
Usage: check_app_smoke.py SCREENSHOT.ppm APP_LOG"""
import re,sys
def main(ppm,log):
    text=open(log).read()
    frames=[int(m) for m in re.findall(r'^frame (\d+):',text,re.M)]
    if not frames or frames!=list(range(1,len(frames)+1)):sys.exit(f'app-smoke: frame lines missing or out of order ({len(frames)})')
    if 'not drawn' in text:sys.exit('app-smoke: some draws were not drawn: '+re.search(r'not drawn:[^\n]*',text).group(0))
    if 'guest stopped' in text:sys.exit('app-smoke: the guest stopped: '+re.search(r'guest stopped:[^\n]*',text).group(0))
    d=open(ppm,'rb').read();parts=d.split(b'\n',3);w,h=map(int,parts[1].split());px=parts[3]
    if (w,h)!=(640,480):sys.exit(f'app-smoke: screenshot is {w}x{h}, not 640x480')
    white=blue=0;colours=set()
    for i in range(0,len(px),3):
        r,g,b=px[i],px[i+1],px[i+2];colours.add((r,g,b))
        if r>240 and g>240 and b>240:white+=1
        elif b>r+60 and b>120:blue+=1
    if len(colours)<2:sys.exit('app-smoke: the screenshot is one colour')
    if len(frames)==120 and (white<0.8*w*h or blue<2000):sys.exit(f'app-smoke: frame 120 is not the SEGA screen (white {white}, blue {blue})')
    print(f'app-smoke: {len(frames)} guest frames drawn; frame {len(frames)}: {len(colours)} colours, white {white}, blue {blue}')
if __name__=='__main__':main(*sys.argv[1:3])
