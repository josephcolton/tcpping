# tcpping
Utility for performing a TCP based ping

# Installation
You can build the program using the following instructions.

```
make
sudo make install
```

The tcpping utility will be installed in the **/usr/bin/** directory.

# Running tcpping
The tcpping utility is used to test the round trip time RTT latency between your client and a remote server.  Because TCP requires a port number to connect to, tcpping defaults to connecting to TCP port 443 (HTTPS).  It will connect to the remote server using the TCP threeway handshake.  This process requires the tcpping utility to send a SYN packet and wait for the SYN-ACK to return.  At this point, the utility has seen a round trip communication, so it can disconnect and report the time that was consumed in the process.

The basic execution of the program would be to use tcpping to connect to a remote server.  The following would connect to example.com on TCP port 443.

```
tcpping example.com
```

This would start an infinite loop of pings similar to a standard Linux based ping command.  You can use the **Ctrl-C** key sequence to end the program and get statistics.

You can connect to a different port number using the **-p** option.  The following would connect to TCP port 22 (SSH).

```
tcpping -p 22 example.com
```

If you want to indicate the number of TCP pings to send, you can use the **-c** option.

```
tcpping -c 10 example.com
```

The following is a sample execution of running tcpping against a web site renamed **example.com** with **5** probes targetting TCP port **443**.

```
bash$ tcpping -c 5 -p 443 example.com
TCP PING example.com (10.10.10.10) tcp port 443
10.10.10.10: seq=1 time=7.738 ms
10.10.10.10: seq=2 time=7.942 ms
10.10.10.10: seq=3 time=8.488 ms
10.10.10.10: seq=4 time=7.794 ms
10.10.10.10: seq=5 time=7.828 ms
--- example.com tcp ping statistics ---
5 pings, 5 success, 0 failed, 0.0% loss, total run time: 4041.058 ms
rtt min/ave/max/range/jitter = 7.738/7.958/8.488/0.750/0.369 ms
```

# Credits
The tcpping utility was written by Joseph Colton <josephcolton@gmail.com> - https://github.com/josephcolton

# License
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
