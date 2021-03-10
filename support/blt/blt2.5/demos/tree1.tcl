#!/usr/bin/env wize

 variable Users {
     tom  { Name "Tom Brown"  Sex M Age 19  Class {4 5} Rate {A 1 B 2}}
     mary { Name "Mary Brown" Sex F Age 16  Class {5}   Rate {A 2}}
     sam  { Name "Sam Spade"  Sex M Age 19  Class {3 4} Rate {B 3}}
 }
 set t [tree create]
 foreach {i d} $Users {
    $t insert 0 -tags $i -data $d -label $i 
 }

 tree op update  $t tom Sex F Name "Tomi Brown"

 $t update  tom     Sex F Name "Tomi Brown"
 $t append  sam     Name " Jr"
 $t lappend sam     Class 5
 $t incr    mary    Age
 $t update  tom     Rate(A) 2
 $t set     tom     Sax F
 $t set     sam     Rate(C) 0
 $t incr    0->mary Age;         # Address via label instead of tag.

 puts [time { tree op incr $t mary Age } 1000]

 proc ::Aupd {keys t id key op} {
    tclLog "AA: $t $id $key $op"
   if {[lsearch -exact $keys $key]<0} { error "bad key '$key' not in: $keys" }
 }
 $t trace create all * w [list ::Aupd [$t keys all]]
 $t incr mary Age
 $t set mary Sex M

 pack [treeview .t -tree $t] -fill both -expand y
 eval .t column insert end [$t keys all]
puts [$t dump 0]
console show


 variable Info {
   system {
      sol  { OS Linux Version 3.4 }
      bing { OS Win Version 7 }
      gui  { OS Mac Version 8 }
   }
   network {
      intra { Address 192.168.1  Netmask 255.255.255.0 }
      dmz   { Address 192.168.10 Netmask 255.255.255.0 }
      wan   { Address 0.0.0.0 Netmask 0.0.0.0 Class {A 1 B 4}}
   }
   admin {
      sully { Name "Sully Van Damme" Level 3 }
      maverick { Name "Maverick Gump" Level 1 }
   }
 }
 
 set s [tree create]
 foreach {n vals} $Info {
   set ind [$s insert 0 -label $n]
   foreach {i d} $vals {
      if {[$s index $ind->$i]>=0} { error "Duplicate label: $i" }
      $s insert $ind -label $i -data $d
   }
 }
 
 set old [$s get  0->system->bing]
 $s update   0->system->bing   OS Linux Version 3.4
 $s update   0->network->dmz   Address 192.168.11
 $s update   0->network->wan   Class(A) 2
 eval $s set 0->system->bing   $old
 $s insert   0->admin -label linus -data { Name "Linus Torvalds" Level 9 } 
 $s delete   0->admin->sully


 pack [treeview .s -tree $s -width 600] -fill both -expand y
 eval .s column insert end [$s keys all]
