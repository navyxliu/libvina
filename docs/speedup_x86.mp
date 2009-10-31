%GNUPLOT Metapost output: Thu Oct 29 02:41:53 2009


warningcheck:=0;
defaultmpt:=mpt:=4;
th:=.6;
%% Have nice sharp joins on our lines
linecap:=butt;
linejoin:=mitered;

def scalepen expr n = pickup pencircle scaled (n*th) enddef;
def ptsize expr n = mpt:=n*defaultmpt enddef;


textmag:= 1.000;
vardef makepic(expr str) =
  if picture str : str scaled textmag
  % otherwise a string
  else: str infont defaultfont scaled (defaultscale*textmag)
  fi
enddef;

def infontsize(expr str, size) =
  infont str scaled (size / fontsize str)
enddef;

%font changes
verbatimtex
\def\setfont#1#2{%.
  \font\gpfont=#1 at #2pt
\gpfont}
\setfont{cmtt12}{12.00}
etex

color currentcolor; currentcolor:=black;
color fillcolor;
boolean colorlines,dashedlines;
colorlines:=false;
dashedlines:=true;

def _wc = withpen currentpen withcolor currentcolor enddef;
def _ac = addto currentpicture enddef;
def _sms = scaled mpt shifted enddef;
% drawing point-types
def gpdraw (expr n, x, y) =
  if n<0: _ac contour fullcircle _sms (x,y)
  elseif (n=1) or (n=3):
    _ac doublepath ptpath[n] _sms (x,y) _wc;
    _ac doublepath ptpath[n] rotated 90 _sms (x,y) _wc
  elseif n<6: _ac doublepath ptpath[n] _sms (x,y) _wc
  else: _ac contour ptpath[n] _sms (x,y) _wc
  fi
enddef;

% the point shapes
path ptpath[];
%diamond
ptpath0 = ptpath6 = (-1/2,0)--(0,-1/2)--(1/2,0)--(0,1/2)--cycle;
% plus sign
ptpath1 = (-1/2,0)--(1/2,0);
% square
ptpath2 = ptpath7 = (-1/2,-1/2)--(1/2,-1/2)--(1/2,1/2)--(-1/2,1/2)--cycle;
% cross
ptpath3 := (-1/2,-1/2)--(1/2,1/2);
% circle:
ptpath4 = ptpath8:= fullcircle;
% triangle
ptpath5 = ptpath9 := (0,1/2)--(-1/2,-1/2)--(1/2,-1/2)--cycle;

def linetype expr n =
  currentcolor:= if colorlines : col[n] else: black fi;
  if n = -1 :
      drawoptions(withcolor currentcolor withpen (currentpen scaled .5));
  elseif n < 1 :
    drawoptions(_wc);
  else :
    drawoptions( if dashedlines: dashed lt[n] fi _wc);
  fi
enddef;

% dash patterns
picture lt[];
lt1=dashpattern(on 2 off 2); % dashes
lt2=dashpattern(on 2 off 2 on 0.2 off 2); %dash-dot
lt3=lt1 scaled 1.414;
lt4=lt2 scaled 1.414;
lt5=lt1 scaled 2;
lt6:=lt2 scaled 2;
lt7=dashpattern(on 0.2 off 2); %dots

color col[],cyan, magenta, yellow;
cyan=blue+green; magenta=red+blue;yellow=green+red;
col[-2]:=col[-1]:=col0:=black;
col1:=red;
col2:=(.2,.2,1); %blue
col3:=(1,.66,0); %orange
col4:=.85*green;
col5:=.9*magenta;
col6:=0.85*cyan;
col7:=.85*yellow;

%placing text
picture GPtext;
def put_text(expr pic, x, y, r, j) =
  GPtext:=makepic(pic);
  GPtext:=GPtext shifted
    if j = 1: (-(ulcorner GPtext + llcorner GPtext)/2)
    elseif j = 2: (-center GPtext)
    else: (-(urcorner GPtext + lrcorner GPtext)/2)
    fi
    rotated r;
  draw GPtext shifted (x,y)
enddef;

