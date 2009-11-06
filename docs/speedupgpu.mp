%GNUPLOT Metapost output: Mon Nov  2 22:51:22 2009


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
draw (170.4a,220.0b)--(187.0a,220.0b);
draw (820.8a,220.0b)--(804.2a,220.0b);
put_text( btex  0 etex, 149.2a, 220.0b, 0, 3);
draw (170.4a,265.2b)--(187.0a,265.2b);
draw (820.8a,265.2b)--(804.2a,265.2b);
put_text( btex  2 etex, 149.2a, 265.2b, 0, 3);
draw (170.4a,310.4b)--(187.0a,310.4b);
draw (820.8a,310.4b)--(804.2a,310.4b);
put_text( btex  4 etex, 149.2a, 310.4b, 0, 3);
draw (170.4a,355.6b)--(187.0a,355.6b);
draw (820.8a,355.6b)--(804.2a,355.6b);
put_text( btex  6 etex, 149.2a, 355.6b, 0, 3);
draw (170.4a,400.8b)--(187.0a,400.8b);
draw (820.8a,400.8b)--(804.2a,400.8b);
put_text( btex  8 etex, 149.2a, 400.8b, 0, 3);
draw (170.4a,446.0b)--(187.0a,446.0b);
draw (820.8a,446.0b)--(804.2a,446.0b);
put_text( btex  10 etex, 149.2a, 446.0b, 0, 3);
draw (170.4a,491.2b)--(187.0a,491.2b);
draw (820.8a,491.2b)--(804.2a,491.2b);
put_text( btex  12 etex, 149.2a, 491.2b, 0, 3);
draw (170.4a,536.4b)--(187.0a,536.4b);
draw (820.8a,536.4b)--(804.2a,536.4b);
put_text( btex  14 etex, 149.2a, 536.4b, 0, 3);
draw (170.4a,581.6b)--(187.0a,581.6b);
draw (820.8a,581.6b)--(804.2a,581.6b);
put_text( btex  16 etex, 149.2a, 581.6b, 0, 3);
draw (300.5a,220.0b)--(300.5a,236.6b);
draw (300.5a,581.6b)--(300.5a,565.0b);
put_text( btex saxpy etex, 300.5a, 198.8b, -30, 1);
draw (430.6a,220.0b)--(430.6a,236.6b);
draw (430.6a,581.6b)--(430.6a,565.0b);
put_text( btex sgemm etex, 430.6a, 198.8b, -30, 1);
draw (560.6a,220.0b)--(560.6a,236.6b);
draw (560.6a,581.6b)--(560.6a,565.0b);
put_text( btex conv2d etex, 560.6a, 198.8b, -30, 1);
draw (690.7a,220.0b)--(690.7a,236.6b);
draw (690.7a,581.6b)--(690.7a,565.0b);
put_text( btex dotprod etex, 690.7a, 198.8b, -30, 1);
draw (170.4a,581.6b)--(170.4a,220.0b)--(820.8a,220.0b)--(820.8a,581.6b)--(170.4a,581.6b);
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
fill (259.3a,220.0b)--(298.4a,220.0b)--(298.4a,237.6b)--(259.3a,237.6b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (259.3a,220.0b)--(259.3a,237.5b)--(298.3a,237.5b)--(298.3a,220.0b)--(259.3a,220.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (389.4a,220.0b)--(428.5a,220.0b)--(428.5a,279.8b)--(389.4a,279.8b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (389.4a,220.0b)--(389.4a,279.7b)--(428.4a,279.7b)--(428.4a,220.0b)--(389.4a,220.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (519.4a,220.0b)--(558.6a,220.0b)--(558.6a,225.0b)--(519.4a,225.0b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (519.4a,220.0b)--(519.4a,224.9b)--(558.5a,224.9b)--(558.5a,220.0b)--(519.4a,220.0b);
linetype 0;
fillcolor:=currentcolor*0.12+background*0.88;
fill (649.5a,220.0b)--(688.7a,220.0b)--(688.7a,234.6b)--(649.5a,234.6b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (649.5a,220.0b)--(649.5a,234.5b)--(688.6a,234.5b)--(688.6a,220.0b)--(649.5a,220.0b);
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
fill (302.6a,220.0b)--(341.8a,220.0b)--(341.8a,256.0b)--(302.6a,256.0b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (302.6a,220.0b)--(302.6a,255.9b)--(341.7a,255.9b)--(341.7a,220.0b)--(302.6a,220.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (432.7a,220.0b)--(471.9a,220.0b)--(471.9a,491.3b)--(432.7a,491.3b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (432.7a,220.0b)--(432.7a,491.2b)--(471.8a,491.2b)--(471.8a,220.0b)--(432.7a,220.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (562.8a,220.0b)--(601.9a,220.0b)--(601.9a,248.6b)--(562.8a,248.6b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (562.8a,220.0b)--(562.8a,248.5b)--(601.8a,248.5b)--(601.8a,220.0b)--(562.8a,220.0b);
linetype 1;
fillcolor:=currentcolor*0.75+background*0.25;
fill (692.9a,220.0b)--(732.0a,220.0b)--(732.0a,233.7b)--(692.9a,233.7b)--cycle withpen (pencircle scaled 0pt) withcolor fillcolor;
linetype -2;
draw (692.9a,220.0b)--(692.9a,233.6b)--(731.9a,233.6b)--(731.9a,220.0b)--(692.9a,220.0b);
linetype 1;
linetype -2;
draw (170.4a,581.6b)--(170.4a,220.0b)--(820.8a,220.0b)--(820.8a,581.6b)--(170.4a,581.6b);
endfig;
end.
