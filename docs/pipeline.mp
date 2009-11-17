%GNUPLOT Metapost output: Sat Nov 14 13:20:52 2009


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
draw (235.6a,88.0b)--(252.2a,88.0b);
draw (820.8a,88.0b)--(804.2a,88.0b);
put_text( btex  0 etex, 214.4a, 88.0b, 0, 3);
draw (235.6a,149.7b)--(252.2a,149.7b);
draw (820.8a,149.7b)--(804.2a,149.7b);
put_text( btex  50 etex, 214.4a, 149.7b, 0, 3);
draw (235.6a,211.4b)--(252.2a,211.4b);
draw (820.8a,211.4b)--(804.2a,211.4b);
put_text( btex  100 etex, 214.4a, 211.4b, 0, 3);
draw (235.6a,273.1b)--(252.2a,273.1b);
draw (820.8a,273.1b)--(804.2a,273.1b);
put_text( btex  150 etex, 214.4a, 273.1b, 0, 3);
draw (235.6a,334.8b)--(252.2a,334.8b);
draw (820.8a,334.8b)--(804.2a,334.8b);
put_text( btex  200 etex, 214.4a, 334.8b, 0, 3);
draw (235.6a,396.5b)--(252.2a,396.5b);
draw (820.8a,396.5b)--(804.2a,396.5b);
put_text( btex  250 etex, 214.4a, 396.5b, 0, 3);
draw (235.6a,458.2b)--(252.2a,458.2b);
draw (820.8a,458.2b)--(804.2a,458.2b);
put_text( btex  300 etex, 214.4a, 458.2b, 0, 3);
draw (235.6a,519.9b)--(252.2a,519.9b);
draw (820.8a,519.9b)--(804.2a,519.9b);
put_text( btex  350 etex, 214.4a, 519.9b, 0, 3);
draw (235.6a,581.6b)--(252.2a,581.6b);
draw (820.8a,581.6b)--(804.2a,581.6b);
put_text( btex  400 etex, 214.4a, 581.6b, 0, 3);
draw (235.6a,88.0b)--(235.6a,104.6b);
draw (235.6a,581.6b)--(235.6a,565.0b);
put_text( btex  1 etex, 235.6a, 44.0b, 0, 2);
draw (319.2a,88.0b)--(319.2a,104.6b);
draw (319.2a,581.6b)--(319.2a,565.0b);
put_text( btex  2 etex, 319.2a, 44.0b, 0, 2);
draw (402.8a,88.0b)--(402.8a,104.6b);
draw (402.8a,581.6b)--(402.8a,565.0b);
put_text( btex  3 etex, 402.8a, 44.0b, 0, 2);
draw (486.4a,88.0b)--(486.4a,104.6b);
draw (486.4a,581.6b)--(486.4a,565.0b);
put_text( btex  4 etex, 486.4a, 44.0b, 0, 2);
draw (570.0a,88.0b)--(570.0a,104.6b);
draw (570.0a,581.6b)--(570.0a,565.0b);
put_text( btex  5 etex, 570.0a, 44.0b, 0, 2);
draw (653.6a,88.0b)--(653.6a,104.6b);
draw (653.6a,581.6b)--(653.6a,565.0b);
put_text( btex  6 etex, 653.6a, 44.0b, 0, 2);
draw (737.2a,88.0b)--(737.2a,104.6b);
draw (737.2a,581.6b)--(737.2a,565.0b);
put_text( btex  7 etex, 737.2a, 44.0b, 0, 2);
draw (820.8a,88.0b)--(820.8a,104.6b);
draw (820.8a,581.6b)--(820.8a,565.0b);
put_text( btex  8 etex, 820.8a, 44.0b, 0, 2);
draw (235.6a,581.6b)--(235.6a,88.0b)--(820.8a,88.0b)--(820.8a,581.6b)--(235.6a,581.6b);
currentcolor:=0white;
put_text( btex average time per output etex, 86.4a, 334.8b, 90, 2);
linetype -2;
linetype 0;
linetype -2;
put_text( btex 5usec etex, 655.8a, 543.0b, 0, 3);
linetype 0;
draw (677.0a,543.0b)--(778.4a,543.0b);
draw (235.6a,112.8b)--(319.2a,103.6b)--(402.8a,99.5b)--(486.4a,98.1b)--(570.0a,95.6b)
--(653.6a,94.5b)--(737.2a,94.3b)--(820.8a,93.1b);
gpdraw(4,235.6a,112.8b);
gpdraw(4,319.2a,103.6b);
gpdraw(4,402.8a,99.5b);
gpdraw(4,486.4a,98.1b);
gpdraw(4,570.0a,95.6b);
gpdraw(4,653.6a,94.5b);
gpdraw(4,737.2a,94.3b);
gpdraw(4,820.8a,93.1b);
gpdraw(4,727.7a,543.0b);
linetype 1;
linetype -2;
put_text( btex 20usec etex, 655.8a, 499.0b, 0, 3);
linetype 1;
draw (677.0a,499.0b)--(778.4a,499.0b);
draw (235.6a,187.9b)--(319.2a,152.8b)--(402.8a,133.8b)--(486.4a,123.4b)--(570.0a,115.8b)
--(653.6a,111.4b)--(737.2a,109.6b)--(820.8a,106.1b);
gpdraw(3,235.6a,187.9b);
gpdraw(3,319.2a,152.8b);
gpdraw(3,402.8a,133.8b);
gpdraw(3,486.4a,123.4b);
gpdraw(3,570.0a,115.8b);
gpdraw(3,653.6a,111.4b);
gpdraw(3,737.2a,109.6b);
gpdraw(3,820.8a,106.1b);
gpdraw(3,727.7a,499.0b);
linetype 2;
linetype -2;
put_text( btex 50usec etex, 655.8a, 455.0b, 0, 3);
linetype 2;
draw (677.0a,455.0b)--(778.4a,455.0b);
draw (235.6a,334.7b)--(319.2a,215.1b)--(402.8a,174.1b)--(486.4a,150.9b)--(570.0a,151.6b)
--(653.6a,132.1b)--(737.2a,132.2b)--(820.8a,128.6b);
gpdraw(2,235.6a,334.7b);
gpdraw(2,319.2a,215.1b);
gpdraw(2,402.8a,174.1b);
gpdraw(2,486.4a,150.9b);
gpdraw(2,570.0a,151.6b);
gpdraw(2,653.6a,132.1b);
gpdraw(2,737.2a,132.2b);
gpdraw(2,820.8a,128.6b);
gpdraw(2,727.7a,455.0b);
linetype 3;
linetype -2;
put_text( btex 100usec etex, 655.8a, 411.0b, 0, 3);
linetype 3;
draw (677.0a,411.0b)--(778.4a,411.0b);
draw (235.6a,581.1b)--(319.2a,336.1b)--(402.8a,254.8b)--(486.4a,212.8b)--(570.0a,189.9b)
--(653.6a,174.1b)--(737.2a,163.9b)--(820.8a,155.0b);
gpdraw(1,235.6a,581.1b);
gpdraw(1,319.2a,336.1b);
gpdraw(1,402.8a,254.8b);
gpdraw(1,486.4a,212.8b);
gpdraw(1,570.0a,189.9b);
gpdraw(1,653.6a,174.1b);
gpdraw(1,737.2a,163.9b);
gpdraw(1,820.8a,155.0b);
gpdraw(1,727.7a,411.0b);
linetype -2;
draw (235.6a,581.6b)--(235.6a,88.0b)--(820.8a,88.0b)--(820.8a,581.6b)--(235.6a,581.6b);
endfig;
end.
