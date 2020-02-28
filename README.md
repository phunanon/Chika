# Chika

| ![Chika Logo](media/Chika-badge.svg) | Experimental s-expression programming language for both PC and Arduino. |
| -- | -- |

*Chika* is a programming language targeting both Arduino as firmware or Linux as an executable. It facilitates high-level round-robin multi-tasking, loading programs from either an SD card or Linux filesystem.  
Its goal is to lean toward agility over both speed and memory footprint, with a unique stack memory model, LISP-inspired syntax, and MQTT-style internal messaging.  
Its spirit is: decouple everything through inter-task communication.

### Similar projects

- https://github.com/technoblogy/ulisp
- https://github.com/phunanon/FlintOS
- https://github.com/zooxo/aos
- https://github.com/sau412/Arduino_Basic

### Examples

See [core.chi](corpus/programs/core.chi) and the rest of [the corpus](corpus) for more, realistic examples.

    //Calculates Nth term of Fibonacci Sequence
    (fn fib n
      (if (< n 3) 1i
        (+ (fib (- n 1i)) (fib (- n 2i)))))
    (fib 35i) => 832040

    //Returns absolute of a number
    (fn abs n
      (if (< n 0) (* n -1i) n))
    (abs -1234i) => 1234

    //Prints `15`
    (print
      (do a= + b= 10 c= 5
        (a b c)))

    //Prints `Hello!`
    (fn my-print
      (print str))
    (do str= "Hello!"
      (my-print))

    //Filter function, found in core.chi
    //Note: `1st`, `append`, and `odd?` are all implemented in Chika in core.chi
    //Note: `filtered` is an optional argument - `append` accepts nil as a vector
    (fn filter v pred; filtered
      (if (= (len v) 0)
        filtered
        (do f= (1st v)
          (recur (sect v) pred
            (if (pred f)
              (append filtered f)
              filtered)))))
    (filter [0 1 2 3] odd?) => [1 3]


### Compilation and running

#### Arduino

Open `Chika_Arduino.ino` in Arduino IDE, upload to your Arduino.  
Ensure there is an SD card inserted with the file `init.kua`.  
Suitable devices:
- MKRZero

#### Linux

In terminal run `sh compile.sh && ./bin/mOS bin/init.kua`. This also recompiles `corpus/programs/init.chi`, a basic shell.

### Rationale

#### The vision...

