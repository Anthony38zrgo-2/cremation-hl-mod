$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-david/goldsrc/david.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-david/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-david/goldsrc"
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
$hbox 2 "Torso" -5.62998 -9.84011 0.79991 5.38278 9.84010 19.13721
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -6.64574 -4.85118 -4.15882 3.96821 4.83438 8.19847
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -2.58444 -3.38231 -13.02317 3.49440 2.82030 3.26423
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -1.67971 -2.12999 -12.36697 3.27974 2.51317 2.02811
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 blank
}
$hbox 5 "Hand_R" -2.91500 -2.88011 -7.74765 2.97667 1.37962 1.53055
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -2.58395 -2.97004 -13.02563 3.49174 3.23257 3.26373
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -1.67971 -2.66291 -12.36698 3.27974 1.98026 2.02810
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 blank
}
$hbox 4 "Hand_L" -2.91500 -1.52936 -7.74764 2.97667 2.73038 1.53055
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -5.12567 -7.13395 -11.32058 5.19259 7.13395 7.41068
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -3.88034 -3.72315 -19.22008 4.85784 3.86092 3.91427
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -3.13221 -3.00935 -22.61418 4.98411 3.83408 2.32665
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -10.79882 -3.00941 -3.04657 3.92588 3.52240 2.09856
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -3.88048 -3.64464 -19.22004 4.85773 3.93943 3.91430
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -3.13221 -3.61780 -20.34817 5.05567 3.22563 2.32665
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -10.79882 -3.30612 -3.04657 3.92588 3.22570 2.09857
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
