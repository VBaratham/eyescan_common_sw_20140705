#!/usr/bin/python
# -*- coding: utf-8 -*-

import os

def read_dump_file( fname = 'center_error.txt' ) :
    es_points = {}
    #for l in open( 'all.dump' , 'r' ).readlines() :
        #ents = l.split()
        #chan = int(ents[0])
        #samp = int(ents[5])
        #pres = int(ents[6]) & 0x001F
        #cerr = int(ents[8])
        
        #totsamp = samp * ( 40 << ( 1 + pres ) )
        
        #if chan not in es_points :
            #es_points[chan] = [ 1 , 0 ]
        
        #es_points[chan][0] += cerr
        #es_points[chan][1] += samp
    for l in open( fname , 'r' ).xreadlines() :
        ents = l.split()
        chan = int(ents[0])
        cerr = int(ents[1])
        samp = int(ents[2])
        
        if cerr == 0 :
            cerr = 1
        
        es_points[chan] = [ cerr , samp ]
        
    return es_points

def central_error_plot( fname = 'center_error.txt' , title = 'Central Bit Error Rate' ) :
    es_points = read_dump_file( fname )
    
    from ROOT import TGraph , TCanvas , kCircle

    canv = TCanvas()
    gr = TGraph(48)
    for idx in range(0,48) :
        ch = idx
        ber = float(es_points[idx][0])/float(es_points[idx][1])
        gr.SetPoint( idx , idx , ber )
    gr.Draw('AP')
    gr.SetTitle( title )
    gr.SetMarkerStyle(kCircle)
    gr.SetMarkerSize(1)
    gr.GetXaxis().SetTitle('Channel')
    gr.GetYaxis().SetTitle('ber')
    canv.SetLogy()
    canv.Update()
    canv.SaveAs( 'central_ber.pdf' )
    
    return

if __name__ == '__main__' :
    title = 'Central Bit Error Rate'
    fname = 'center_error.txt'
    if len(os.sys.argv) > 1 and os.sys.argv[1] != '-b' :
        title = os.sys.argv[1]
    if len(os.sys.argv) > 2 and os.sys.argv[2] != '-b' :
        fname = os.sys.argv[2]
    central_error_plot( fname , title )
