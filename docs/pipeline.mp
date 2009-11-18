%GNUPLOT Metapost output: Wed Nov 18 17:10:16 2009


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
draw (235.6a,140.8b)--(252.2a,140.8b);
draw (820.8a,140.8b)--(804.2a,140.8b);
put_text( btex  0 etex, 214.4a, 140.8b, 0, 3);
draw (235.6a,195.9b)--(252.2a,195.9b);
draw (820.8a,195.9b)--(804.2a,195.9b);
put_text( btex  50 etex, 214.4a, 195.9b, 0, 3);
draw (235.6a,251.0b)--(252.2a,251.0b);
draw (820.8a,251.0b)--(804.2a,251.0b);
put_text( btex  100 etex, 214.4a, 251.0b, 0, 3);
draw (235.6a,306.1b)--(252.2a,306.1b);
draw (820.8a,306.1b)--(804.2a,306.1b);
put_text( btex  150 etex, 214.4a, 306.1b, 0, 3);
draw (235.6a,361.2b)--(252.2a,361.2b);
draw (820.8a,361.2b)--(804.2a,361.2b);
put_text( btex  200 etex, 214.4a, 361.2b, 0, 3);
draw (235.6a,416.3b)--(252.2a,416.3b);
draw (820.8a,416.3b)--(804.2a,416.3b);
put_text( btex  250 etex, 214.4a, 416.3b, 0, 3);
draw (235.6a,471.4b)--(252.2a,471.4b);
draw (820.8a,471.4b)--(804.2a,471.4b);
put_text( btex  300 etex, 214.4a, 471.4b, 0, 3);
draw (235.6a,526.5b)--(252.2a,526.5b);
draw (820.8a,526.5b)--(804.2a,526.5b);
put_text( btex  350 etex, 214.4a, 526.5b, 0, 3);
draw (235.6a,581.6b)--(252.2a,581.6b);
draw (820.8a,581.6b)--(804.2a,581.6b);
put_text( btex  400 etex, 214.4a, 581.6b, 0, 3);
draw (235.6a,140.8b)--(235.6a,157.4b);
draw (235.6a,581.6b)--(235.6a,565.0b);
put_text( btex  1 etex, 235.6a, 96.8b, 0, 2);
draw (319.2a,140.8b)--(319.2a,157.4b);
draw (319.2a,581.6b)--(319.2a,565.0b);
put_text( btex  2 etex, 319.2a, 96.8b, 0, 2);
draw (402.8a,140.8b)--(402.8a,157.4b);
draw (402.8a,581.6b)--(402.8a,565.0b);
put_text( btex  3 etex, 402.8a, 96.8b, 0, 2);
draw (486.4a,140.8b)--(486.4a,157.4b);
draw (486.4a,581.6b)--(486.4a,565.0b);
put_text( btex  4 etex, 486.4a, 96.8b, 0, 2);
draw (570.0a,140.8b)--(570.0a,157.4b);
draw (570.0a,581.6b)--(570.0a,565.0b);
put_text( btex  5 etex, 570.0a, 96.8b, 0, 2);
draw (653.6a,140.8b)--(653.6a,157.4b);
draw (653.6a,581.6b)--(653.6a,565.0b);
put_text( btex  6 etex, 653.6a, 96.8b, 0, 2);
draw (737.2a,140.8b)--(737.2a,157.4b);
draw (737.2a,581.6b)--(737.2a,565.0b);
put_text( btex  7 etex, 737.2a, 96.8b, 0, 2);
draw (820.8a,140.8b)--(820.8a,157.4b);
draw (820.8a,581.6b)--(820.8a,565.0b);
put_text( btex  8 etex, 820.8a, 96.8b, 0, 2);
draw (235.6a,581.6b)--(235.6a,140.8b)--(820.8a,140.8b)--(820.8a,581.6b)--(235.6a,581.6b);
currentcolor:=0white;
put_text( btex average time per output etex, 86.4a, 361.2b, 90, 2);
linetype -2;
currentcolor:=0white;
put_text( btex The number of cores etex, 528.2a, 30.8b, 0, 2);
linetype -2;
linetype 0;
linetype -2;
put_text( btex 5usec etex, 655.8a, 543.0b, 0, 3);
linetype 0;
draw (677.0a,543.0b)--(778.4a,543.0b);
draw (235.6a,163.0b)--(319.2a,154.7b)--(402.8a,151.1b)--(486.4a,149.8b)--(570.0a,147.6b)
--(653.6a,146.6b)--(737.2a,146.4b)--(820.8a,145.4b);
gpdraw(4,235.6a,163.0b);
gpdraw(4,319.2a,154.7b);
gpdraw(4,402.8a,151.1b);
gpdraw(4,486.4a,149.8b);
gpdraw(4,570.0a,147.6b);
gpdraw(4,653.6a,146.6b);
gpdraw(4,737.2a,146.4b);
gpdraw(4,820.8a,145.4b);
gpdraw(4,727.7a,543.0b);
linetype 1;
linetype -2;
put_text( btex 20usec etex, 655.8a, 499.0b, 0, 3);
linetype 1;
draw (677.0a,499.0b)--(778.4a,499.0b);
draw (235.6a,230.1b)--(319.2a,198.7b)--(402.8a,181.7b)--(486.4a,172.4b)--(570.0a,165.6b)
--(653.6a,161.7b)--(737.2a,160.0b)--(820.8a,156.9b);
gpdraw(3,235.6a,230.1b);
gpdraw(3,319.2a,198.7b);
gpdraw(3,402.8a,181.7b);
gpdraw(3,486.4a,172.4b);
gpdraw(3,570.0a,165.6b);
gpdraw(3,653.6a,161.7b);
gpdraw(3,737.2a,160.0b);
gpdraw(3,820.8a,156.9b);
gpdraw(3,727.7a,499.0b);
linetype 2;
linetype -2;
put_text( btex 50usec etex, 655.8a, 455.0b, 0, 3);
linetype 2;
draw (677.0a,455.0b)--(778.4a,455.0b);
draw (235.6a,361.1b)--(319.2a,254.3b)--(402.8a,217.7b)--(486.4a,196.9b)--(570.0a,197.6b)
--(653.6a,180.2b)--(737.2a,180.3b)--(820.8a,177.1b);
gpdraw(2,235.6a,361.1b);
gpdraw(2,319.2a,254.3b);
gpdraw(2,402.8a,217.7b);
gpdraw(2,486.4a,196.9b);
gpdraw(2,570.0a,197.6b);
gpdraw(2,653.6a,180.2b);
gpdraw(2,737.2a,180.3b);
gpdraw(2,820.8a,177.1b);
gpdraw(2,727.7a,455.0b);
linetype 3;
linetype -2;
put_text( btex 100usec etex, 655.8a, 411.0b, 0, 3);
linetype 3;
draw (677.0a,411.0b)--(778.4a,411.0b);
draw (235.6a,581.1b)--(319.2a,362.3b)--(402.8a,289.8b)--(486.4a,252.2b)--(570.0a,231.8b)
--(653.6a,217.7b)--(737.2a,208.6b)--(820.8a,200.6b);
gpdraw(1,235.6a,581.1b);
gpdraw(1,319.2a,362.3b);
gpdraw(1,402.8a,289.8b);
gpdraw(1,486.4a,252.2b);
gpdraw(1,570.0a,231.8b);
gpdraw(1,653.6a,217.7b);
gpdraw(1,737.2a,208.6b);
gpdraw(1,820.8a,200.6b);
gpdraw(1,727.7a,411.0b);
linetype -2;
draw (235.6a,581.6b)--(235.6a,140.8b)--(820.8a,140.8b)--(820.8a,581.6b)--(235.6a,581.6b);
endfig;
end.
