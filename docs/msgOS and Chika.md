#msgOS and Chika

##msgOS

*msgOS* is an Arduino firmware. It facilitates high-level round-robin multi-processing, loading programs primarily from SD card.  
Its goal is to lean toward agility over both speed and memory footprint, driven by a unique, native LISP-inspired language—Chika—and MQTT-style internal messaging.

###msgOS Operating Middleware (msgOM)

- facilitates personal-computer functionality

##Chika

*Chika* is a unique LISP-inspired dialect natively executed by msgOS. It substitutes heap- for stack-style contiguous memory management.

###Machine

Using pre-allocated memory to store and execute a variable number of programs at once...
- can be compiled as C++ program
- uses stacks
- uses CRV items

###Program lifetime

- entry
- heartbeat
- messages

###Written

####Data types

`".."`: string, whereby `..` are 0 to 2^16 ASCII characters, or `""` for empty.

`0`: 32-bit signed integer.

`0W`: 16-bit unsigned integer.

`0B`: 8-bit unsigned integer.

`[..]`: vector, whereby `..` are 0 to 2^16 items delimited by space, or `[]` for empty.

####Native functions

`+` 0 arg: returns `0`.  
`+` N arg: returns sum of N integers.

`str` 0 arg: returns `""`.  
`str` N arg: returns concatenation of N args as a string.

`print` 0 arg: returns `nil`; prints `nil`.  
`print` N arg: returns `nil`; prints result of `str` of N args.

###Compiled

