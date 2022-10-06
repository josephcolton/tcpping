# tcpping
Utility for performing a TCP based ping

# Installation
You can build the program using the following instructions.

'''
make
sudo make install
'''

The tcpping utility will be installed in the /usr/bin/ directory.

# Running tcpping
The tcpping utility is used to test the round trip time RTT latency between your client and a remote server.  Because TCP requires a port number to connect to, tcpping defaults to connecting to TCP port 443 (HTTPS).  It will connect to the remote server using the TCP threeway handshake.  This process requires the tcpping utility to send a SYN packet and wait for the ACK to return.  At this point, the utility has seen a round trip communication, so it can disconnect and report the time that was consumed in the process.

The basic execution of the program would be to use tcpping to connect to a remote server.  The following would connect to example.com on TCP port 443.

'''
tcpping example.com
'''

This would start an infinite loop of pings similar to a standard Linux based ping command.  You can use the **Ctrl-C** key sequence to end the program and get statistics.

You can connect to a different port number using the **-p** option.  The following would connect to TCP port 22 (SSH).

'''
tcpping -p 22 example.com
'''

If you want to indicate the number of TCP pings to send, you can use the **-c** option.

'''
tcpping -c 10 example.com
'''

# Credits
The tcpping utility was written by Joseph Colton - https://github.com/josephcolton
