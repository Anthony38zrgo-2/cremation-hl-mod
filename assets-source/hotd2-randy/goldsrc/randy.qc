$modelname "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-randy/goldsrc/randy.mdl"
$cd "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-randy/goldsrc"
$cdtexture "D:/SteamLibrary/steamapps/common/Half-Life/Cremation/assets-source/hotd2-randy/goldsrc"
$origin 0 0 0 -90
$eyeposition 0 0 54
$bbox -16 -16 0 16 16 61
$attachment 0 "Head" 0 0 0
$bodygroup Torso
{
 studio "part_01_0"
 studio "part_01_1"
 studio "part_01_2"
 blank
}
$hbox 2 "Torso" -4.31539 -11.55302 -0.56633 12.55604 11.56287 27.37516
$bodygroup Head
{
 studio "part_02_0"
 studio "part_02_1"
 studio "part_02_2"
 blank
}
$hbox 1 "Head" -10.30550 -5.95447 -6.43995 2.57510 5.94494 9.70474
$bodygroup UpperArm_R
{
 studio "part_03_0"
 studio "part_03_1"
 studio "part_03_2"
 blank
}
$hbox 5 "UpperArm_R" -5.15912 -4.67916 -16.86199 4.86540 5.63998 6.35063
$bodygroup Forearm_R
{
 studio "part_04_0"
 studio "part_04_1"
 studio "part_04_2"
 blank
}
$hbox 5 "Forearm_R" -4.04972 -3.89228 -18.29633 5.15931 3.94401 7.88700
$bodygroup Hand_R
{
 studio "part_05_0"
 studio "part_05_1"
 studio "part_05_2"
 blank
}
$hbox 5 "Hand_R" -2.63039 -5.59476 -21.25232 8.78635 4.27758 1.27070
$bodygroup UpperArm_L
{
 studio "part_06_0"
 studio "part_06_1"
 studio "part_06_2"
 blank
}
$hbox 4 "UpperArm_L" -5.09805 -5.62566 -16.86206 4.92647 4.69349 6.35055
$bodygroup Forearm_L
{
 studio "part_07_0"
 studio "part_07_1"
 studio "part_07_2"
 blank
}
$hbox 4 "Forearm_L" -3.98865 -3.92968 -18.29641 5.22038 3.90662 7.88693
$bodygroup Hand_L
{
 studio "part_08_0"
 studio "part_08_1"
 studio "part_08_2"
 blank
}
$hbox 4 "Hand_L" -2.56932 -4.26325 -21.25240 8.84742 5.60908 1.27063
$bodygroup Pelvis
{
 studio "part_09_0"
}
$hbox 3 "Pelvis" -6.70013 -9.22842 -14.13496 8.64653 9.21525 3.61742
$bodygroup Thigh_R
{
 studio "part_10_0"
 studio "part_10_1"
 studio "part_10_2"
 blank
}
$hbox 7 "Thigh_R" -5.07433 -4.02469 -18.47181 5.43857 5.05316 3.12123
$bodygroup Shin_R
{
 studio "part_11_0"
 studio "part_11_1"
 studio "part_11_2"
 blank
}
$hbox 7 "Shin_R" -7.08840 -4.49713 -19.46499 3.30920 4.49819 5.47033
$bodygroup Foot_R
{
 studio "part_12_0"
 blank
}
$hbox 7 "Foot_R" -12.23347 -3.34405 -5.34267 4.62480 3.98532 2.89581
$bodygroup Thigh_L
{
 studio "part_13_0"
 studio "part_13_1"
 studio "part_13_2"
 blank
}
$hbox 6 "Thigh_L" -5.07411 -5.08323 -18.46332 5.43884 3.99726 3.13311
$bodygroup Shin_L
{
 studio "part_14_0"
 studio "part_14_1"
 studio "part_14_2"
 blank
}
$hbox 6 "Shin_L" -7.05467 -4.56591 -19.46296 3.35093 4.42941 5.52694
$bodygroup Foot_L
{
 studio "part_15_0"
 blank
}
$hbox 6 "Foot_L" -12.40863 -4.05793 -5.23925 4.42745 3.27144 2.90605
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
