
Downloads:
    sudo apt-get install libssl-dev //to download md5




make all - for the program

Part A: Chat
Usage: 
	Server Side: ./stnc -s <PORT>
	Client Side: ./stnc -c <IP> <PORT>
Now you can use the chat tool etween the peers, by writing to terminal that you want.
Close the tool by CTRL+C(SIG INT) from both sides.

Part B: Performence Tests
Usage:
	Server Side: ./stnc -s port -p (p for performance test) -q (q for quiet)
		-p flag will let you know that we are going to test the performance.
		-q flag will enable quiet mode, in which only testing results will be printed.
	Client Side: ./stnc -c IP PORT -p <type> <param>
		Optional 8 combinations:
		You have only 8 combinations:
		ipv4 tcp
		ipv4 udp
		ipv6 tcp
		ipv6 udp
		uds dgram
		uds stream
		mmap filename
		pipe filename

Programmed By: Lior Jerbi & Assaf Shmryahu