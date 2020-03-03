# Chika

| ![Chika Logo](docs/media/chika.svg) | Experimental s-expression programming language for both PC and Arduino. |
| -- | -- |

*Chika* is a programming language targeting both Arduino as firmware or Linux as an executable. It facilitates high-level round-robin multi-tasking, loading programs from either an SD card or Linux filesystem.  
Its goal is to lean toward agility over both speed and memory footprint, with a unique stack memory model, Lisp-inspired syntax, and MQTT-style internal messaging.  
Its spirit is: decouple everything through inter-task communication.

**[Visit the website](https://phunanon.github.io/Chika) for more information**, including rationale, comparisons to other projects, idomatic recommendations, and more.

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

    //Returns [15 9], using an inline-function with one argument - `#`
    (map {# 12 3} [+ -])


### Compilation and running

#### Chika VM target: Arduino

Open `Chika_Arduino.ino` in Arduino IDE, upload to your Arduino.  
Ensure there is an SD card inserted with the file `init.kua`.  
Suitable devices:
- MKRZero

#### Chika VM target: Linux

In terminal run `sh compile.sh && ./bin/mOS bin/init.kua`. This also recompiles `corpus/programs/init.chi`, a basic shell.

#### Chika compilers

Using a web-browser: open `compile.html`, convert the hex output into a binary image. For Linux use `xxd -r -p chika.hex init.kua`.  
Using NodeJS: `nodejs compiler.js source.chi` => source.kua  
Using Chika: a work in progress!

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

`{form}`: an inline-function, comprised as one form.

`#`: first argument reference within an inline-function.  
`#num`: positional argument reference within an inline-function, e.g. `#3`.

`//...`: a comment, which can be suitated on a new line or at the end of one.

`/*...*/`: a multiline comment.

`;`: a semicolon, treated as whitespace.

`,`: a comma, treated as whitespace, and whitespace after it is erased.

`..=`: binding, whereby `..` is a label.  
Note: parameters take precedent over bindings.  
Note: when *redefining* bindings, the binding **must** be the very first argument mentioned after the binding declaration. E.g. `a= (+ a 1)` works, but `a= (+ 1 a)` will not as `1` would be the next item on the ChVM stack after the binding. Consider instead using a different binding label.

Parameters override variables.

Functions must end in a form - to return a value use `val`.

#### Data types

`".."`: string, whereby `..` are 0 to 2^16 ASCII characters, or `""` for empty.  
Use `\dq` and `str` for double-quotations, as strings provide no escaped characters.

`0`: 8-bit unsigned integer.

`0w`: 16-bit unsigned integer.

`0i`: 32-bit signed integer.

`\c`: ASCII character.  
Extended: \nl newline, \sp space, \bs backslash, \dq double-quotations, \cm comma, \sc semicolon.

`[..]`: vector, whereby `..` are 0 to 2^16 items delimited by space, or `[]` for empty.

#### Constants

`args`: emits a vector of function arguments.

#### Native functions

`+` / `-` / `*` / `/` / `mod` / `pow` /  
`&` / `|` / `^` / `<<` / `>>` N arg:  
returns sum / subtraction / multiplication / division / modulus / raise-to-the-power /  
AND / OR / XOR / left shift / right shift of N integers.  
Zero args returns nil. Will cast all parameters as the type of the first argument.  
`~` 1 arg: returns NOT.  
Examples: `(+ 1 1) => 2`, `(+ 155 200) => 100`, `(+ 155w 200) => 355w`

`if` 2 arg: returns second arg if first arg is truthy, else nil.  
`if` 3 arg: returns second arg if first arg is truthy, else third arg.

`not` 1 arg: negates argument.

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
