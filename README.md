# ITT440: Daytime Protocol (RFC 867) — Cross-Platform Client-Server Implementation

**Universiti Teknologi MARA (UiTM)**<br>
**Course:** ITT440 — Network Programming <br>
**Student Name:** Ahmad Solehudin Bin Muhammad <br>
**Student ID:** 2025485944 <br>
**Group:** NBCS25555A

---

## 1. Overview
This project implements the **Daytime Protocol (RFC 867)** on **well-known port 13**, using a cross-platform client-server design:

* **Server (`server.c`):** C with **POSIX sockets**, running on **Ubuntu (WSL2)**. It uses `select()` I/O multiplexing to serve **TCP and UDP** requests on port 13 from a single process.
* **Client (`client.c`):** C with **Winsock2**, compiled with **MinGW** on Windows 11. It has an interactive menu to query the time over TCP or UDP and measures Round-Trip Time (RTT).

### How RFC 867 works (basic behaviour)
| Transport | Behaviour |
|---|---|
| **TCP** | Client connects to port 13. Server sends the current date and time as an ASCII string and closes the connection. Any data sent by the client is ignored by the standard protocol. |
| **UDP** | Client sends any datagram (even empty) to port 13. Server replies with one datagram containing the date and time string. |

The RFC does not fix an exact string format; this project uses `Weekday, Month DD, YYYY HH:MM:SS TZ`.

### Extended features (beyond RFC 867)
1. **Dual-transport server:** one process listens on both `SOCK_STREAM` and `SOCK_DGRAM` on port 13 using `select()`.
2. **Custom formats:** the client can send a keyword (`MYT`, `UTC`, `ISO`, `UNIX`) to get Malaysia Time, UTC, an ISO-8601 timestamp (`+08:00`), or a Unix epoch timestamp.
3. **RTT measurement:** the client uses `QueryPerformanceCounter` to measure response latency in milliseconds.
4. **Server audit log:** every request is logged with transport (`[TCP]`/`[UDP]`), client IP, ephemeral port, and the payload sent.
5. **Backward compatibility with standard clients:** the TCP handler waits only 50 ms (`SO_RCVTIMEO`) for an optional keyword. Standard clients such as `nc`, which send nothing, still receive the time immediately.

---

## 2. Project Structure
```text
ITT440_Daytime/
├── server.c      # Ubuntu (WSL2) POSIX socket server — TCP + UDP on port 13
├── client.c      # Windows (MinGW) Winsock2 interactive client
└── README.md     # Documentation
```

---

## 3. How to Compile & Run

### Step 1 — Start the server (Ubuntu / WSL2)
Port 13 is a privileged port (< 1024), so the server must run as root:
```bash
gcc server.c -o server
sudo ./server
```
Find the WSL2 IP address:
```bash
hostname -I
```

> **Timezone assumption:** the server uses the system timezone for the `MYT` and `ISO` modes. The host must be set to Malaysia time (UTC+8). Check with `date`.

### Step 2 — Compile and run the client (Windows / MinGW)
```cmd
gcc client.c -o client.exe -lws2_32
client.exe
```
Enter the WSL2 IP address when prompted, then choose options `1`–`5`.

### Client menu
| Option | Transport | Mode sent | Output |
|---|---|---|---|
| 1 | TCP | `MYT` | Standard daytime, Malaysia time |
| 2 | UDP | `MYT` | Standard daytime, Malaysia time |
| 3 | TCP | `UTC` | UTC time |
| 4 | UDP | `ISO` | ISO-8601 timestamp (`+08:00`) |
| 5 | TCP | `UNIX` | Unix epoch seconds |

---

## 4. Networking Note: WSL2 and UDP
During testing, the Windows client could reach the server over **TCP** on `127.0.0.1` (WSL2 localhost forwarding), but **UDP on `127.0.0.1` timed out**. Connecting to the WSL2 virtual interface IP from `hostname -I` (e.g. `192.168.x.x`) works for **both TCP and UDP**.

**Always enter the WSL2 IP in the client**, not `127.0.0.1`. Note that the WSL2 IP can change after a restart, so re-run `hostname -I`.

---

## 5. RFC 867 Compliance Verification
A standard client that sends no payload gets the time immediately:
```bash
nc <UBUNTU_IP> 13          # TCP
echo | nc -u -w1 <UBUNTU_IP> 13   # UDP
```

---

## 6. Test Results
Sample RTT from the Windows client to the WSL2 server (single run each, on the same machine):

| Mode | Transport | RTT |
|---|---|---|
| MYT | TCP | ~2.25 ms |
| MYT | UDP | ~1.91 ms |
| UTC | TCP | ~1.74 ms |
| ISO-8601 | UDP | ~1.12 ms |
| Unix epoch | TCP | ~2.03 ms |

**Notes:**
* The TCP timer starts before `connect()`, so TCP RTT **includes the 3-way handshake**; the UDP timer does not. The two are therefore not a like-for-like comparison.
* In this run, UDP was faster than TCP for the same MYT query (1.91 ms vs 2.25 ms), which is expected because UDP has no handshake. These are single measurements on a local virtual network, so the gap is small and varies between runs.

**Server (left, WSL2) and Windows client (right) running side by side:**

![Server and client](screenshots/server_client.png)

The UTC response (09:41) is exactly 8 hours behind MYT (17:41), and the server log shows each `[TCP]`/`[UDP]` request.

---

## 7. Limitations
* **Iterative server:** one TCP client is handled at a time. This is acceptable for Daytime because each request is a single short exchange.
* **IPv4 only.**
* **No `connect()` timeout** in the client: an unreachable IP may take a long time to fail.
* **50 ms keyword window:** a TCP client whose keyword arrives later than 50 ms receives the default MYT response.
* The server assumes the host timezone is Malaysia time (see Section 3).

---

## 8. References
* J. Postel, *Daytime Protocol*, **RFC 867**, May 1983. https://www.rfc-editor.org/rfc/rfc867
* Microsoft Docs, *Winsock 2 API*.
* Linux man pages: `socket(2)`, `select(2)`, `bind(2)`, `recvfrom(2)`.
