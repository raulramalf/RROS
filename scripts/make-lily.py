import math
C=(256,286)
def tf(pts,ang,mir):
    a=math.radians(ang); o=[]
    for x,y in pts:
        if mir: y=-y
        o.append((C[0]+x*math.cos(a)-y*math.sin(a), C[1]+x*math.sin(a)+y*math.cos(a)))
    return o
def petal(ang,mir,L=1.0):
    # pétalo estrecho que se arquea y se riza hacia atrás en la punta (como el del lirio araña)
    P=[(0,-4),(60,-30),(118,-26),(150,-18),(168,-34),(152,-50),(150,-36),(140,-30),(112,-14),(56,-10),(0,4)]
    p=tf([(x*L,y*1.55+6) for x,y in P],ang,mir)
    f=lambda q:f'{q[0]:.1f},{q[1]:.1f}'
    return (f'M{f(p[0])} C{f(p[1])} {f(p[2])} {f(p[3])} C{f(p[4])} {f(p[5])} {f(p[6])} '
            f'C{f(p[7])} {f(p[8])} {f(p[9])} L{f(p[10])}Z')
def stamen(ang,L):
    a=math.radians(ang)
    x2,y2=C[0]+L*math.cos(a), C[1]+L*math.sin(a)-30
    side=1 if math.cos(a)>0 else -1
    cx,cy=C[0]+L*0.35*math.cos(a)-side*10, C[1]-L*0.75
    return f'M{C[0]},{C[1]} Q{cx:.1f},{cy:.1f} {x2:.1f},{y2:.1f}',(x2,y2)
pet=''
for ang in (-176,-146,-116): pet+=f'<path d="{petal(ang,False)}"/>'
for ang in (-4,-34,-64): pet+=f'<path d="{petal(ang,True)}"/>'
st='';dots=''
for ang,L in ((-160,190),(-140,215),(-118,225),(-62,225),(-40,215),(-20,190)):
    d,(x,y)=stamen(ang,L); st+=f'<path d="{d}"/>'; dots+=f'<circle cx="{x:.1f}" cy="{y:.1f}" r="7"/>'
svg=f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512" width="512" height="512">
<title>Lily</title>
<defs><linearGradient id="r" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ff4458"/><stop offset="1" stop-color="#c0142e"/></linearGradient></defs>
<g fill="none" stroke="#e02a41" stroke-width="5.5" stroke-linecap="round">{st}</g>
<g fill="#ff4a5e">{dots}</g>
<g fill="url(#r)">{pet}</g>
<circle cx="{C[0]}" cy="{C[1]}" r="9" fill="#a80f26"/>
<path d="M256,292 C250,350 262,410 256,468" stroke="#2f7a3a" stroke-width="10" fill="none" stroke-linecap="round"/>
</svg>'''
open('lily.svg','w').write(svg)
