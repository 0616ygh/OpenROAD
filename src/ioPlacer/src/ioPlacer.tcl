###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################


sta::define_cmd_args "set_io_pin_constraint" {[-direction direction] \
                                              [-names names] \
                                              [-region region]}

proc set_io_pin_constraint { args } {
  sta::parse_key_args "set_io_pin_constraint" args \
  keys {-direction -names -region}

  if [info exists keys(-region)] {
    set region $keys(-region)
  }

  set dbTech [ord::get_db_tech]
  set lef_units [$dbTech getLefUnits]

  if [regexp -all {(top|bottom|left|right):(.+)} $region - edge interval] {
    set edge_ [ppl::parse_edge "-region" $edge]

    if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
      if {$begin == {*}} {
        set begin [ppl::get_edge_extreme "-region" 1 $edge]
      }
      if {$end == {*}} {
        set end [ppl::get_edge_extreme "-region" 0 $edge]
      }

      set begin [expr { int($begin * $lef_units) }]
      set end [expr { int($end * $lef_units) }]
    } elseif {$interval == {*}} {
      set begin [ppl::get_edge_extreme "-region" 1 $edge]
      set end [ppl::get_edge_extreme "-region" 0 $edge]
    }
  }

  if {[info exists keys(-direction)] && [info exists keys(-name)]} {
    ord::error "set_io_pin_constraint: only one constraint allowed"
  }

  if [info exists keys(-direction)] {
    set direction $keys(-direction)
    set dir [ppl::parse_direction "set_io_pin_constraint" $direction]
    puts "Restrict $direction pins to region $begin-$end, in the $edge edge"
    ppl::add_direction_constraint $dir $edge_ $begin $end
  }

  if [info exists keys(-names)] {
    set names $keys(-names)
    foreach name $names {
      puts "Restrict I/O pin $name to region $begin-$end, in the $edge edge"
      ppl::add_name_constraint $name $edge_ $begin $end
    }
  }
}

sta::define_cmd_args "place_pins" {[-hor_layers h_layers]\
                                  [-ver_layers v_layers]\
                                  [-random_seed seed]\
                       	          [-random]\
                                  [-boundaries_offset offset]\
                                  [-min_distance min_dist]\
                                  [-exclude region]\
                                 }

proc io_placer { args } {
  ord::warn "io_placer command is deprecated. Use place_pins instead"
  [eval place_pins $args]
}

