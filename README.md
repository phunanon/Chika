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

#### Online

[![Run on Repl.it](https://repl.it/badge/github/phunanon/msgOS)](https://repl.it/github/phunanon/msgOS)

#### Arduino

To use msgOS on Arduino open `msgOS.ino` on Arduino IDE, upload to your Arduino.  
Ensure there is an SD card inserted with the file `init.kua`.  
Suitable devices:
- MKRZero

#### Linux

To use msgOS on Linux run `sh compile.sh && ./bin/mOS bin/init.kua`. This recompiles and runs `Chika/programs/init.chi`, a basic shell.

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

I prefer to work toward its stability, agility, and ecosystem before giving any further serious consideration on its data-handling. At the moment I'm not being bashful using frequent abstractions to manage memory.  
I *think* I'm doing okay in managing stack memory... I can't visualise a cleaner solution to duplicating items on the stack for processing.

#### Why a native programming language? Why not real-time scheduling?

Real-time scheduling has been [done to death](https://github.com/search?q=Arduino+RTOS&type=Repositories), and they supply *compile-time* kernels.  
The vision calls for the ability to write, compile, and execute programs without the need of a second device. A simple, run-time-managed native language achieves this.  
Also, the vision does *not* call for real-time applications - just applications that eventually do magic.

#### Why use MQTT-style messaging?

MQTT messages are predominant in the IoT world for connecting and monitoring devices, and I find programs on a computer to be no less important.  
I want to bring that abstraction at an OS-level, facilitating drop-in, many-to-many relationships of logic and data. It is easy to monitor, secure, and compose.

#### Why no REPL, drivers, compiler, &c?

I've chosen to have the REPL, any peripheral drivers, and a compiler into Chika-space. That is, they will have to be written in Chika, and will not come with the system as standard.  
Composition of programs at runtime is its strength, and I much prefer it to messy, baked-in drivers (e.g. "this OS works with X screen and pretty much only that!").  
A really nice example is [SSD1306 support in uLisp](http://library.ulisp.com/show?2TMA).  
Essentially, other than the SD card, serial, and I/O pins, I want no other msgOS-level abstractions.

### msgOS Operating Middleware (msgOM)

- facilitates personal-computer functionality

## Chika

*Chika* is a unique LISP-inspired language natively executed by msgOS. It substitutes heap- for stack-style contiguous memory management.

### Examples

    ;Calculates Nth term of Fibonacci Sequence
    (fn fib n
      (if (< n 3) 1I
        (+ (fib (- n 1I)) (fib (- n 2I)))))

    ;Returns absolute of a number
    (fn abs n
      (if (< n 0) (* n -1I) n))

    ;Prints `15`
    (print
      (do a= 10 b= 5
        (+ a b)))

    ;Prints `Hello!`
    (fn my-print
      (print str))
    (do str= "Hello!"
      (my-print))

    ;Prints `2 1`
    (do a= (if true + -)
        b= abs
      (print (a 1 1) " " (b -1I)))

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
Chika doesn't include homoiconicity, native reader/compiler, macros, laziness, arity, &c., but it succeeds in abstracting away memory management and static typing.

#### Why dynamic variables?

Well, it's a LISP, and I also want to keep the compilation stage simple so it can eventually self-host. It's not too much overhead (3 bytes per item), and most functions expect only certain types.

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

`//...`: a comment, which can be suitated on a new line or at the end of one.

`/*...*/`: a multiline comment.

`,`: a comma, treated as whitespace.

`..=`: binding, whereby `..` is a symbol name.

Parameters override variables.

Unmodified parameters must be returned through `val`.

#### Data types

`".."`: string, whereby `..` are 0 to 2^16 ASCII characters, or `""` for empty.

`0`: 8-bit unsigned integer.

`0W`: 16-bit unsigned integer.

`0I`: 32-bit signed integer.

`[..]`: vector, whereby `..` are 0 to 2^16 items delimited by space, or `[]` for empty.

#### Native functions

Note: mathematical functions will cast all arguments as the type of the first argument.

`+` / `-` / `*` / `/` / `mod` N arg: returns sum / subtraction / multiplication / division / modulus of N integers. Zero args returns nil.

`if` 2 arg: returns second arg if first arg is truthy, else nil.  
`if` 3 arg: returns second arg if first arg is truthy, else third arg.

`or` N arg: returns first truthy arg.

`and` N arg: returns true if all args truthy.

`=` N arg: equality, returns true if N items are of the same type, length, and byte equality.

`==` N arg: equity, returns true if N items are of byte equality.

`<` / `<=` / `>` / `>=` N arg: returns true if N items are in monotonically increasing / non-decreasing / decreasing / non-increasing order.

`vec` 0 arg: returns empty vector.  
`vec` N arg: returns vector of N args.

`nth vec N`: returns item `N` of vector `vec`.

`len` 1 arg: returns either vector, string, or internal item length.

`str` 0 arg: returns `""`.  
`str` N arg: returns concatenation of N args as a string.

`len` 1 arg: returns length of vector parameter.

`burst v`: explodes vector `v` onto the argument stack (like Lisp `apply`).

`reduce f[ s*N] v`: returns reduction of vector `v` through `f`, with 0-N seeds. `f` is (item acc) => acc.

`map f v*N`: returns mapping of 1-N vectors through `f`, where `f` is (item*N) => mapped.

`val` 1-N arg: returns its first argument.

`do` 1-N arg: returns its final argument.

`ms-now` 0 arg: returns milliseconds since Machine initialisation.

`print` 0 arg: returns `nil`; prints `nil`.  
`print` N arg: returns `nil`; prints result of `str` of N args.

### Compilation

#### Structure

A compiled Chika binary is composed solely of **functions**. Functions contain **forms**. Forms contain **args** (which can also be forms) and end with an **operation**. The hexadecimal byte formats are:

**function**  
`NNNNLLLL..`  
`NNNN`, uint16_t incrementing function ID; `LLLL`, uint16_t function body length; `..`, `LLLL`-length function body.

**form**  
`00..args..OO`  
`00`, form marker; `[args]`, 0-N args; `OO` an operation.

**arg**  
or `AA..`, a form  
`AA`, uint8_t arg-code; `..` variable-sized bespoke argument body.

**operation**  
`OO`  
`OO`, uint8_t op-code.

#### Arg- and Op-codes

00 frm form  
TODO

#### Argument formats

TODO