$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-ken/goldsrc/ken.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-ken/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-ken/goldsrc"
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
$hbox 2 "Torso" -5.64775 -9.45327 -2.00875 3.79670 9.45327 17.71102
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -6.37229 -7.93183 -5.04761 5.37002 7.87709 8.07897
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -2.11049 -2.88426 -11.68838 2.79774 2.22584 2.73312
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -2.00087 -2.79941 -12.61716 4.29114 2.81936 5.51109
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 blank
}
$hbox 5 "Hand_R" -2.24733 -4.44652 -13.74600 5.80032 2.54682 1.88350
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -2.11049 -2.36658 -11.68839 2.79774 2.74352 2.73311
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -2.00087 -2.96010 -12.61716 4.29114 2.65867 5.51109
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 blank
}
$hbox 4 "Hand_L" -2.24749 -2.54666 -13.74607 5.80016 4.44668 1.88343
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -3.46132 -5.76395 -10.64154 3.95086 5.76395 4.57128
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -3.63664 -3.61319 -18.09584 4.30016 3.79599 3.70639
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -4.54996 -3.19732 -19.57133 3.17396 3.19731 5.15945
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -9.22062 -2.37634 -2.22497 3.64972 2.88102 3.71424
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -3.63665 -3.59269 -18.09583 4.30016 3.81648 3.70639
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -4.54996 -2.99402 -19.57132 3.17396 3.40062 5.15945
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -9.22062 -2.67772 -2.22496 3.64972 2.57963 3.71424
$body abdomen "abdomen"
$sequence idle "idle" fps 30 loop
$sequence walk "walk" fps 30 loop LX
$sequence run "run" fps 30 loop LX
$sequence attack_00 "attack_00" fps 30 { event 1 28 }
$sequence attack_01 "attack_01" fps 30 { event 2 28 }
$sequence attack_02 "attack_02" fps 30 { event 3 24 }
$sequence pain "pain" fps 30
$sequence pain_big "pain_big" fps 30
$sequence die_simple "die_simple" fps 30
$sequence die_back "die_back" fps 30
$sequence die_forward "die_forward" fps 30
$sequence die_violent "die_violent" fps 30
$sequence die_head "die_head" fps 30
