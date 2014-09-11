set chan [socket 192.168.1.99 7]
puts $chan "dbgeyescan"
flush $chan
set number [gets $chan]
puts $number
close $chan

gets stdin

for { set N 0 } { $N < 1 } { incr N } {
set chan [socket 192.168.1.99 7]
puts $chan "dbgeyescan $N"
flush $chan
for { set i 0 } { $i < 190 } { incr i } {
	puts [gets $chan]
}
close $chan
}

#set chan [socket 192.168.1.99 7]
#puts $chan "esread 0"
#flush $chan
#set number [gets $chan]
#puts $number
#close $chan

#gets stdin

#set chan [socket 192.168.1.99 7]
#puts $chan "esread 0 $number"
#flush $chan
#puts [gets $chan]
#close $chan
