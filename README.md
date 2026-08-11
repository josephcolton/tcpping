# tcpping
Utility for performing a TCP based ping

# Installation
```
make
sudo make install
```

The tcpping utility will be installed in **/usr/bin/** and the man page in **/usr/share/man/man1/**.

# Running tcpping
The tcpping utility measures the round-trip time (RTT) latency between your client and a remote server by timing the TCP three-way handshake.  It sends a SYN packet and waits for the SYN-ACK, then immediately closes the connection and reports the elapsed time.

The basic execution connects to a remote host on TCP port 443 (HTTPS):

```
tcpping example.com
```

This starts an infinite loop of pings similar to a standard Linux ping.  Press **Ctrl-C** to stop and see statistics.

# Options

| Option | Default | Description |
|--------|---------|-------------|
| `-4`, `--ipv4` | yes | Use IPv4 |
| `-6`, `--ipv6` | no | Use IPv6 |
| `-a`, `--audible` | off | Audible bell on each ping |
| `-c COUNT`, `--count COUNT` | unlimited | Stop after COUNT pings |
| `-p PORT`, `--port PORT` | 443 | Target TCP port |
| `-i SEC`, `--interval SEC` | 1 | Seconds between pings |
| `-s COUNT`, `--skip COUNT` | 0 | Pings to exclude from statistics |
| `-t SEC`, `--timeout SEC` | 3 | Seconds before timeout |
| `-d MODE`, `--display MODE` | `all` | Output mode: `all`, `stat`, or `clean` |

The `-s`/`--skip` option excludes the first N pings from statistics, which is useful to let the connection warm up before measuring.

The `-d clean` mode outputs minimal key=value lines suitable for scripted parsing.

# Examples

Connect to TCP port 22 (SSH) with a count of 5:

```
tcpping -p 22 -c 5 example.com
```

Connect using IPv6:

```
tcpping -6 example.com
```

Collect 10 pings, skipping the first 3 from statistics, showing only the summary:

```
tcpping -s 3 -c 13 -d stat example.com
```

Sample output for 5 pings to TCP port 443:

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

# Exit Status
Following the convention set by `ping`, the exit code reflects reachability so that `tcpping` composes cleanly in shell scripts:

| Code | Meaning |
|------|---------|
| `0` | At least one tcp ping succeeded |
| `1` | The run completed but no tcp ping succeeded (100% loss, or interrupted before any reply) |
| `2` | An error prevented the pings from running (invalid arguments, hostname lookup failure, or socket error) |

For example:

```
tcpping -p 22 -c 5 example.com && echo up || echo down
```

# Credits
The tcpping utility was written by Joseph Colton <josephcolton@gmail.com> - https://github.com/josephcolton

# License
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