beginfig(0);
w:=5.000in;h:=3.000in;
a:=w/1200.0;b:=h/720.0;
scalepen 1; ptsize 1.000;linetype -2;
draw (193.2a,140.8b)--(209.8a,140.8b);
draw (724.8a,140.8b)--(708.2a,140.8b);
put_text( btex  1 etex, 172.0a, 140.8b, 0, 3);
draw (193.2a,257.9b)--(209.8a,257.9b);
draw (724.8a,257.9b)--(708.2a,257.9b);
put_text( btex  2 etex, 172.0a, 257.9b, 0, 3);
draw (193.2a,374.9b)--(209.8a,374.9b);
draw (724.8a,374.9b)--(708.2a,374.9b);
put_text( btex  4 etex, 172.0a, 374.9b, 0, 3);
draw (193.2a,492.0b)--(209.8a,492.0b);
draw (724.8a,492.0b)--(708.2a,492.0b);
put_text( btex  8 etex, 172.0a, 492.0b, 0, 3);
draw (193.2a,140.8b)--(193.2a,157.4b);
draw (193.2a,492.0b)--(193.2a,475.4b);
put_text( btex  1 etex, 193.2a, 96.8b, 0, 2);
draw (353.2a,140.8b)--(353.2a,157.4b);
draw (353.2a,492.0b)--(353.2a,475.4b);
put_text( btex  2 etex, 353.2a, 96.8b, 0, 2);
draw (513.3a,140.8b)--(513.3a,157.4b);
draw (513.3a,492.0b)--(513.3a,475.4b);
put_text( btex  4 etex, 513.3a, 96.8b, 0, 2);
draw (673.3a,140.8b)--(673.3a,157.4b);
draw (673.3a,492.0b)--(673.3a,475.4b);
put_text( btex  8 etex, 673.3a, 96.8b, 0, 2);
draw (193.2a,492.0b)--(193.2a,140.8b)--(724.8a,140.8b)--(724.8a,492.0b)--(193.2a,492.0b);
currentcolor:=0white;
put_text( btex Speedup etex, 86.4a, 316.4b, 90, 2);
linetype -2;
currentcolor:=0white;
put_text( btex Number of cores etex, 459.0a, 30.8b, 0, 2);
linetype -2;
put_text( btex Speedup on HTN etex, 459.0a, 558.0b, 0, 2);
linetype 0;
linetype -2;
put_text( btex saxpy etex, 362.8a, 453.4b, 0, 3);
linetype 0;
draw (384.0a,453.4b)--(485.4a,453.4b);
draw (193.2a,140.8b)--(353.2a,233.4b)--(513.3a,249.2b)--(673.3a,255.3b);
gpdraw(4,193.2a,140.8b);
gpdraw(4,353.2a,233.4b);
gpdraw(4,513.3a,249.2b);
gpdraw(4,673.3a,255.3b);
gpdraw(4,434.7a,453.4b);
linetype 1;
linetype -2;
put_text( btex mm etex, 362.8a, 409.4b, 0, 3);
linetype 1;
draw (384.0a,409.4b)--(485.4a,409.4b);
draw (193.2a,140.8b)--(353.2a,249.2b)--(513.3a,344.0b)--(673.3a,452.5b);
gpdraw(1,193.2a,140.8b);
gpdraw(1,353.2a,249.2b);
gpdraw(1,513.3a,344.0b);
gpdraw(1,673.3a,452.5b);
gpdraw(1,434.7a,409.4b);
linetype 2;
linetype -2;
put_text( btex conv etex, 362.8a, 365.4b, 0, 3);
linetype 2;
draw (384.0a,365.4b)--(485.4a,365.4b);
draw (193.2a,140.8b)--(353.2a,235.3b)--(513.3a,352.4b)--(673.3a,476.5b);
gpdraw(2,193.2a,140.8b);
gpdraw(2,353.2a,235.3b);
gpdraw(2,513.3a,352.4b);
gpdraw(2,673.3a,476.5b);
gpdraw(2,434.7a,365.4b);
linetype 3;
linetype -2;
put_text( btex dotprod etex, 362.8a, 321.4b, 0, 3);
linetype 3;
draw (384.0a,321.4b)--(485.4a,321.4b);
draw (193.2a,140.8b)--(353.2a,229.4b)--(513.3a,240.1b)--(673.3a,251.8b);
gpdraw(3,193.2a,140.8b);
gpdraw(3,353.2a,229.4b);
gpdraw(3,513.3a,240.1b);
gpdraw(3,673.3a,251.8b);
gpdraw(3,434.7a,321.4b);
linetype -2;
draw (193.2a,492.0b)--(193.2a,140.8b)--(724.8a,140.8b)--(724.8a,492.0b)--(193.2a,492.0b);
endfig;
end.
