#!/bin/zsh
# usage: game.sh shot NAME | game.sh in <winput args...> | game.sh fps [N] | game.sh continue   (SHIMDIR overrides where the shim writes)
S=/private/tmp/claude-501/-Users-nitinpunn-Documents-dev-foundations-game/2808f2bb-ac02-4502-bd44-3977225bd620/scratchpad
G="$HOME/Library/Application Support/CrossOver/Bottles/Foundation/drive_c/Program Files (x86)/Steam/steamapps/common/Foundation"
D="${SHIMDIR:-$G}"
CX=/Users/nitinpunn/Applications/CrossOver.app/Contents/SharedSupport/CrossOver
shot() { rm -f "$D/shot.ppm"; touch "$D/shot.req"; for i in $(seq 1 40); do [ -s "$D/shot.ppm" ] && ! [ -e "$D/shot.req" ] && break; sleep 0.25; done; sleep 0.3
         $S/venv/bin/python -I -c "from PIL import Image; im=Image.open('$D/shot.ppm'); im.save('$S/shots/$1.png'); print('$S/shots/$1.png', im.size)"; }
menu_visible() { $S/venv/bin/python -I -c "
from PIL import Image; im=Image.open('$D/shot.ppm').convert('RGB')
# main menu: 'Continue' text region is bright on dark-green; check a pixel row of the Continue label
r=im.crop((60,528,175,548)); import sys; px=list(r.get_flattened_data()) if hasattr(r,'get_flattened_data') else list(r.getdata()); bright=sum(1 for p in px if min(p)>200); sys.exit(0 if bright>40 else 1)"; }
case $1 in
  shot) shot $2;;
  in) shift; $CX/bin/wine --bottle Foundation --wait-children -- 'C:\tools\winput.exe' "$@" 2>/dev/null | tr -d '\r';;
  fps) tail -${2:-3} "$D/fps.log";;
  continue) for w in $(seq 1 100); do shot probe >/dev/null 2>&1; menu_visible && break; sleep 3; done; menu_visible || { echo 'menu never appeared'; exit 1; }; echo 'menu visible'; for try in 1 2 3 4 5 6; do $CX/bin/wine --bottle Foundation --wait-children -- 'C:\tools\winput.exe' move 115 538 >/dev/null 2>&1; sleep 0.4; $CX/bin/wine --bottle Foundation --wait-children -- 'C:\tools\winput.exe' click 115 538 >/dev/null 2>&1; sleep 3; shot probe >/dev/null; if ! menu_visible; then echo "continue clicked (try $try)"; exit 0; fi; sleep 2; done; echo "continue click FAILED"; exit 1;;
esac
