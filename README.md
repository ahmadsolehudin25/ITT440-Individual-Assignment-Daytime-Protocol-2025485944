# ITT440: Daytime Protocol (RFC 867) — Cross-Platform Client-Server Implementation

**Universiti Teknologi Mara (UiTM)**  
**Course:** ITT440 — Network Programming  
**Student Name:** Ahmad Solehudin Bin Muhammad  
**Student ID:** 2025485944  
**Group:** NBCS25555A


---

## 1. Overview
This project implements the **Daytime Protocol (RFC 867)** operating on **Well-Known Port 13** using a cross-platform client-server architecture:
* **Server (`server.c`):** Written in C using **POSIX Sockets** on **Ubuntu (WSL2)**. Uses `select()` I/O multiplexing to handle **both TCP and UDP** requests simultaneously on Port 13.
* **Client (`client.c`):** Written in C using **Windows Sockets (Winsock2)** compiled via **MinGW**. Provides an interactive CLI menu to query time over TCP or UDP and measures Round-Trip Time (RTT) latency.

### Extended Features (Beyond Standard RFC 867)
1. **Dual-Transport Support (TCP & UDP):** A single server process listens on both `SOCK_STREAM` (TCP) and `SOCK_DGRAM` (UDP) on Port 13.
2. **Custom Time Formats & Timezones:** Clients can request **Malaysia Time (MYT)**, **UTC**, **ISO-8601 (`+08:00`)**, or **Unix Epoch Timestamps**.
3. **Real-Time RTT Latency Measurement:** Uses Windows `QueryPerformanceCounter` to compare connection-oriented (TCP) vs. connectionless (UDP) response latency.
4. **Live Server Audit Log:** Logs every client IP address, ephemeral port, transport protocol, and payload served.

---

## 2. Project Structure
```text
ITT440_Daytime/
├── server.c      # Ubuntu WSL POSIX socket server (TCP + UDP on Port 13)
├── client.c      # Windows MinGW Winsock2 interactive client
└── README.md     # Documentation and setup guide
```

---

## 3. How to Compile & Run

### Step 1: Start the Server on Ubuntu (WSL2)
Because **Port 13** is a privileged well-known port (`< 1024`), root (`sudo`) privileges are required to bind to it:
```bash
gcc server.c -o server
sudo ./server
```
Find your Ubuntu WSL IP address so the Windows client can reach both TCP and UDP sockets:
```bash
hostname -I
```

### Step 2: Compile & Run the Client on Windows (MinGW)
Link the Winsock2 library (`-lws2_32`) during compilation:
```cmd
gcc client.c -o client.exe -lws2_32
client.exe
```
Enter the Ubuntu WSL IP address when prompted, then choose options `1`–`5` from the menu.

---

## 4. RFC 867 Compliance Verification
Standard Linux utilities such as **Netcat (`nc`)** can connect directly to the TCP port without sending any payload and immediately receive the formatted daytime string before the server closes the connection:
```bash
nc <UBUNTU_IP> 13
```
