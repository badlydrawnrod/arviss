# Arviss - A Risc-V Instruction Set Simulator

## Dev Diary
I've started keeping a Dev Diary for Arviss. Here are the most recent entries:

[12/9/21 - Table driven processing in Very Angry Robots III](./dev-diary/very-angry-robots/20210912-table-driven.md)

[3/9/21 - Collisions and Events in Very Angry Robots III](./dev-diary/very-angry-robots/20210903-collisions.md)

[1/9/21 - Player Movement in Very Angry Robots III](./dev-diary/very-angry-robots/20210901-movement.md) 

[31/8/21 - Very Angry Robots III](./dev-diary/very-angry-robots/20210831-intro.md)

## History
One of the things that inspired me to write Arviss was an old game named [Robot War](https://en.wikipedia.org/wiki/RobotWar). I never actually played it, but I remember reading about it in *Byte* magazine back in the eighties, and thinking that it was the coolest thing ever. Many years later, I went on to play with [Robocode](https://robocode.sourceforge.io/). It was a lot of fun, and definitely contributed to how I learned Java. But once in a while, my brain would take me back to Robot War with the question, **"What would it take to write such a game in C and ensure that it is still fair?"**

To put it another way, how do you make it possible for people to program simulated robots in C, and have their robots compete against robots developed by other people, while preventing any given robot from using more CPU time or resources than the others?

Eventually I realised that a CPU emulator (or more accurately, an instruction set simulator) could solve this problem, as it would allow for the host program to run multiple emulators while having very tight control of their resource usage. 

With this in mind, I looked for suitable a suitable ISA  to emulate. This needed to be something that current C compilers had good support for, so that immediately ruled out 8-bit favourites such as 6502 and Z80. I came very close to choosing 68000, as there are some great emulators for that, and I have fond memories of programming it, but the C support didn't seem to be that great. And then I heard about [RISC-V](https://en.wikipedia.org/wiki/RISC-V), and discovered that recent versions of both gcc and clang have good support for it, so it was a natural choice.

Of course, there was also the question of which emulator? RISC-V has some very good emulators, but many of them seemed to be focused on emulating hardware, or booting Linux, rather than running potentially hundreds of instances at the same time. I couldn't really find anything that met my needs, and I was increasingly interested in writing an emulator myself, and so Arviss was born.

