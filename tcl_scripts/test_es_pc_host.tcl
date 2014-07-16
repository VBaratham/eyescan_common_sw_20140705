source es_pc_host.tcl

proc run_test {} {
    
    #####################
    # Set test parameters
    #####################
    set ch_list [list 0 1 2 3 4 5 6 7]
    #set ch_list [list 0]
    set NUM_CH 8
    array set data_width {}
    foreach i $ch_list {
    
        # test_ch: Set to 1 to run eye scan on channel. Otherwise set to 0.
        set test_ch($i) 1
        
        # horz_step: Horizontal sweep step size. Min value is 1. Max value depends on data rate mode (see user guide & rate variable below)
        set horz_step($i) 8
        # set horz_step($i) 255
        
        # vert_step: Vertical sweep step size. Min value is 1. Max value is 127.
        # set vert_step($i) 8
        # use this setting to generate 1-d bathtub plots
        set vert_step($i) 127
        
        # max_prescale: Maximum prescale value for sample count. Min value is 0. Max value is 31.
        set max_prescale($i) 8
        
        # data_width: Parallel data width interface. Valid values are 16, 20, 32, 40.
        set data_width($i) 40

        # lpm_mode: Set to 1 for LPM mode. Set to 0 for DFE mode.
        set lpm_mode($i) 1

        # rate: Set depending on full, half, quarter, octal, or hex modes (see user guide)
        set rate($i) 256
        
        # out_file: Set to desired raw output file name.
        set out_file($i) "Ch$i.dump"
    }
    
    #####################
    # Run test
    #####################
    set retval [es_host_run test_ch horz_step vert_step lpm_mode rate max_prescale data_width out_file]
    
    #####################
    # Post-process
    #####################
    if {$retval == 1} {
        # Specify file for ascii eye of all channels tested
        set f_out [open "ascii_eye.txt" w]

        for { set i 0 } { $i < $NUM_CH } { incr i } {
            
            if {$test_ch($i) == 1} {
                # Eye scan results will be populated into following arrays
                array set horz_arr {}
                array set vert_arr {} 
                array set utsign_arr {}
                array set sample_count_arr {}
                array set error_count_arr {}
                array set prescale_arr {}
                
                # Generate raw dump and ess. Extract data into arrays.
                es_host_process_dump Ch${i}.dump $i horz_arr vert_arr utsign_arr sample_count_arr error_count_arr prescale_arr $data_width($i)
                
                # Generate ASCII output
                puts $f_out "\nCHANNEL $i"
                puts "CHANNEL $i"
                es_host_plot_ascii_eye $f_out $lpm_mode($i) horz_arr vert_arr utsign_arr sample_count_arr error_count_arr
                
                # Generate CSV for ibertplotter
                es_host_gen_csv Ch${i}.csv $lpm_mode($i) $vert_step($i) horz_arr vert_arr utsign_arr sample_count_arr error_count_arr prescale_arr $data_width($i) $rate($i)

            }
            
        }

        close $f_out
    }

}
