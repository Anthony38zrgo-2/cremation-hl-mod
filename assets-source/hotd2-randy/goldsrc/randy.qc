$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-randy/goldsrc/randy.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-randy/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-randy/goldsrc"
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
$hbox 2 "Torso" -5.02144 -13.56423 -0.59633 14.74811 13.57585 32.23953
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -12.09174 -6.95610 -7.52913 2.96733 6.94484 11.38264
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -6.01732 -5.45081 -19.83054 5.67064 6.58490 7.42369
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -4.70787 -4.52204 -21.52354 6.01755 4.58309 9.23711
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 blank
}
$hbox 5 "Hand_R" -3.03259 -6.53152 -25.01258 10.29865 4.97681 1.42772
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -5.94524 -6.56799 -19.83063 5.74272 5.46773 7.42360
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -4.63579 -4.56618 -21.52363 6.08963 4.53896 9.23703
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 blank
}
$hbox 4 "Hand_L" -2.96051 -4.95991 -25.01267 10.37073 6.54842 1.42763
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -7.83622 -10.82043 -16.61176 10.13361 10.80489 4.19761
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -5.91724 -4.67832 -21.73066 6.34716 5.89226 3.61195
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -8.29451 -5.23595 -22.90294 3.83381 5.23721 6.38465
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -14.36737 -3.87495 -6.23397 5.38665 4.63185 3.34587
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -5.91698 -5.92775 -21.72064 6.34748 4.64594 3.62597
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -8.25470 -5.31714 -22.90055 3.88307 5.15603 6.45147
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -14.57412 -4.71755 -6.11190 5.15372 3.78924 3.35797
$body abdomen "abdomen"
$sequence idle "idle" fps 3.750000 loop
$sequence run1 "run1" fps 30.000000 loop LX
$sequence diesimple "diesimple" fps 37.500000 { event 2001 15 }
$sequence dieforward "dieforward" fps 52.500000 { event 2001 18 }
$sequence diebackward "diebackward" fps 81.666667 { event 2001 29 }
$sequence attack1 "attack1" fps 127.500000 loop { event 1 24 } { event 2 66 }
$sequence jumpattack "jumpattack" fps 17.000000 { event 4 5 }
$sequence climb "climb" fps 30.000000 { event 1003 15 } { event 1003 30 } { event 1003 45 } { event 1003 60 } { event 1003 75 } { event 1003 90 } { event 1003 105 } { event 1003 120 } { event 1003 135 } { event 1003 150 } { event 1003 165 } { event 1003 180 } { event 1003 195 }
$sequence climb2 "climb2" fps 30.000000 { event 1003 15 } { event 1003 30 } { event 1003 45 } { event 1003 60 } { event 1003 75 } { event 1003 90 } { event 1003 105 } { event 1003 120 } { event 1003 135 } { event 1003 150 } { event 1003 165 }
