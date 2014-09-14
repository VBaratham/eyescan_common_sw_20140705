#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import glob
import datetime

TURN_ON_COMMANDS = True

def run_command( command ) :
    ''' wrapper around os.system '''
    if TURN_ON_COMMANDS :
        os.system( command )
    else :
        print command

def run_eye_scan( label , scan_type ) :
    horz_step = 1 
    vert_step = 8
    if scan_type == 1 :
        vert_step = 127
    max_prescale = 8
    data_width = 40
    lpm_mode = 0 
    rate = 32
    
    TCLDIR='../tcl_scripts'
    if os.path.exists( '%s/test.tcl' % TCLDIR ) :
        pwd = os.path.realpath( os.curdir )
        os.chdir( TCLDIR )
        run_command( 'rm *.dump *.csv ascii_eye.txt' )
        run_command( 'tclsh test.tcl %d %d %d %d %d %d' % ( horz_step , vert_step , max_prescale , data_width , lpm_mode , rate ) )
        td = datetime.date.today()
        tdstr = '%04d%02d%02d' % ( td.year , td.month , td.day )
        run_command( 'tar zcvf %s_%sd_%s.tar.gz *.dump *.csv ascii_eye.txt' % ( label , scan_type , tdstr ) )
        run_command( 'rm *.dump *.csv ascii_eye.txt' )
        run_command( 'mv %s_%sd_%s.tar.gz %s/' % ( label , scan_type , tdstr , pwd ) )
        os.chdir( pwd )
    else :
        print 'Tcl directory %s doesn\'t exist' % TCLDIR    

def make_plots( fn , scan_type , title , freq ) :
    if not os.path.exists( fn ) :
        return False
    dn = fn.strip( '.tar.gz' ).split('/')[-1]
    afn = os.path.realpath( fn )
    run_command( 'mkdir -p %s' % dn )
    os.chdir( dn )
    run_command( 'tar zxvf %s' % afn )
    gl = glob.glob( 'Ch*.csv' )
    nf = len(gl)
    for fn in gl :
        if scan_type == 1 :
            import bathtub_plot
            bathtub_plot.bathtub_plot( fn0 = fn , title = '%s %s' % ( title , fn ) )
        elif scan_type == 2 :
            import eyescan_plot
            eyescan_plot.eyescan_plot( fn = fn , title = '%s %s' % ( title , fn ) )
    run_command( 'tar zxvf ../dummy.tar.gz' )
    run_command( 'mv dummy %s' % dn )
    run_command( 'mv %s/dummy.tex %s/%s.tex' % ( dn , dn , dn ) )
    run_command( 'sed -i \'s:DUMMY:%s:g\' %s/%s.tex' % ( dn , dn , dn ) )
    run_command( 'sed -i \'s:freq:%d:g\' %s/%s.tex' % ( freq , dn , dn ) )
    if scan_type == 1 :
        run_command( 'sed -i \'s:TYPE:Bathtub:g\' %s/%s.tex' % ( dn , dn ) )
    elif scan_type == 2 :
        run_command( 'sed -i \'s:TYPE:Eyescan:g\' %s/%s.tex' % ( dn , dn ) )
    for idx in range( 0 , nf ) :
        run_command( 'convert Ch%d.csv.png %s/%s_Ch%d.pdf' % ( idx , dn , dn , idx ) )
    os.chdir( '%s' % dn )
    run_command( 'pdflatex %s.tex' % dn )
    run_command( 'pdflatex %s.tex' % dn )

if __name__ == '__main__' :
    if os.sys.argv[1] == 'esrun' :
        scan_type = 1
        scan_label = 'default'
        if len(os.sys.argv) > 2 :
            scan_label = os.sys.argv[2]
        if len(os.sys.argv) > 3 :
            try :
                if int(os.sys.argv[3]) == 2 :
                    scan_type = 2
            except :
                pass
        print 'scan will be %d-d, label will be %s' % ( scan_type , scan_label )
        
        run_eye_scan( scan_label , scan_type )
    elif os.sys.argv[1] == 'esplot' :
        tarfile = None
        scan_type = 1
        freq = 6400
        if len(os.sys.argv) > 2 :
            tarfile = os.path.relpath( os.sys.argv[2] )
        else :
            print 'no tar file given'
            exit(0)
        if len(os.sys.argv) > 3 :
            try :
                if int(os.sys.argv[3]) == 2 :
                    scan_type = 2
            except :
                pass
        if len(os.sys.argv) > 4 :
            try :
                freq = int(os.sys.argv[4])
            except :
                pass
        title = 'Bathtub at %d MHz' % freq
        if scan_type == 2 :
            title = 'Eyescan at %d MHz' % freq
        make_plots( tarfile , scan_type , title , freq )
        