%GNUPLOT Metapost output: Mon Nov  2 22:41:18 2009


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
draw (193.2a,88.0b)--(209.8a,88.0b);
draw (820.8a,88.0b)--(804.2a,88.0b);
put_text( btex  1 etex, 172.0a, 88.0b, 0, 3);
draw (193.2a,236.6b)--(209.8a,236.6b);
draw (820.8a,236.6b)--(804.2a,236.6b);
put_text( btex  2 etex, 172.0a, 236.6b, 0, 3);
draw (193.2a,385.2b)--(209.8a,385.2b);
draw (820.8a,385.2b)--(804.2a,385.2b);
put_text( btex  4 etex, 172.0a, 385.2b, 0, 3);
draw (193.2a,533.8b)--(209.8a,533.8b);
draw (820.8a,533.8b)--(804.2a,533.8b);
put_text( btex  8 etex, 172.0a, 533.8b, 0, 3);
draw (193.2a,88.0b)--(193.2a,104.6b);
draw (193.2a,581.6b)--(193.2a,565.0b);
put_text( btex  1 etex, 193.2a, 44.0b, 0, 2);
draw (382.1a,88.0b)--(382.1a,104.6b);
draw (382.1a,581.6b)--(382.1a,565.0b);
put_text( btex  2 etex, 382.1a, 44.0b, 0, 2);
draw (571.1a,88.0b)--(571.1a,104.6b);
draw (571.1a,581.6b)--(571.1a,565.0b);
put_text( btex  4 etex, 571.1a, 44.0b, 0, 2);
draw (760.0a,88.0b)--(760.0a,104.6b);
draw (760.0a,581.6b)--(760.0a,565.0b);
put_text( btex  8 etex, 760.0a, 44.0b, 0, 2);
draw (193.2a,581.6b)--(193.2a,88.0b)--(820.8a,88.0b)--(820.8a,581.6b)--(193.2a,581.6b);
currentcolor:=0white;
put_text( btex Speedup etex, 86.4a, 334.8b, 90, 2);
linetype -2;
linetype 0;
linetype -2;
put_text( btex saxpy etex, 362.8a, 543.0b, 0, 3);
linetype 0;
draw (384.0a,543.0b)--(485.4a,543.0b);
draw (193.2a,88.0b)--(382.1a,205.5b)--(571.1a,225.6b)--(760.0a,233.3b);
gpdraw(4,193.2a,88.0b);
gpdraw(4,382.1a,205.5b);
gpdraw(4,571.1a,225.6b);
gpdraw(4,760.0a,233.3b);
gpdraw(4,434.7a,543.0b);
linetype 1;
linetype -2;
put_text( btex sgemm etex, 362.8a, 499.0b, 0, 3);
linetype 1;
draw (384.0a,499.0b)--(485.4a,499.0b);
draw (193.2a,88.0b)--(382.1a,225.6b)--(571.1a,345.9b)--(760.0a,483.6b);
gpdraw(1,193.2a,88.0b);
gpdraw(1,382.1a,225.6b);
gpdraw(1,571.1a,345.9b);
gpdraw(1,760.0a,483.6b);
gpdraw(1,434.7a,499.0b);
linetype 2;
linetype -2;
put_text( btex conv2d etex, 362.8a, 455.0b, 0, 3);
linetype 2;
draw (384.0a,455.0b)--(485.4a,455.0b);
draw (193.2a,88.0b)--(382.1a,208.0b)--(571.1a,356.6b)--(760.0a,514.1b);
gpdraw(2,193.2a,88.0b);
gpdraw(2,382.1a,208.0b);
gpdraw(2,571.1a,356.6b);
gpdraw(2,760.0a,514.1b);
gpdraw(2,434.7a,455.0b);
linetype 3;
linetype -2;
put_text( btex dotprod etex, 362.8a, 411.0b, 0, 3);
linetype 3;
draw (384.0a,411.0b)--(485.4a,411.0b);
draw (193.2a,88.0b)--(382.1a,200.5b)--(571.1a,214.0b)--(760.0a,229.0b);
gpdraw(3,193.2a,88.0b);
gpdraw(3,382.1a,200.5b);
gpdraw(3,571.1a,214.0b);
gpdraw(3,760.0a,229.0b);
gpdraw(3,434.7a,411.0b);
linetype -2;
draw (193.2a,581.6b)--(193.2a,88.0b)--(820.8a,88.0b)--(820.8a,581.6b)--(193.2a,581.6b);
endfig;
end.
