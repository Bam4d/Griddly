#!/bin/bash
# Record a winning run for every level of a game with hunters, then replay them all:
#   bash studio/griddly-demo/tools/remaster/record_hunt.sh <game_id> [levels...]
# Per level: the cautious bot (bot_action.mjs, 20 seeds) records the first win; if it never wins, the
# engine-state planner (plan_hunt.mjs) searches seeds 1..6. Both replace only that level's entry in
# apps/web/src/games/solutions/<id>.json.
here=$(cd "$(dirname "$0")" && pwd)
id=$1; shift
n=$(node -e "
const fs=require('fs');const p='$id'==='spiders'?'resources/games/Single-Player/Mini-Grid/minigrid-spiders-garden.yaml':'resources/games/Single-Player/Remastered/$id.yaml';
const s=fs.readFileSync('$here/../../../../'+p,'utf8');console.log((s.split('\nActions:')[0].match(/^    - \|/mg)||[]).length)")
levels=${@:-$(seq 0 $((n - 1)))}
for l in $levels; do
  if node "$here/bot_action.mjs" "$id" 20 2500 "$l" --record 2>&1 | grep -q "record: wrote"; then
    echo "$id L$l: recorded by the cautious bot"
    continue
  fi
  ok=0
  for seed in 1 2 3 4 5 6; do
    if node "$here/plan_hunt.mjs" "$id" "$l" --seed=$seed --budget=${BUDGET:-1500} --record 2>&1 | grep -E "WIN"; then ok=1; break; fi
  done
  [ $ok = 1 ] || echo "$id L$l: NO RECORDED WIN"
done
node "$here/replay_solutions.mjs" "$id" 2>&1 | grep -v "\[info\]"
