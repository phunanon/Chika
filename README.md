# msgOS and Chika

## msgOS

*msgOS* is an Arduino firmware. It facilitates high-level round-robin multi-tasking, loading programs primarily from SD card.  
Its goal is to lean toward agility over both speed and memory footprint, driven by a unique, native LISP-inspired language—Chika—and MQTT-style internal messaging.  
Its spirit is: decouple everything through inter-task communication.

### Similar projects

- https://github.com/technoblogy/ulisp
- https://github.com/phunanon/FlintOS
- https://github.com/zooxo/aos
- https://github.com/sau412/Arduino_Basic

### Compiling and running msgOS

#### Arduino

To use msgOS on Arduino open `msgOS.ino` on Arduino IDE, upload to your Arduino.  
Suitable devices:
- MKRZero

#### Linux

To use msgOS on Linux run `sh compile.sh && ./mOS`.

### Rationale

#### The vision...

This is the philosophy. Both I and [Dylan Eddies](https://github.com/thelazurite) are keen on our own pocket PC's, built from scratch (within reason).  
Its capabilities should enable a user to, unless there's a major update, never need to reflash their device with new firmware. Instead, be able to write, compile, and run programs on the device.  
It doesn't have to be performant, rather agile and comfortable to develop on. Inter-task comms should be available to help compose complex, uncoupled systems.  
It should be able to fit onto something as small as a watch to anything larger, be headless, or support multiple peripherals.  
It should be able to interact with peripherals through its native programs, rather than compiled support with the OS itself.

#### Why Arduino?

It's cheap and cool! I have no experience coding anything else, and it's a very well-defined, accessible platform. And its use of C++ enables msgOS to be emulated on Linux PC too.

#### Why are speed and memory not a priority?


#### Why a native programming language? Why not real-time scheduling?

I'll answer these two at once: real-time scheduling has been [done to death](https://github.com/search?q=Arduino+RTOS&type=Repositories), and they supply *compile-time* kernels.  
The vision calls for the ability to write, compile, and execute programs without the need of a second device. A simple, run-time-managed native language achieves this.  
Also, the vision does *not* call for real-time applications - just applications that eventually do magic.

#### Why use MQTT-style messaging?


#### Why no REPL, drivers, compiler, &c?


### msgOS Operating Middleware (msgOM)

- facilitates personal-computer functionality

## Chika

*Chika* is a unique LISP-inspired language natively executed by msgOS. It substitutes heap- for stack-style contiguous memory management.

### Compiling and running

#### Compilers

Using `compile.html` in the browser, convert the hexadecimal output into a binary image. For Linux use `xxd -r -p chika.hex init.kua`.  
Using NodeJS:  
`nodejs compiler.js source.chi` => source.kua

### Rationale

#### Why not heap memory?

With the 1-dimensional mind I have I realised Chika's memory can be implemented as a stack. Cheaper so leaning on C's stack, using recursion when Chika enters a function.  
Not to forget using heap memory in Arduino programs is cautioned against, due to code size and memory fragmentation.  
However, the lack of garbage collection and dynamic memory management means items must be duplicated on the stack before processing.

#### Why separate program memories?

[*uLisp®*](https://github.com/technoblogy/ulisp), for example, seems to just allow saving "images" and composing them all together at start-up, sharing memory. I like it, but I like the opportunity of sandboxed areas of memory, with inter-process communication, more.  
Decoupling logic through 0-to-many relationships with other programs has its benefits, especially in avoiding recompilation to accommodate new logic. Rather, programs can just assume there will be a program implementing the logic, by emitting messages.

#### Why a LISP?

I like it. I've been very happy learning Clojure coming from the C++ and C# world. I feel its syntax strikes the perfect balance between person and machine.  
I know other languages are more suitable for microcontrollers (e.g. Forth), and appreciate C *et al* more for what they *don't* compile into an image.
Chika doesn't include homoiconicity, a built-in compiler, macros, &c., but it succeeds in abstracting away memory management and static typing.

#### Why dynamic variables?

Well, it's a LISP, and I also want to keep the compilation stage simple. It's not too much overhead (3 bytes per item), and most functions expect only certain types.

### Machine

Using pre-allocated memory to store and execute a variable number of programs at once...
- can be compiled as C++ program
- uses stacks
- uses C/V items

### Program lifetime

- entry
- heartbeat
- messages

### Written

#### Syntax

Names can include (almost) any characters excluding whitespace.

`(func[ N args])`: a form, with a function in the head position, and 0-N arguments separated by spaces. Arguments can be forms.

`(fn func-name[ N params] [1-N forms])`: a function definition, with 0-N parameter symbols separated by spaces, and 1-N forms.

`;...`: a comment, which can be suitated on a new line or at the end of one.

#### Data types

`".."`: string, whereby `..` are 0 to 2^16 ASCII characters, or `""` for empty.

`0`: 32-bit signed integer.

`0W`: 16-bit unsigned integer.

`0B`: 8-bit unsigned integer.

`[..]`: vector, whereby `..` are 0 to 2^16 items delimited by space, or `[]` for empty.

#### Native functions

`val` 1-N arg: returns its first argument.

`+` 0 arg: returns `0`.  
`+` N arg: returns sum of N integers.

`vec` 0 arg: returns empty vector.  
`vec` N arg: returns vector of N args.

`nth vec N`: returns item `N` of vector `vec`.

`str` 0 arg: returns `""`.  
`str` N arg: returns concatenation of N args as a string.

`print` 0 arg: returns `nil`; prints `nil`.  
`print` N arg: returns `nil`; prints result of `str` of N args.


### Compilation

#### Using the reference compiler

TODO - currently compiler.html

#### Binary format

##### Structure

A compiled Chika binary is composed solely of **functions**. Functions contain **forms**. Forms contain **args** (which can also be forms) and end with an **operation**. The hexadecimal byte formats are:

**function**  
`NNNNLLLL..`  
`NNNN`, uint16_t incrementing function ID; `LLLL`, uint16_t function body length; `..`, `LLLL`-length function body.

**form**  
`00..args..OO`  
`00`, form marker; `[args]`, 0-N args; `OO` an operation.

**arg**  
a form, or `AA..`  
`AA`, uint8_t arg-code; `..` variable-sized bespoke argument body.

**operation**  
`OO`  
`OO`, uint8_t op-code.

##### Arg- and Op-codes

00 frm form  
01 str string  
TODO

##### Argument formats

TODO

### Examples

	(fn f a b (+ a b))
	(print "Hello, world!")
	(print (f 1 2))

