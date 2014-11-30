#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import glob
import datetime

remote_user = 'ddboline'
remote_system = 'dboline.physics.sunysb.edu'
remote_dir = '/home/ddboline/setup_files/build/eyescanOTC_20140907/otc/Debug/py_scripts'

TURN_ON_COMMANDS = True
USE_PYTHON_CONTROLLER = True

def run_command(command) :
    ''' wrapper around os.system '''
    if TURN_ON_COMMANDS :
        os.system(command)
    else :
        print command

def run_eye_scan(label, scan_type, scan_rate = 32, max_prescale = 8, data_width = 40, lpm_mode = 0) :
    '''
    Run eye scan, taking scan_type and scan_rate as inputs.  Communicate with OTC through network socket.
    Use either the old tcl scripts, or newly written run_es_host_pc.
    '''
    horz_step = 1
    vert_step = 8
    if scan_type == 1 :
        vert_step = 127
    if scan_rate >= 128 :
        horz_step = 8

    td = datetime.date.today()
    tdstr = '%04d%02d%02d' % (td.year, td.month, td.day)
    SAMP_NAME = '%s_%sd_%s' % (label, scan_type, tdstr)
    PWD = os.path.realpath(os.curdir)

    if not USE_PYTHON_CONTROLLER :
        TCLDIR = '%s/../tcl_scripts' % PWD
        if os.path.exists('%s/test.tcl' % TCLDIR) :
            os.chdir(TCLDIR)
            run_command('rm *.dump *.csv *.txt *.output')
            run_command('tclsh test.tcl %d %d %d %d %d %d' % (horz_step, vert_step, max_prescale, data_width, lpm_mode, scan_rate))
        else :
            print 'Tcl directory %s doesn\'t exist' % TCLDIR
            return
    else :
        from run_es_host_pc import run_es_host, process_es_output

        os.makedirs('%s/%s' % (PWD, SAMP_NAME))
        os.chdir('%s/%s' % (PWD, SAMP_NAME))
        run_es_host(horz_step, vert_step, max_prescale, data_width, lpm_mode, scan_rate)
        process_es_output(horz_step, vert_step, max_prescale, data_width, lpm_mode, scan_rate)

    run_command('tar zcvf %s.tar.gz *.dump *.csv *.output *.txt' % (SAMP_NAME))
    run_command('rm *.dump *.csv *.txt *.output')
    run_command('mv %s.tar.gz %s/' % (SAMP_NAME, PWD))
    os.chdir(PWD)
    run_command('rm -rf %s/%s/' % (PWD, SAMP_NAME))
    run_command('scp %s.tar.gz %s@%s:%s/' % (SAMP_NAME, remote_user, remote_system, remote_dir))

    return