This is the philosophy. Both I and [Dylan Eddies](https://github.com/thelazurite) are keen on our own pocket PC's, built from scratch (within reason).  
Its capabilities should enable a user to, unless there's a major update, never need to reflash their device with new firmware. Instead, be able to write, compile, and run programs on the device.  
It doesn't have to be performant, rather agile and comfortable to develop on. Inter-task comms should be available to help compose complex, uncoupled systems.  
It should be able to fit onto something as small as a watch to anything larger, be headless, or support multiple peripherals.  
It should be able to interact with peripherals through its native programs, rather than compiled support with the OS itself.

#### Why are speed and memory not a priority?

I prefer to work toward its stability, agility, and ecosystem before giving any further serious consideration on its data-handling. At the moment I'm not being bashful using frequent abstractions to manage memory.  
I *think* I'm doing okay in managing stack memory right now...

#### Why not real-time scheduling?

Real-time scheduling has been [done to death](https://github.com/search?q=Arduino+RTOS&type=Repositories), and they supply *compile-time* kernels.  
The vision calls for the ability to write, compile, and execute programs without the need of a second device. A simple, run-time-managed language achieves this.  
Also, the vision does *not* call for real-time applications - just applications that eventually do magic.

#### Why use MQTT-style messaging?

[MQTT](https://en.wikipedia.org/wiki/MQTT) is predominant in the IoT world for directing and monitoring devices, and I find programs on a computer to be no less important. This will hopefully facilitate drop-in, many-to-many relationships of logic and data, being easier to monitor, secure, and compose.

#### Why no REPL, drivers, compiler, &c?

I've chosen to have the REPL, any peripheral drivers, and a compiler written in Chika, rather than a feature of the VM.  
Composition of programs at runtime is its strength, and I much prefer it to messy, baked-in drivers (e.g. "this Arduino OS works with X display and pretty much only that!").  
A really nice example is [SSD1306 support in uLisp](http://library.ulisp.com/show?2TMA).  
Essentially, other than the SD card, serial, and I/O pins, I want no other firmware-level abstractions.

### Chika Compilers

Using a web-browser: open `compile.html`, convert the hex output into a binary image. For Linux use `xxd -r -p chika.hex init.kua`.  
Using NodeJS: `nodejs compiler.js source.chi` => source.kua  
Using Chika: a work in progress!

### Rationale

#### Why not heap or linked-list memory?

With the 1-dimensional mind I have I realised Chika's memory can be implemented as a stack. Cheaper so leaning on C's stack, using recursion when Chika enters a function.  
Using this method, global variables are not practical to implement, with immutability guaranteed.  
Futhermore, heap memory on the Arduino platform is cautioned against, due to code size and memory fragmentation.  
However, the lack of garbage collection and dynamic memory management means items must be duplicated on the stack before processing.  
uLisp uses a linked-list for managing memory, following the traditional implementation of cells. This approach was not used to avoid re-inventing the wheel, and the vision of a leaner memory management model through stack memory. It does pay off in most cases, but operating with big data items - regularly duplicated for processing - means more RAM is required.

#### Why separate program memories?

[*uLisp*](https://github.com/technoblogy/ulisp), again for an example, seems to just allow saving "images" and composing them all together at start-up, sharing memory. I like it, but I like the opportunity of sandboxed areas of memory, with inter-process communication, more.  
Decoupling logic through 0-to-many relationships with other programs has its benefits, especially in avoiding recompilation to accommodate new logic. Rather, programs can just assume there will be a program implementing the logic, by emitting messages.

#### Why a LISP?

I like it. I've been very happy learning Clojure coming from the C++ and C# world. I feel its syntax strikes the perfect balance between person and machine.  
I know other languages are more suitable for microcontrollers (e.g. Forth), and appreciate C *et al* more for what they *don't* compile into an image.
Chika doesn't include homoiconicity, native reader/compiler, macros, laziness, arity, &c., but it succeeds in abstracting away memory management and static typing.

#### Why dynamic variables?

Well, it's a LISP, and I also want to keep the compilation stage simple so it can eventually self-host. It's not too much overhead (3 bytes per item), and most functions expect only certain types.

### Chika Virtual Machine (ChVM) implementation

Using pre-allocated memory to store and execute a variable number of programs at once...
- entry and heartbeat
- state first last on heartbeat
- can be compiled as C++ program
- uses stacks
- uses C/V items

TODO

#### Internal features

- tail-call optimisation

TODO

### Program lifetime

- entry
- heartbeat
- messages

TODO

### Written

#### Syntax

Names can include (almost) any characters excluding whitespace.

`(func[ N args])`: a form, with a function in the head position, and 0-N arguments separated by spaces. Arguments can be forms.

`(fn func-name[ N params] [1-N forms])`: a function definition, with 0-N parameter symbols separated by spaces, and 1-N forms.

`//...`: a comment, which can be suitated on a new line or at the end of one.

`/*...*/`: a multiline comment.

`;`: a semicolon, treated as whitespace.

`,`: a comma, treated as whitespace, and spaces after it are erased.

`..=`: binding, whereby `..` is a symbol name.

Parameters override variables.

Functions must end in a form - to return a value use `val`.

#### Data types

`".."`: string, whereby `..` are 0 to 2^16 ASCII characters, or `""` for empty.

`0`: 8-bit unsigned integer.

`0w`: 16-bit unsigned integer.

`0i`: 32-bit signed integer.

`\c`: ASCII character. Extended: \nl newline.

`[..]`: vector, whereby `..` are 0 to 2^16 items delimited by space, or `[]` for empty.

#### Constants

`args`: emits a vector of function arguments.

#### Native functions

`!`: negates argument.

`+` / `-` / `*` / `/` / `mod` / `&` / `|` / `^` / `<<` / `>>` N arg:  
returns sum / subtraction / multiplication / division / modulus / AND / OR / XOR / left shift / right shift of N integers.  
Zero args returns nil. Will cast all parameters as the type of the first argument.  
`~` 1 arg: returns NOT.  
Examples: `(+ 1 1) => 2`, `(+ 155 200) => 100`, `(+ 155w 200) => 355w`

`if` 2 arg: returns second arg if first arg is truthy, else nil.  
`if` 3 arg: returns second arg if first arg is truthy, else third arg.

`or` N arg: returns first truthy arg.

`and` N arg: returns true if all args truthy.

`recur` N arg: on the stack replace the parameters with N arguments and recall the function.

`=` N arg: equality, true if all args are of the same type, length, and byte equality. Compares ints by value.  
`!=` N arg: negative equality.

`==` N arg: equity, returns true if N items are of byte equality.  
`!==` N arg: negative equity.

`<` / `<=` / `>` / `>=` N arg: returns true if N items are in monotonically increasing / non-decreasing / decreasing / non-increasing order.

`vec` 0-N arg: returns vector of its arguments.

`nth i N`: returns item or character `N` of vector or string `i`, or `nil` if `N` is in an improper range.

(unimplemented) `pin-mode` 2 arg: returns `nil`; sets the mode of the first argument pin number to the second argument boolean - truthy as INPUT, falsey as OUTPUT.

(unimplemented) `dig-out` 2 arg: returns `nil`; sets the digital output state of the first argument pin number to the second argument boolean - truthy as HIGH, falsey as LOW.

(unimplemented) `dig-in` 1 arg: returns digital input state of the first argument pin number.

(unimplemented) `ana-out` 2 arg: returns `nil`; sets the analog/PWM output state of the first argument pin number to the second argument word.

(unimplemented) `ana-in` 1 arg: returns analog input state of the first argument pin number.

`str` 0 arg: returns empty string.  
`str` N arg: returns concatenation of N args as a string.

`type` 1 arg: returns type code of argument.

`cast` 2 arg: returns first argument as the second argument type code.

`len i`: returns either vector, string, or internal item length.

`sect v`: returns `v` with first item omitted;  
`sect v skip`: returns `v` with first `skip` items omitted;  
`sect v skip take`: returns `v` with `take` length and first `skip` items omitted;  
`b-sect`: the same as `sect` but returns items burst.

`burst v`: explodes vector or string `v` onto the argument stack as either vector items or Val_Char items (like Lisp `apply`).

`reduce f[ s*N] i`: returns reduction of vector or string `i` through `f`, with 0-N seeds. `f` is (item acc) => acc.

`map f v*N`: returns mapping of 1-N vectors through `f`, where `f` is (item*N) => mapped.  
Example: `(map str [\a \b \c] [1 2 3]) => [a1 b2 c3]`

`for f v*N`: returns iterative mapping of 1-N vectors through `f`, where `f` is (item*N) => mapped.  
Example: `(for str [\a \b \c] [1 2 3]) => [a1 a2 a3 b1 b2 b3 c1 c2 c3]`

`val` 1-N arg: returns its first argument.

`do` 1-N arg: returns its final argument.

`ms-now` 0 arg: returns milliseconds since Machine initialisation.

`print` 0-N arg: returns `nil`; prints new line of result of `str` of N args.

### Binary structure

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

### Arg- and Op-codes

00 frm form  
TODO

### Argument formats

TODO
