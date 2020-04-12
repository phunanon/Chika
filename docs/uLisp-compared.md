# uLisp and Chika compared

## Disclaimer

These are informed speculations. I welcome corrections!

## Memory size

### Objects & Items

All uLisp *objects* (either CAR/CDR or type/data) are 4B while Chika *item* descriptors (type/len) are 3B.  
Included as 3+ and 4+ is the necessary object/item for data recall.
Ranges are a-b.

| What               | uLisp       | Chika         | uLisp why                         | Chika why                            |
| ------------------ | ----------- | ------------- | --------------------------------- | ------------------------------------ |
| Minimum size       | 4B          | 3+0B          | Each object is 4B                 | Each item descriptor is 3B           |
| Maximum size       | --          | 3+2^16B       | Uses linked lists                 | Item described len is 16bit          |
| 8-32bit int size   | 4+4B        | 3+1-4B        | Only holds one int32              | Holds either uint8, uint16, or int32 |
| String size        | 4+(len\*2)  | 3+len         | Stores characters in 2B of object | Stores strings whole                 |
| List/vector size   | 4+(item\*6) | 3+(item\*3)+2 | Data is 2B, each link is 4B       | Serialised item descriptors + len    |
| Function reference |             | 3+1-2         |                                   | Uses op code or function ID          |

### Programs

| What | uLisp  | Chika |
| ---- | ------ | ----- |
| sq   | 4B*15  | 10B   |
| fib  | 4B\*37 | 57B   |
| tak  |        | 81B   |

## Benchmarks

http://www.ulisp.com/show?1EO1

Note: Mega 2560 benchmarks were before significant performance improvements.  
Note: all with no compiler optimisations.

| Which            | Arduino   | uLisp | Chika  |
| ---------------- | --------- |------ | ------ |
| Takeuchi 18 12 6 | Mega 2560 | 49s   |        |
| Fibonacci 15     | Mega 2560 | 512ms | 651ms  |
| Fibonacci 23     | Mega 2560 | 28s   | 38s    |
| Fibonacci 23     | MKRZero   | 12s   | 7832ms |
| Takeuchi 18 12 6 | MKRZero   | 18s   | 9575ms |