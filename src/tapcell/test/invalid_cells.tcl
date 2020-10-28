source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def invalid_cells.def

tapcell -endcap_cpp "1" -distance "25" -tapcell_master "FILLCELL_X1x" -endcap_master "FILLCELL_X1x"