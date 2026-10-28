$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-max/goldsrc/max.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-max/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-max/goldsrc"
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
$hbox 2 "Torso" -6.09211 -11.47526 -1.36255 7.97506 11.47526 19.61104
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -7.51046 -4.29587 -4.19089 1.99439 4.28737 7.24394
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -4.50311 -4.06337 -13.71121 4.08970 3.79329 4.40907
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -2.86636 -2.98291 -12.62764 3.73268 3.41015 1.72592
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 blank
}
$hbox 5 "Hand_R" -5.14528 -6.02264 -32.63740 5.63667 2.66262 1.90068
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -4.50311 -3.94628 -13.71122 4.08970 3.91037 4.40907
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -2.86636 -3.56315 -12.62765 3.73268 2.82991 1.72591
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 blank
}
$hbox 4 "Hand_L" -5.14526 -5.32563 -32.63740 5.63669 3.35963 1.90068
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -5.77512 -9.55821 -11.99331 5.63066 7.71764 4.43544
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -5.70879 -3.92252 -20.74543 4.55619 5.14460 3.44394
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -3.19173 -3.75077 -23.09800 5.59842 4.26253 1.92107
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -11.02532 -3.06625 -3.10421 4.00267 3.59041 2.13557
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -4.52597 -4.46780 -20.74543 4.55619 4.14351 3.44394
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -4.10624 -4.73224 -21.09352 5.45702 3.84680 1.92107
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -11.02532 -3.36942 -3.10420 4.00267 3.28724 2.13557
$body abdomen "abdomen"
$sequence idle "idle" fps 30 loop
$sequence walk "walk" fps 30 loop LX
$sequence run "run" fps 30 loop LX
$sequence attack_00 "attack_00" fps 30 { event 3 14 }
$sequence attack_01 "attack_01" fps 30 { event 3 40 }
$sequence attack_02 "attack_02" fps 30 { event 3 14 }
$sequence attack_03 "attack_03" fps 30 { event 3 14 }
$sequence attack_04 "attack_04" fps 30 { event 3 18 }
$sequence attack_05 "attack_05" fps 30 { event 3 18 }
$sequence attack_06 "attack_06" fps 30 { event 3 27 }
$sequence attack_07 "attack_07" fps 30 { event 3 31 }
$sequence attack_08 "attack_08" fps 30 { event 3 31 }
$sequence pain "pain" fps 30
$sequence die_simple "die_simple" fps 30
$sequence die_back "die_back" fps 30
$sequence die_forward "die_forward" fps 30
$sequence die_violent "die_violent" fps 30
$sequence die_head "die_head" fps 30
