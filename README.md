<h1 align="center">Multiplayer-Snake-Game</h1>
<p align="center">
  <strong align="center"><i>Simple & Funny!<i></strong>
</p>
<p align="center"> <img src="https://github.com/manhminno/Multiplayer-Snake-Game/blob/master/demo.png"></p>
  
## Table of Contents  

#### [Getting started](#headers) 
#### [Requirement](#2-requirement) 
#### [Usage](#3-usage)
#### [Reference](#4-reference)
#### [Licence](#5-licence)
<br>
<a name="headers"/>

## 1. Getting started:
- Snake is the common name for a video game concept where the player maneuvers a line which grows in length and avoid obstacles.
- This game uses parallel TCP/IP socket server with multithreading and multiprocessing in C together with other Unix/Linux kernel data structures and therefore only functions properly on terminals in a Unix environment.
- This game uses *ncurses* to display game console - a programming library providing an application programming interface (API) that allows the programmer to write text-based user interfaces in a terminal-independent manner.
<a name="2-requirement"/>

## 2. Requirement:
- If ncurses is present, the output should produce two lines with the tag [installed] at the end. If no output is produced, then ncurses is not present and you need to install it by running:
```
sudo apt-get install ncurses-dev
```
- Because it is a game console, only *ncurses* package is needed to run the game. Gcc conpile, socket libraries are available on ubuntu.
<a name="3-usage"/>

## 3. Usage:
- *nguoidung.txt:* file containing user information: username, password, played-time, won-times, ...
> ⇒ In this project I do not use the database to store information, you can integrate database into this project to to store information.
- before run makefile you can edit port to suit your computer which use to run socket/client:
```
server.c: line 13: #define PORT 5500
client.c: line 20: #define PORT 5500

```
*Here I am setting port 5500*
- Run *makefile* and enjoy the game:
```
On server run: ./server
On clients run: ./client (ip of server)
```
> For example, if you run the server and client on the same computer, the ip might be 127.0.0.1. Otherwise, get the *IPv4 address* of the server to connect server.
- Enjoy the game:
```
- Use the keys [W], [A], [S], [D] to move your snake.
- Eat fruit to grow in length.
- Do not run in to other snakes, the game border, the game wall or yourself.
- The first snake to reach length 10 wins!
```
<a name="4-reference"/>

## 4. Reference:
- <strong><a href="https://soict.hust.edu.vn/can-bo/ts-dang-tuan-linh.html">Teacher: Dr. Dang Tuan Linh</a></strong>
- <strong><a href="https://dzone.com/articles/parallel-tcpip-socket-server-with-multi-threading">Multi-threading with TCP/IP</a></strong>
- <strong><a href="https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/">Socket Programming in C/C++</a></strong>
<a name="5-licence"/>

## 5. Licence
* <strong><a href="https://opensource.org/licenses/MIT">MIT License</a></strong>
