#!/usr/bin/env wish

#package require BLT
# --------------------------------------------------------------------------
# Starting with Tcl 8.x, the BLT commands are stored in their own 
# namespace called "blt".  The idea is to prevent name clashes with
# Tcl commands and variables from other packages, such as a "table"
# command in two different packages.  
#
# You can access the BLT commands in a couple of ways.  You can prefix
# all the BLT commands with the namespace qualifier "blt::"
#  
#    blt::graph .g
#    blt::table . .g -resize both
# 
# or you can import all the command into the global namespace.
#
#    namespace import blt::*
#    graph .g
#    table . .g -resize both
#
# --------------------------------------------------------------------------
if { $tcl_version >= 8.0 } {
    namespace import -force blt::*
    interp alias {} table {} blttable
    #namespace import -force blt::tile::*
}
source scripts/demo.tcl
#option add *Scrollbar.relief	flat
set oldLabel "dummy"
#wm geometry . 1020x750
variable pc
set pc(-bgexec) 0
set pc(-tearoff) 0
array set pc $argv
set pc(top) 0
set pc(win) {}

set pc(int) {}
proc killint {{c {}} {op {}} args} {
    variable pc
    if {$c == {}} { set c $pc(int) }
    if {$pc(int) != $c || $c == {}} return
    if {$op == "win"  && [lindex $args 0] != "."} return
    set pc(int) {}
    #puts "KILL [info level 0]"
    catch "interp delete $c"
}


proc RunDemo { program } {
    variable pc
    set tt .pw.swd.tab
    if {[string tolower [file extension [set pfile $program]]] != ".tcl"} {
        append pfile .tcl
    }
    if { ![file exists $pfile] } {
	return
    }
    if {$pc(-bgexec)} {
        global programInfo
        set cmd [list [info nameofexecutable] $pfile -name "demo:$program" -geom -4000-4000]
        if { [info exists programInfo(lastProgram)] } {
            set programInfo($programInfo(lastProgram)) 0
        }
        eval bgexec programInfo($program) $cmd &
        set programInfo(lastProgram) $program
    } else {
    }
    $tt.f2.t delete 1.0 end
    set data [read [set fp [open $pfile]]]
    close $fp
    $tt.f2.t insert end $data
    update
    #puts stderr [$tt.f1 find -name demo:$program]
    if {$pc(-bgexec)} {
        $tt.f1 configure -name demo:$program
    } else {
        set w  .pw.swd.tab.f1.f 
        if {[winfo exists $w]} { destroy $w }
        if {$pc(int) != {}} { killint $pc(int) }
        set c [interp create]
        interp eval $c [list set auto_path $::auto_path]
        set tl [winfo toplevel $tt]
        set tl $tt
        set maxg [winfo width $tl]x[winfo height $tl]
        if {$pc(-tearoff)} {
            pack [container $w] -fill both -expand y     
            $c eval "set argv { -geom $maxg }"
            $c eval "set argv0 {}"
        } else {
            pack [frame $w -container 1] -fill both -expand y   
            $c eval "set argv {-use [winfo id $w] -geom $maxg }"
            $c eval "set argv0 {}"
        }

        catch {interp eval $c {load {} Tk}}
        interp eval $c {package require Tk}
        interp eval $c {package require BLT}
        interp eval $c "bind . <Destroy> {__delete_interp %W}"
        interp alias $c __delete_interp {} killint $c win
        interp alias $c exit {} killint $c exit
        update
        if {$pc(-tearoff)} {
            update
            $c eval { if {![winfo ismapped .]} { tkwait visibility .} }
            $w conf -window $c.
            set pc(win) $c.
            set pc(top) 0
        }
        set pc(int) $c
        if {[catch {interp eval $c "source $pfile"} rc]} {
            tclLog "SRC=$pfile: $rc"
        }
    }
    if {$::tcl_platform(platform) == "unix"} {
        if {[catch {
            set prog [string trim $program {0123456789}]
            set man [exec man ../man/$prog.mann |& col -b]
            $tt.f3.t delete 1.0 end
            $tt.f3.t insert end $man
        } erc]} {
            tclLog "MAN=$pfile: $erc"
        }
    }
}

pack [panedwindow .pw] -fill both -expand y
frame .pw.swt
labelframe .pw.swd -text "Synopsis"
hierbox .pw.swt.hier -separator "." -scrollmode canvas -xscrollincrement 1 \
    -yscrollcommand { .pw.swt.yscroll set } -xscrollcommand { .pw.swt.xscroll set } \
    -selectcommand { 
	set index [.pw.swt.hier curselection]
	set label [.pw.swt.hier entry cget $index -label]
	.pw.swd configure -text $label
	.pw.swd.tab tab configure Example -window .pw.swd.tab.f1 
	if { $label != $oldLabel }  {
	    RunDemo $label
	}
    }
	
proc toplev {} {
    variable pc
    set ww .pw.swd.tab.f1.f
    set new [expr {!$pc(top)?$pc(win):{}}]
    if {$new != [$ww cget -window]} {
        $ww conf -window $new
    }
}

scrollbar .pw.swt.yscroll -command { .pw.swt.hier yview }
scrollbar .pw.swt.xscroll -command { .pw.swt.hier xview } -orient horizontal
#label .pw.swt.mesg -relief groove -borderwidth 2 
#label .pw.swd.title -text "Synopsis" -highlightthickness 0
if {$pc(-tearoff)} {
    pack [checkbutton .pw.swd.ext -variable ::pc(top) -text "Tearoff"   -command "toplev"] -anchor e
}
tabset .pw.swd.tab -side bottom -relief flat -bd 0 -highlightthickness 0 \
    -pageheight 4i

