$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-johnny/goldsrc/johnny.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-johnny/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-johnny/goldsrc"
$origin 0 0 0 -90
$eyeposition 0 0 64
$bbox -16 -16 0 16 16 72
$attachment 0 "Head" 0 0 0
$bodygroup Torso
{
 studio "part_01_0"
 studio "part_01_1"
 studio "part_01_2"
 blank
}
$hbox 2 "Torso" -6.48558 -9.16417 0.74106 5.17557 9.20675 18.31308
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -5.89151 -5.96312 -5.12652 4.99609 5.96312 8.14286
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -2.66036 -3.40246 -13.04942 3.56583 2.83758 3.28360
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -1.88660 -2.68009 -12.45592 3.90984 2.79738 2.79019
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 studio "part_05_3"
 blank
}
$hbox 5 "Hand_R" -8.73985 -14.83650 -14.28296 5.35925 4.72469 1.53055
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -2.66037 -2.98832 -13.04943 3.56583 3.25171 3.28359
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -1.88660 -3.09120 -12.45593 3.90984 2.56788 2.79018
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 studio "part_08_3"
 blank
}
$hbox 4 "Hand_L" -8.73981 -4.72469 -14.28304 5.35929 14.83650 1.53055
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -5.95996 -7.37197 -11.39435 5.98936 7.37197 5.47777
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -4.07863 -3.74560 -19.34720 4.88795 3.88430 3.93801
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -3.15067 -3.02881 -22.76423 5.01507 3.85728 2.33966
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -10.86906 -3.02704 -3.06445 3.94969 3.54349 2.11003
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -4.07863 -3.66656 -19.34720 4.88795 3.96334 3.93801
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -3.15067 -3.63954 -22.76423 5.01507 3.24655 2.33967
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -10.86906 -3.32575 -3.06444 3.94969 3.24478 2.11004
$body abdomen "abdomen"
$sequence idle "idle" fps 30 loop
$sequence walk "walk" fps 30 loop LX
$sequence run "run" fps 30 loop LX
$sequence attack_right "attack_right" fps 30 { event 1 10 }
$sequence throw "throw" fps 30 { event 20 18 }
$sequence attack_left "attack_left" fps 30 { event 2 27 }
$sequence attack_both "attack_both" fps 30 { event 3 29 }
$sequence guard "guard" fps 30
$sequence pain_small "pain_small" fps 30
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
