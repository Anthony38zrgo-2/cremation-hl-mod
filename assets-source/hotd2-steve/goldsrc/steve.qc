$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-steve/goldsrc/steve.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-steve/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-steve/goldsrc"
$origin 0 0 0 -90
$eyeposition 0 0 64
$bbox -16 -16 0 16 16 72
$attachment 0 "Head" 0 0 0
$bodygroup Torso
{
 studio "part_01_0"
 studio "part_01_1"
 studio "part_01_2"
 studio "part_01_3"
}
$hbox 2 "Torso" -6.20454 -9.26143 1.19656 5.21687 9.61588 18.91361
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -7.02599 -4.98597 -4.64368 4.27594 4.97752 7.59628
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -2.34499 -3.27653 -13.90822 5.07433 4.01455 3.72253
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -1.71353 -2.00496 -12.55124 3.31005 2.69763 2.05142
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 blank
}
$hbox 5 "Hand_R" -3.15180 -1.58369 -7.99779 3.13183 2.37085 1.42383
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -2.34499 -4.01455 -13.90821 5.07433 3.27653 3.72253
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -1.71353 -2.69763 -12.55124 3.31005 2.00496 2.05142
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 blank
}
$hbox 4 "Hand_L" -3.15180 -2.37085 -7.99779 3.13183 1.58369 1.42383
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -5.74756 -7.45374 -11.48942 6.03769 7.41078 5.96732
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -4.60777 -3.77453 -20.18904 6.06134 4.83000 3.96150
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -2.90497 -2.82392 -20.65648 3.90349 3.05193 1.16449
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -10.16419 -3.05004 -3.11765 3.70112 3.57040 2.41552
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -4.60778 -4.61037 -20.18904 6.06134 3.99415 3.96861
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -2.90497 -2.83230 -20.65648 3.90349 3.04354 1.16449
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -10.16419 -3.35077 -3.11764 3.70112 3.26966 2.41552
$body abdomen "abdomen"
$sequence idle "idle" fps 30 loop
$sequence walk "walk" fps 30 loop LX
$sequence run "run" fps 30 loop LX
$sequence attack_right "attack_right" fps 30 { event 1 21 }
$sequence attack_left "attack_left" fps 30 { event 2 27 }
$sequence attack_both "attack_both" fps 30 { event 3 20 }
$sequence pain_small "pain_small" fps 30
$sequence pain_big "pain_big" fps 30
$sequence die_simple "die_simple" fps 30
$sequence die_back "die_back" fps 30
$sequence die_forward "die_forward" fps 30
$sequence die_violent "die_violent" fps 30
$sequence die_head "die_head" fps 30
$sequence pain_head "pain_head" fps 30
$sequence pain_chest "pain_chest" fps 30
$sequence pain_stomach "pain_stomach" fps 30
$sequence pain_leftarm "pain_leftarm" fps 30
$sequence pain_rightarm "pain_rightarm" fps 30
$sequence pain_leftleg "pain_leftleg" fps 30
$sequence pain_rightleg "pain_rightleg" fps 30
$sequence crawl "crawl" fps 30 loop LX
$sequence crawl_idle "crawl_idle" fps 30 loop
$sequence prone_attack_right "prone_attack_right" fps 30 { event 1 25 }
$sequence prone_attack_left "prone_attack_left" fps 30 { event 2 28 }
$sequence prone_attack_both "prone_attack_both" fps 30 { event 3 28 }
$sequence prone_die "prone_die" fps 30
