for F in *.py *.gz ; do diff -u $F ../../eyescanOTC_20140907/otc/Debug/py_scripts/$F ; diff -u $F ../../eyescanVC707_20140715/eyescan/Debug/py_scripts/$F ; done
