%GNUPLOT Metapost output: Sat Nov 14 13:19:57 2009


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
draw (214.4a,220.0b)--(231.0a,220.0b);
draw (820.8a,220.0b)--(804.2a,220.0b);
put_text( btex  0 etex, 193.2a, 220.0b, 0, 3);
draw (214.4a,265.2b)--(231.0a,265.2b);
draw (820.8a,265.2b)--(804.2a,265.2b);
put_text( btex  2 etex, 193.2a, 265.2b, 0, 3);
draw (214.4a,310.4b)--(231.0a,310.4b);
draw (820.8a,310.4b)--(804.2a,310.4b);
put_text( btex  4 etex, 193.2a, 310.4b, 0, 3);
draw (214.4a,355.6b)--(231.0a,355.6b);
draw (820.8a,355.6b)--(804.2a,355.6b);
put_text( btex  6 etex, 193.2a, 355.6b, 0, 3);
draw (214.4a,400.8b)--(231.0a,400.8b);
draw (820.8a,400.8b)--(804.2a,400.8b);
put_text( btex  8 etex, 193.2a, 400.8b, 0, 3);
draw (214.4a,446.0b)--(231.0a,446.0b);
draw (820.8a,446.0b)--(804.2a,446.0b);
put_text( btex  10 etex, 193.2a, 446.0b, 0, 3);
draw (214.4a,491.2b)--(231.0a,491.2b);
draw (820.8a,491.2b)--(804.2a,491.2b);
put_text( btex  12 etex, 193.2a, 491.2b, 0, 3);
draw (214.4a,536.4b)--(231.0a,536.4b);
draw (820.8a,536.4b)--(804.2a,536.4b);
put_text( btex  14 etex, 193.2a, 536.4b, 0, 3);
draw (214.4a,581.6b)--(231.0a,581.6b);
draw (820.8a,581.6b)--(804.2a,581.6b);
put_text( btex  16 etex, 193.2a, 581.6b, 0, 3);
draw (335.7a,220.0b)--(335.7a,236.6b);
draw (335.7a,581.6b)--(335.7a,565.0b);
put_text( btex saxpy etex, 335.7a, 198.8b, -30, 1);
draw (457.0a,220.0b)--(457.0a,236.6b);
draw (457.0a,581.6b)--(457.0a,565.0b);
put_text( btex sgemm etex, 457.0a, 198.8b, -30, 1);
draw (578.2a,220.0b)--(578.2a,236.6b);
draw (578.2a,581.6b)--(578.2a,565.0b);
put_text( btex conv2d etex, 578.2a, 198.8b, -30, 1);
draw (699.5a,220.0b)--(699.5a,236.6b);
draw (699.5a,581.6b)--(699.5a,565.0b);
put_text( btex dotprod etex, 699.5a, 198.8b, -30, 1);
draw (214.4a,581.6b)--(214.4a,220.0b)--(820.8a,220.0b)--(820.8a,581.6b)--(214.4a,581.6b);
currentcolor:=0white;
put_text( btex Gflops etex, 86.4a, 400.8b, 90, 2);
linetype -2;
linetype 0;
linetype -2;
put_text( btex cpu etex, 655.8a, 543.0b, 0, 3);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (677.0a,532.0b)--(778.4a,532.0b)--(778.4a,554.0b)--(677.0a,554.0b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (677.0a,532.0b)--(778.4a,532.0b)--(778.4a,554.0b)--(677.0a,554.0b)--(677.0a,532.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (297.3a,220.0b)--(333.8a,220.0b)--(333.8a,237.6b)--(297.3a,237.6b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (297.3a,220.0b)--(297.3a,237.5b)--(333.7a,237.5b)--(333.7a,220.0b)--(297.3a,220.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (418.6a,220.0b)--(455.0a,220.0b)--(455.0a,279.8b)--(418.6a,279.8b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (418.6a,220.0b)--(418.6a,279.7b)--(454.9a,279.7b)--(454.9a,220.0b)--(418.6a,220.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (539.8a,220.0b)--(576.3a,220.0b)--(576.3a,225.0b)--(539.8a,225.0b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (539.8a,220.0b)--(539.8a,224.9b)--(576.2a,224.9b)--(576.2a,220.0b)--(539.8a,220.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (661.1a,220.0b)--(697.6a,220.0b)--(697.6a,234.6b)--(661.1a,234.6b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (661.1a,220.0b)--(661.1a,234.5b)--(697.5a,234.5b)--(697.5a,220.0b)--(661.1a,220.0b);
linetype 0;
linetype 1;
linetype -2;
put_text( btex gpu etex, 655.8a, 499.0b, 0, 3);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (677.0a,488.0b)--(778.4a,488.0b)--(778.4a,510.0b)--(677.0a,510.0b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (677.0a,488.0b)--(778.4a,488.0b)--(778.4a,510.0b)--(677.0a,510.0b)--(677.0a,488.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (337.7a,220.0b)--(374.2a,220.0b)--(374.2a,256.0b)--(337.7a,256.0b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (337.7a,220.0b)--(337.7a,255.9b)--(374.1a,255.9b)--(374.1a,220.0b)--(337.7a,220.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (459.0a,220.0b)--(495.5a,220.0b)--(495.5a,491.3b)--(459.0a,491.3b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (459.0a,220.0b)--(459.0a,491.2b)--(495.4a,491.2b)--(495.4a,220.0b)--(459.0a,220.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (580.3a,220.0b)--(616.7a,220.0b)--(616.7a,248.6b)--(580.3a,248.6b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (580.3a,220.0b)--(580.3a,248.5b)--(616.6a,248.5b)--(616.6a,220.0b)--(580.3a,220.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (701.5a,220.0b)--(738.0a,220.0b)--(738.0a,233.7b)--(701.5a,233.7b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (701.5a,220.0b)--(701.5a,233.6b)--(737.9a,233.6b)--(737.9a,220.0b)--(701.5a,220.0b);
linetype 1;
linetype -2;
draw (214.4a,581.6b)--(214.4a,220.0b)--(820.8a,220.0b)--(820.8a,581.6b)--(214.4a,581.6b);
endfig;
end.