def make_plots(fn, scan_type, title, freq, fn2 = None) :
    if not os.path.exists(fn) :
        return False
    dn = fn.replace('.tar.gz', '').split('/')[-1]
    afn = os.path.realpath(fn)
    if fn2 :
        dn2 = fn2.replace('.tar.gz', '').split('/')[-1]
        afn2 = os.path.realpath(fn2)
        run_command('rm -rf %s' % dn2)
        run_command('mkdir -p %s' % dn2)
        os.chdir(dn2)
        run_command('tar zxvf %s' % afn2)
        os.chdir('../')
    run_command('rm -rf %s' % dn)
    run_command('mkdir -p %s' % dn)
    os.chdir(dn)
    run_command('tar zxvf %s' % afn)
    import parse_registers
    parse_registers.parse_registers(debug_fname = 'debug_end.output', regmap_fname = '../register_map.txt', output_fname = 'register_settings.txt', ibert_fname = '../register_default_ibert_640.txt', ibert_comp_outname = 'ibert_comp.txt')
    import central_error_plot
    central_error_plot.central_error_plot(fname = 'center_error.txt', title = 'Central BER')
    gl = glob.glob('Ch*.csv')
    nf = len(gl)
    for fn in gl :
        if scan_type == 1 :
            import bathtub_plot
            if not fn2 :
                bathtub_plot.bathtub_plot(fn0 = fn, title = '%s %s' % (title, fn))
            else :
                bathtub_plot.bathtub_plot(fn0 = fn, fn1 = '../%s/%s' % (dn2, fn), title = '%s %s' % (title, fn))
        elif scan_type == 2 :
            import eyescan_plot
            eyescan_plot.eyescan_plot(fn = fn, title = '%s %s' % (title, fn))
    run_command('tar zxvf ../dummy.tar.gz')
    run_command('mv dummy %s' % dn)
    run_command('mv %s/dummy.tex %s/%s.tex' % (dn, dn, dn))
    run_command('sed -i \'s:DUMMY:%s:g\' %s/%s.tex' % (dn, dn, dn))
    run_command('sed -i \'s:freq:%d:g\' %s/%s.tex' % (freq, dn, dn))
    run_command('mv central_ber.pdf %s/' % dn)
    if os.path.exists('/usr/bin/a2ps') :
        run_command('a2ps ibert_comp.txt -o %s/ibert_comp.ps' % dn)
        run_command('a2ps register_settings.txt -o %s/register_settings.ps' % dn)
    else :
        run_command('cat ibert_comp.txt | ssh ddboline@dboline.physics.sunysb.edu "cat - | a2ps -o -" > %s/ibert_comp.ps' % dn)
        run_command('cat register_settings.txt | ssh ddboline@dboline.physics.sunysb.edu "cat - | a2ps -o -" > %s/register_settings.ps' % dn)
    run_command('ps2pdf %s/ibert_comp.ps %s/ibert_comp.pdf' % (dn, dn))
    run_command('ps2pdf %s/register_settings.ps %s/register_settings.pdf' % (dn, dn))
    if scan_type == 1 :
        run_command('sed -i \'s:TYPE:Bathtub:g\' %s/%s.tex' % (dn, dn))
    elif scan_type == 2 :
        run_command('sed -i \'s:TYPE:Eyescan:g\' %s/%s.tex' % (dn, dn))
    for idx in range(0, nf) :
        if not fn2 :
            run_command('mv Ch%d.csv.pdf %s/%s_Ch%d.pdf' % (idx, dn, dn, idx))
        else :
            run_command('mv Ch%d.csvCh%d.csv.pdf %s/%s_Ch%d.pdf' % (idx, idx, dn, dn, idx))
    os.chdir('%s' % dn)
    run_command('pdflatex %s.tex' % dn)
    run_command('pdflatex %s.tex' % dn)
    if os.path.exists('/usr/bin/pdfunite') :
        run_command('pdfunite %s.pdf ibert_comp.pdf register_settings.pdf %s_temp.pdf' % (dn, dn))
    else :
        run_command('scp %s.pdf ibert_comp.pdf register_settings.pdf ddboline@dboline.physics.sunysb.edu:/tmp/ ; ssh ddboline@dboline.physics.sunysb.edu "cd /tmp/ ; pdfunite %s.pdf ibert_comp.pdf register_settings.pdf %s_temp.pdf ; rm ibert_comp.pdf register_settings.pdf" ; scp ddboline@dboline.physics.sunysb.edu:/tmp/%s_temp.pdf .' % (dn, dn, dn, dn))
    run_command('mv %s_temp.pdf %s.pdf' % (dn, dn))
    run_command('mkdir -p ../../../pdf_files/')
    run_command('cp %s.pdf ../../../pdf_files/' % dn)

if __name__ == '__main__' :
    if os.sys.argv[1] == 'esrun' :
        scan_type = 1
        scan_label = 'default'
        scan_rate = 32
        if len(os.sys.argv) > 2 :
            scan_label = os.sys.argv[2]
        if len(os.sys.argv) > 3 :
            try :
                if int(os.sys.argv[3]) == 2 :
                    scan_type = 2
            except :
                pass
        if len(os.sys.argv) > 4 :
            try :
                scan_rate = int(os.sys.argv[4])
            except :
                pass
        print 'scan will be %d-d, label will be %s, rate will be %d' % (scan_type, scan_label, scan_rate)

        exit(run_eye_scan(scan_label, scan_type, scan_rate))
    elif os.sys.argv[1] == 'esplot' :
        tarfile = None
        scan_type = 1
        freq = 6400
        if len(os.sys.argv) > 2 :
            tarfile = os.path.relpath(os.sys.argv[2])
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
        exit(make_plots(tarfile, scan_type, title, freq))
    elif os.sys.argv[1] == 'escomp' :
        tarfile0 = None
        tarfile1 = None
        scan_type = 1
        freq = 6400
        if len(os.sys.argv) > 3 :
            tarfile0 = os.path.relpath(os.sys.argv[2])
            tarfile1 = os.path.relpath(os.sys.argv[3])
        if len(os.sys.argv) > 4 :
            try :
                freq = int(os.sys.argv[4])
            except :
                pass
        title = 'eyescanOTC vs IBERT OTC Bathtub at %d MHz' % freq
        exit(make_plots(tarfile0, scan_type, title, freq, tarfile1))
    else :
        print 'python run_eye_scan.py <esrun|esplot> <label|tarfile> <1|2> <rate/freq>'
        exit(0)
