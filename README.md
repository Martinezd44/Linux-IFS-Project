# IFS - Mini Linux File System Simulator

Welcome to **IFS**, a simple Linux-like file system simulation implemented as a project in C.  
This project allows users to explore basic Linux commands in a controlled environment and learn about file system operations.

---

## Features

The following commands are implemented:

| Command  | Description |
|----------|-------------|
| `cd`     | Change directory (exact path required) |
| `ls`     | List files and directories |
| `pwd`    | Print working directory |
| `mkdir`  | Create a new directory |
| `touch`  | Create a new empty file |
| `rm`     | Delete a file |
| `cat`    | Display contents of a file |
| `stat`   | Show file or directory information |
| `whoami` | Show current user ID |

> **Note:** `cd ..` (parent directory) is **not supported** in this version. Paths must be typed exactly.

---

## Getting Started

### Prerequisites

This program is written for **Linux/Ubuntu**. To run it on Windows, you need to install [WSL (Windows Subsystem for Linux)](https://learn.microsoft.com/en-us/windows/wsl/install).

---

### Compilation

Navigate to the project folder and compile using:

```bash
make all
Running the Program
./ifs -u <userID> -f hardDrive
-u <userID>: Specify the user ID (try 1 as default if unsure).

-f hardDrive: Path to the hardDrive file (included in this repo).
