for F in *.h *.c ; do diff -u $F ../../eyescanOTC_20140907/otc/src/$F ; diff -u $F ../../eyescanVC707_20140715/eyescan/src/$F ; done