proc place_pins { args } {
  set regions [ppl::parse_excludes_arg $args]
  sta::parse_key_args "place_pins" args \
  keys {-hor_layers -ver_layers -random_seed -boundaries_offset -min_distance -exclude} \
  flags {-random}

  set dbTech [ord::get_db_tech]
  if { $dbTech == "NULL" } {
    ord::error "missing dbTech"
  }

  set dbBlock [ord::get_db_block]
  if { $dbBlock == "NULL" } {
    ord::error "missing dbBlock"
  }

  set db [::ord::get_db]
  
  set blockages {}

  foreach inst [$dbBlock getInsts] {
    if { [$inst isBlock] } {
      if { ![$inst isPlaced] } {
        puts "\[ERROR\] Macro [$inst getName] is not placed"
          continue
      }
      lappend blockages $inst
    }
  }

  puts "#Macro blocks found: [llength $blockages]"

  set seed 42
  if [info exists keys(-random_seed)] {
    set seed $keys(-random_seed)
  }
  ppl::set_rand_seed $seed

  if [info exists keys(-hor_layers)] {
    set hor_layers $keys(-hor_layers)
  } else {
    ord::error "-hor_layers is mandatory"
  }       
  
  if [info exists keys(-ver_layers)] {
    set ver_layers $keys(-ver_layers)
  } else {
    ord::error "-ver_layers is mandatory"
  }

  set offset 5
  if [info exists keys(-boundaries_offset)] {
    set offset $keys(-boundaries_offset)
    ppl::set_boundaries_offset $offset
  } else {
    puts "Using ${offset}u default boundaries offset"
    ppl::set_boundaries_offset $offset
  }

  set min_dist 2
  if [info exists keys(-min_distance)] {
    set min_dist $keys(-min_distance)
    ppl::set_min_distance $min_dist
  } else {
    puts "Using $min_dist tracks default min distance between IO pins"
    ppl::set_min_distance $min_dist
  }

  set bterms_cnt [llength [$dbBlock getBTerms]]

  if { $bterms_cnt == 0 } {
    ord::error "Design without pins"
  }

  foreach hor_layer $hor_layers {
    set hor_track_grid [$dbBlock findTrackGrid [$dbTech findRoutingLayer $hor_layer]]
    if { $hor_track_grid == "NULL" } {
      ord::error "Horizontal routing layer ($hor_layer) not found"
    }

    if { ![ord::db_layer_has_hor_tracks $hor_layer] } {
      ord::error "Routing tracks not found for layer $hor_layer"
    }
  }

  foreach ver_layer $ver_layers {
    set ver_track_grid [$dbBlock findTrackGrid [$dbTech findRoutingLayer $ver_layer]]
    if { $ver_track_grid == "NULL" } {
      ord::error "Vertical routing layer ($ver_layer) not found"
    }

    if { ![ord::db_layer_has_ver_tracks $ver_layer] } {
      ord::error "Routing tracks not found for layer $hor_layer"
    }
  }

  set num_tracks_x [llength [$ver_track_grid getGridX]]
  set num_tracks_y [llength [$hor_track_grid getGridY]]
  
  set num_slots [expr (2*$num_tracks_x + 2*$num_tracks_y)/$min_dist]

  if { ($bterms_cnt > $num_slots) } {
    ord::error "Number of pins ($bterms_cnt) exceed max possible ($num_slots)"
  }
 
  if { $regions != {} } {
    set lef_units [$dbTech getLefUnits]
    
    foreach region $regions {
      if [regexp -all {(top|bottom|left|right):(.+)} $region - edge interval] {
        set edge_ [ppl::parse_edge "-exclude" $edge]

        if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
          if {$begin == {*}} {
            set begin [ppl::get_edge_extreme "-exclude" 1 $edge]
          }
          if {$end == {*}} {
            set end [ppl::get_edge_extreme "-exclude" 0 $edge]
          }
          set begin [expr { int($begin * $lef_units) }]
          set end [expr { int($end * $lef_units) }]

          ppl::exclude_interval $edge_ $begin $end
        } elseif {$interval == {*}} {
          set begin [ppl::get_edge_extreme "-exclude" 1 $edge]
          set end [ppl::get_edge_extreme "-exclude" 0 $edge]

          ppl::exclude_interval $edge_ $begin $end
        } else {
          ord::error "-exclude: $interval is an invalid region"
        }
      } else {
        ord::error "-exclude: invalid syntax in $region. use (top|bottom|left|right):interval"
      }
    }
  }

  foreach hor_layer $hor_layers {
    ppl::add_hor_layer $hor_layer 
  }

  foreach ver_layer $ver_layers {
    ppl::add_ver_layer $ver_layer
  }
  
  ppl::run_io_placement [info exists flags(-random)]
}

namespace eval ppl {

proc parse_edge { cmd edge } {
  if {$edge != "top" && $edge != "bottom" && \
      $edge != "left" && $edge != "right"} {
    ord::error "$cmd: $edge is an invalid edge. use top, bottom, left or right"
  }
  return [ppl::get_edge $edge]
}

proc parse_direction { cmd direction } {
  if {[regexp -nocase -- {^INPUT$} $direction] || \
      [regexp -nocase -- {^OUTPUT$} $direction] || \
      [regexp -nocase -- {^INOUT$} $direction] || \
      [regexp -nocase -- {^FEEDTHRU$} $direction]} {
    set direction [string tolower $direction]
    return [ppl::get_direction $direction]      
  } else {
    ord::error "$cmd: Invalid pin direction"
  }
}

proc parse_excludes_arg { args_var } {
  set regions {}
  while { $args_var != {} } {
    set arg [lindex $args_var 0]
    if { $arg == "-exclude" } {
      lappend regions [lindex $args_var 1]
      set args_var [lrange $args_var 1 end]
    } else {
      set args_var [lrange $args_var 1 end]
    }
  }

  return $regions
}

proc get_edge_extreme { cmd begin edge } {
  set dbBlock [ord::get_db_block]
  set die_area [$dbBlock getDieArea]
  if {$begin} {
    if {$edge == "top" || $edge == "bottom"} {
      set extreme [$die_area xMin]
    } elseif {$edge == "left" || $edge == "right"} {
      set extreme [$die_area yMin]
    } else {
      ord::error "$cmd: Invalid edge"
    }
  } else {
    if {$edge == "top" || $edge == "bottom"} {
      set extreme [$die_area xMax]
    } elseif {$edge == "left" || $edge == "right"} {
      set extreme [$die_area yMax]
    } else {
      ord::error "$cmd: Invalid edge"
    }
  }
}

proc exclude_intervals { cmd intervals } {
  if { $intervals != {} } {
    foreach interval $intervals {
      ppl::exclude_interval $interval
    }
  }
}

# ppl namespace end
}
