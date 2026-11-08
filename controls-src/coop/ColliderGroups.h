// Mechanism group table (ColliderSync.h). {level, colliders, vars{address,pointer,trigger}, anchorMesh, radius}.
// Order inside a level is the wire record index; CoopMoverGroupCounts in CoopFrame.h
// (and GameFrame.cs) must match (checked by NativeTests).
static const int GroupRadius=0x30000;
static const ColliderGroupDesc colliderGroups[]={
 // Level 4 balance pulleys: 49ec00 mode 2 at 41c75b/41c7d8/41c851/41c8cd, counterweights 7/8/9/6
 // (41bee0 ropes), return path 48acc0 while nobody stands on the pair.
 {4,{1,7},{{0x52f7f8},{0x52f874,true},{0x52f854},{0x52f78c}},-1,0},
 {4,{2,8},{{0x52f7fc},{0x52f870,true},{0x52f860},{0x52f790}},-1,0},
 {4,{3,9},{{0x52f800},{0x52f87c,true},{0x52f85c},{0x52f794}},-1,0},
 {4,{4,6},{{0x52f808},{0x52f878,true},{0x52f858},{0x52f788}},-1,0},
 // Level 4 path movers 48acc0 (colliders 0xe..0x11, meshes 0x29..0x2c): program, speed, state.
 {4,{0xe,-1},{{0x52f898,true},{0x52f888},{0x52f77c}},-1,0},
 {4,{0xf,-1},{{0x52f894,true},{0x52f890},{0x52f780}},-1,0},
 {4,{0x10,-1},{{0x52f8a0,true},{0x52f88c},{0x52f774}},-1,0},
 {4,{0x11,-1},{{0x52f89c,true},{0x52f768},{0x52f778}},-1,0},
 // Level 4 tipping bridge: collider 0x1a, state 52f7e0 (0 idle -> 1 on the deck surface 0x24).
 {4,{0x1a,-1},{{0x52f7e0,false,true}},-1,GroupRadius},
 // Level 4 crumbling platforms 41bc20 (41cbdd/41cbfa/41cc17): timer (trigger), fall offset, fall speed,
 // collider 0x24/0x25/0x26 (enabled while solid), mesh 0xa/0xc/0xb position (moved natively only near
 // the local player, so it is shared too).
 {4,{0x24,-1},{{0x52f7a4,false,true},{0x52f7b8},{0x52f7ac},{0,false,false,0,0,0xa,0},{0,false,false,0,0,0xa,1},{0,false,false,0,0,0xa,2}},0xa,GroupRadius},
 {4,{0x25,-1},{{0x52f7a0,false,true},{0x52f7b4},{0x52f7b0},{0,false,false,0,0,0xc,0},{0,false,false,0,0,0xc,1},{0,false,false,0,0,0xc,2}},0xc,GroupRadius},
 {4,{0x26,-1},{{0x52f79c,false,true},{0x52f7bc},{0x52f7a8},{0,false,false,0,0,0xb,0},{0,false,false,0,0,0xb,1},{0,false,false,0,0,0xb,2}},0xb,GroupRadius},
 // Level 5 lift pairs 41e150 (stopped while someone stands on them) and balances 41ea34/41ea55.
 {5,{5,8},{{0x52f8a8},{0x52f8b4}},-1,0},
 {5,{6,7},{{0x52f8ac},{0x52f97c}},-1,0},
 {5,{9,0xa},{{0x52f944},{0x52f908}},-1,0},
 {5,{0xb,0xc},{{0x52f900},{0x52f8f4}},-1,0},
 {5,{2,-1},{{0x52f988}},-1,0},
 {5,{1,-1},{{0x52f93c}},-1,0},
 // Level 5 bubble rides (41edfe..41f5f2, after the 52f938 button): cycle phase 52f8f8 (& 0x1ff) starts
 // bubble 1 (state 52f91c, position 52f910/914/918, velocity 52f8c0/c4/c8, meshes 0xc/0xd) at 0x20 and
 // bubble 2 (52f92c, 52f920/924/928, 52f8d8/dc/e0, meshes 0x12/0x13) at 0x120. A bubble carries the
 // player riding it (53c668, native platform record 559d64). The phase lives only in bubble 1.
 {5,{-1,-1},{{0x52f8f8},{0x52f91c},{0x52f910},{0x52f914},{0x52f918},{0x52f8c0},{0x52f8c4},{0x52f8c8}},0xc,GroupRadius},
 {5,{-1,-1},{{0x52f92c},{0x52f920},{0x52f924},{0x52f928},{0x52f8d8},{0x52f8dc},{0x52f8e0}},0x12,GroupRadius},
 // Level 7 bobbing ride (collider 0, phase 52fa30, ride state 52f9e4 set on boarding).
 {7,{0,-1},{{0x52fa30},{0x52f9e4,false,true}},-1,0},
 // Level 7 target gate: target 48b640(0xf) turns collider 0xf/mesh 0x21 for a timed run (52fa24/52fa28).
 {7,{0xf,-1},{{0x52fa24,false,true},{0x52fa28}},-1,GroupRadius},
 // Level 7 press platforms 0xa (52fa4c) and 0xe (52f9e8); the launch itself stays local.
 {7,{0xa,-1},{{0x52fa4c},{0x52f9bc}},-1,0},   // + its velocity 52f9bc (COLMOVE at 421bcf)
 {7,{0xe,-1},{{0x52f9e8}},-1,0},
 // Level 7 path movers 48acc0 at 421e80..421fe1.
 {7,{7,-1},{{0x52fa00,true},{0x52fa44},{0x52f9d4}},-1,0},
 {7,{8,-1},{{0x52fa04,true},{0x52fa48},{0x52f9d8}},-1,0},
 {7,{9,-1},{{0x52f9f8,true},{0x52fa3c},{0x52f9cc}},-1,0},
 {7,{0xd,-1},{{0x52f9fc,true},{0x52fa40},{0x52f9d0}},-1,0},
 {7,{0xc,-1},{{0x52f9f0,true},{0x52fa34},{0x52f9c4}},-1,0},
 {7,{0xb,-1},{{0x52f9f4,true},{0x52fa38},{0x52f9c8}},-1,0},
 // Level 8 button lift: surface-8 button (timer 52fae4, meshes 0xd/0xe) advances 52fb88 (states via
 // table 424364), which moves meshes 0xa/0xb/0xc/0x10 through 52fba0/52fba4/52fba8 (52fb84 aux).
 {8,{-1,-1},{{0x52fb88,false,true},{0x52fae4},{0x52fba0},{0x52fba4},{0x52fba8},{0x52fb84}},0xa,GroupRadius},
 // Level 8 crane prize (4235db..423c47): prize position 52fb08/0c/10 (mesh 0x35, randomised on the first
 // approach 52fb18), grab result 52fb74 (0 missed, 1 carried, 2 falling, 3 landed -> the native pickup
 // record becomes collectable), fall speed 52fb1c. Same anchor as the lift, so the same owner.
 {8,{-1,-1},{{0x52fb74},{0x52fb08},{0x52fb0c},{0x52fb10},{0x52fb1c},{0x52fb18}},0xa,GroupRadius},
 // Level 10 path movers 48acc0 at 4273b9/427785 (colliders 1/0, meshes 0x34/0x37).
 {10,{1,-1},{{0x52fcf4,true},{0x52fccc},{0x52fce8}},-1,0},
 {10,{0,-1},{{0x52fcf0,true},{0x52fcd4},{0x52fce4}},-1,0},
 // Level 10 floor-switch puzzle (Level10SwitchRead in ColliderSync.h): packed counters, landing = request.
 {10,{-1,-1},{{0x52fcd8,false,true,Level10Switches,0,0,0,Level10SwitchRead}},0x12,GroupRadius},
 // Level 13 balances 42ca92/42cac1.
 {13,{3,-1},{{0x52ff20}},-1,0},
 {13,{4,-1},{{0x52ff24}},-1,0},
 // Level 13 moving platforms: five instances of 42c3e0 (42ce3c..42ce64), 0x20-byte records at 52fe48;
 // record +0x10 is the collider index (run time), +0x14 a native path handle (never sent).
 {13,{-2,-1},{{0x52fe48},{0x52fe4c},{0x52fe50},{0x52fe54},{0x52fe60},{0x52fe64}},-1,GroupRadius,{0x52fe58}},
 {13,{-2,-1},{{0x52fe68},{0x52fe6c},{0x52fe70},{0x52fe74},{0x52fe80},{0x52fe84}},-1,GroupRadius,{0x52fe78}},
 {13,{-2,-1},{{0x52fe88},{0x52fe8c},{0x52fe90},{0x52fe94},{0x52fea0},{0x52fea4}},-1,GroupRadius,{0x52fe98}},
 {13,{-2,-1},{{0x52fea8},{0x52feac},{0x52feb0},{0x52feb4},{0x52fec0},{0x52fec4}},-1,GroupRadius,{0x52feb8}},
 {13,{-2,-1},{{0x52fec8},{0x52fecc},{0x52fed0},{0x52fed4},{0x52fee0},{0x52fee4}},-1,GroupRadius,{0x52fed8}},
 // Level 14 lamp puzzle: lamps/target 52ffd0 (press trigger), moves/blink 52ff70, scrambled flag 52ffb4,
 // flash timer 52ff90, flash mesh 52ff48, door progress 52ff7c. Anchor: lamp mesh 0x23.
 // Level 11 paint can: colour 52fd14 (via 428890, blink bit 0x800 local), level buttons 52fd3c (via
 // 4292c0), target fill 52fd30, fill 52fdfc, button flash 52fd74/52fd44, pop scale 52fdac. Anchor mesh 0x24.
 {11,{-1,-1},{{0x52fd14,false,true,Level11Colour,~0xc00},{0x52fd3c,false,true,Level11Buttons,~0x200},{0x52fd30,false,true},{0x52fdfc},{0x52fd74},{0x52fd44},{0x52fdac}},0x24,GroupRadius,{0,0},0,Level11Request},
 // Level 11 paint-route toy (428e70): route node 52fd4c (table 4f3df8, rewritten by the colour), node
 // index 52fd84, direction 52fdbc, position 52fd98/9c/a0, heading 52fd0c, speed 52fdf0 (slowed when it
 // knocks the LOCAL player). At the route end (52fd4c == 0) collider 0x1b is enabled (4292b2).
 {11,{0x1b,-1},{{0x52fd4c},{0x52fd84},{0x52fdbc},{0x52fd98},{0x52fd9c},{0x52fda0},{0x52fd0c},{0x52fdf0}},0x26,GroupRadius},
 // Level 11 floating platforms 429fb0 (42a4af..42a4f7): colliders 0xd..0x10 bob on the paint fill and dip
 // when a player lands on them (offset/velocity pairs).
 {11,{0xd,-1},{{0x52fd90},{0x52fd28}},-1,0},
 {11,{0xe,-1},{{0x52fd7c},{0x52fd1c}},-1,0},
 {11,{0xf,-1},{{0x52fd88},{0x52fd20}},-1,0},
 {11,{0x10,-1},{{0x52fd50},{0x52fd18}},-1,0},
 {14,{-1,-1},{{0x52ffd0,false,true},{0x52ff70},{0x52ffb4},{0x52ff90},{0x52ff48},{0x52ff7c}},0x23,GroupRadius,{0,0},Level14Refresh,Level14Request},
 // Level 14 carousel 42dcb0 (called first by the tick): angle 52ff84 advances every tick and turns
 // collider 0 and meshes 4..0x56 / 0x10..0x1e around it; free-running, so each PC had its own phase.
 {14,{0,-1},{{0x52ff84}},-1,0},
};
