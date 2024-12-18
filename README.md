# one-machine-twitter
This repository contains my project from studies 

# One Machine Bulletin Board

**One Machine Bulletin Board** is a project written in C that simulates a simple version of internet communicator running on a single machine. It consists of two main components: the server (`server.c`) and the client (`client.c`).

## Table of Contents

- [Project Description](#project-description)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Features](#features)
- [Project Structure](#project-structure)
- [Author](#author)

## Project Description

The aim of this project is to demonstrate basic functionalities of a social networking service, such as creating and displaying posts (tweets), all implemented on a single machine.

## Requirements

- POSIX-compatible operating system (e.g., Linux, macOS)
- GCC compiler
- Standard C libraries

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/mateusz-ratkowski/one-machine-twitter.git
   ```
2. Navigate to the project directory:
   ```bash
   cd one-machine-Bulletin Board
   ```
3. Compile the source files:
   ```bash
   gcc -o server server.c
   gcc -o client client.c
   ```

## Usage

1. Start the server:
   ```bash
   ./server server.c [number of possible posts]
   ```
2. In a separate terminal, start the client:
   ```bash
   ./client server.c [N/P] [if N: nickname]
   ```

## Features

- **Server**:
  - Handles connections from multiple clients.
  - Stores and distributes messages (tweets).
- **Client**:
  - Allows users to send messages.
  - Displays received messages.

## Project Structure

- `server.c` - Source code for the server.
- `client.c` - Source code for the client.
- `README.md` - Project documentation.

## Author

Project created by [Mateusz Ratkowski](https://github.com/mateusz-ratkowski).

---

**Note**: This project is a work in progress and may not include all the features typically found in social networking platforms.