.pw.swd.tab insert end \
    "Example" \
    "See Code" \
    "Manual"
 
image create photo graph.img -file images/mini-windows.gif

.pw.swt.hier entry configure root -label "BLT"
.pw.swt.hier insert end \
    "Plotting" \
    "Plotting.graph" \
    "Plotting.graph.graph1" \
    "Plotting.graph.graph2" \
    "Plotting.graph.graph3" \
    "Plotting.graph.graph4" \
    "Plotting.graph.graph5" \
    "Plotting.graph.graph6" \
    "Plotting.barchart" \
    "Plotting.barchart.barchart1" \
    "Plotting.barchart.barchart2" \
    "Plotting.barchart.barchart3" \
    "Plotting.barchart.barchart4" \
    "Plotting.barchart.barchart5" \
    "Plotting.stripchart" \
    "Plotting.stripchart.stripchart1" \
    "Plotting.stripchart.spline" \
    "Composition" \
    "Composition.htext1" \
    "Composition.tabset" \
    "Composition.tabset.tabset1" \
    "Composition.tabset.tabset2" \
    "Composition.tabset.tabset3" \
    "Composition.tabset.tabset4" \
    "Composition.hierbox" \
    "Composition.hierbox.hierbox1" \
    "Composition.hierbox.hierbox2" \
    "Composition.hierbox.hierbox3" \
    "Composition.hierbox.hierbox4" \
    "Composition.hiertable" \
    "Composition.hiertable.hiertable1" \
    "Composition.hiertable.hiertable2" \
    "Composition.hiertable.hiertable3" \
    "Composition.treeview" \
    "Composition.treeview.treeview1" \
    "Miscellaneous" \
    "Miscellaneous.bitmap" \
    "Miscellaneous.bitmap.bitmap" \
    "Miscellaneous.bitmap.bitmap2" \
    "Miscellaneous.busy" \
    "Miscellaneous.busy.busy1" \
    "Miscellaneous.busy.busy2" \
    "Miscellaneous.bgexec" \
    "Miscellaneous.bgexec.bgexec1" \
    "Miscellaneous.bgexec.bgexec2" \
    "Miscellaneous.bgexec.bgexec3" \
    "Miscellaneous.bgexec.bgexec4" \
    "Miscellaneous.bgexec.bgexec5" \
    "Miscellaneous.container" \
    "Miscellaneous.container.container" \
    "Miscellaneous.container.container3" \
    "Miscellaneous.dnd" \
    "Miscellaneous.dnd.dnd1" \
    "Miscellaneous.dnd.dnd2" \
    "Miscellaneous.dnd.dragdrop1" \
    "Miscellaneous.dnd.dragdrop2" \
    "Miscellaneous.winop" \
    "Miscellaneous.winop.winop1" \
    "Miscellaneous.winop.winop2" \
    "Miscellaneous.eps"
.pw.swt.hier open -r root
.pw.swt.hier entry configure root -labelfont *-helvetica*-bold-r-*-18-* \
    -labelcolor red -labelshadow red3
.pw.swt.hier entry configure "Plotting" "Composition" "Miscellaneous" \
    -labelfont *-helvetica*-bold-r-*-14-* \
    -labelcolor blue4 -labelshadow blue2

.pw.swt.hier entry configure "Plotting.graph" \
    -labelfont *-helvetica*-bold-r-*-14-* -label "X-Y Graph"
.pw.swt.hier entry configure "Plotting.barchart" \
    -labelfont *-helvetica*-bold-r-*-14-* -label "Bar Chart"

.pw.swt.hier entry configure "Plotting.stripchart" \
    -labelfont *-helvetica*-bold-r-*-14-* -label "X-Y Graph"
.pw.swt.hier entry configure "Plotting.stripchart" \
    -labelfont *-helvetica*-bold-r-*-14-* -label "Strip Chart"

.pw.swt.hier entry configure "Plotting.graph" -icon graph.img
.pw.swt.hier entry configure "Plotting.barchart" -icon graph.img

.pw add .pw.swt -sticky news
.pw add .pw.swd -sticky news
#pack .pw.swd.title -fill x
pack .pw.swd.tab -fill both -expand y
grid .pw.swt.hier .pw.swt.yscroll -sticky news
grid .pw.swt.xscroll -sticky news
grid rowconfigure .pw.swt 0 -weight 1
grid columnconfigure .pw.swt 0 -weight 1


proc DoExit { code } {
    global progStatus
    set progStatus 1
    exit $code
}

container .pw.swd.tab.f1 -relief raised -bd 2 -takefocus 0
.pw.swd.tab tab configure Example -window .pw.swd.tab.f1 
foreach {sfx desc} {f2 "See Code" f3 "Manual"} {
    set ff .pw.swd.tab.$sfx
    .pw.swd.tab tab configure $desc -window [frame $ff] -fill both
    pack [scrollbar $ff.sh -command "$ff.t xview" -orient horizontal] -side bottom -fill x
    pack [scrollbar $ff.sv -command "$ff.t yview" ] -side right -fill y
    pack [text $ff.t -width 40 -height 15 -wrap none -xscrollcommand "$ff.sh set" -yscrollcommand "$ff.sv set"] -side right -fill both -expand y
}


if  {$pc(-bgexec)} {
    set cmd "xterm -fn fixed -geom +4000+4000"
    eval bgexec programInfo(xterm) $cmd &
    set programInfo(lastProgram) xterm
    .pw.swd.tab.f1 configure -command $cmd 
} else {
    RunDemo BLT
}
wm protocol . WM_DELETE_WINDOW { destroy . }

