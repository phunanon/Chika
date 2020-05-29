# Chika *et al* compared

## Benchmarks

| Machine      | What             | uLisp `O0` | Chika `O0`   | uLisp `O3` | Chika `O3`  | CircuitPython |
| ------------ | ---------------- | ---------- | ------------ | ---------- | ----------- | ------------- |
| Mega 2560    | Fibonacci 15     | **438ms**  | 477ms        | --         | --          | --            |
| Mega 2560    | Fibonacci 23     | 23,448ms   | **22,345ms** | --         | --          | --            |
| Mega 2560    | Takeuchi 10 6 1  | 2,995ms    | **2,365ms**  | --         | --          | --            |
| Mega 2560    | Takeuchi 18 12 6 | 38,110s    | **24,892ms** | --         | --          | --            |
| MKRZero      | Fibonacci 15     | 255ms      | **166ms**    | 224ms      | **130ms**   | --            |
| MKRZero      | Fibonacci 23     | 12,430ms   | **7,810ms**  | 10,874ms   | **6,118ms** | --            |
| MKRZero      | Takeuchi 10 6 1  | 1,358ms    | **886ms**    | 1,170ms    | **697ms**   | --            |
| MKRZero      | Takeuchi 18 12 6 | 15,023ms   | **9,321ms**  | 12,950ms   | **7,331ms** | --            |
| M0 Adalogger | Fibonacci 17     | 536ms      | **435ms**    | 452ms      | 340ms       | **160ms**     |
| M0 Adalogger | Fibonacci 18     | 880ms      | **704ms**    | 744ms      | **551ms**   | recur err     |
| M0 Adalogger | Takeuchi 10 6 1  | 1,121ms    | **885ms**    | 933ms      | 694ms       | **307ms**     |
| M0 Adalogger | Takeuchi 18 12 6 | 12,494ms   | **9,308ms**  | 10,535ms   | **7,303ms** | recur err     |

**uLisp functions**

Taken from http://www.ulisp.com/show?1EO1

```cl
(defun fib (n)
  (if (< n 3) 1
    (+ (fib (- n 1)) (fib (- n 2)))))

(defun tak (x y z)
  (if (<= x y) z
    (tak (tak (1- x) y z) (tak (1- y) z x) (tak (1- z) x y))))

(for-millis () (print #|function call|#))
```

**Chika functions**

```clj
(fn fib n
  (if (< n 3) 1i
    (+ (fib (- n 1i)) (fib (- n 2i)))))

(fn tak x y z
  (if (<= x y) z
    (tak (tak (- x 1) y z) (tak (- y 1) z x) (tak (- z 1) x y))))
```

```clj
(do
  start= (ms-now)
  (print /*function call*/)
  (print "  took " (- (ms-now) start) "ms"))
```

**CircuitPython functions**

```py
def fib (n):
  if n < 3: return 1
  return fib(n - 1) + fib(n - 2)

def tak (x, y, z):
  if x <= y: return z
  return tak(tak(x - 1, y, z), tak(y - 1, z, x), tak(z - 1, x, y))
```

```py
import time
start = time.monotonic()
#function call
end = time.monotonic()
print(str(int((end - start) * 1000)) + "ms")
```


## Memory size

These are informed speculations. I welcome corrections!

### Objects & Items

All uLisp *objects* (either CAR/CDR or type/data) are 4B (8-bit/16-bit) or 8B (32-bit), while Chika *item* descriptors (type/len) are 3B.  
Included as 3+ or 4+ is the necessary object/item for data recall.
Ranges are a-b.

| What               | uLisp 8/16b | Chika        | uLisp why                           | Chika why                            |
| ------------------ | ----------- | ------------ | ----------------------------------- | ------------------------------------ |
| Minimum size       | 4B          | 3+0B         | Each object is 4B                   | Each item descriptor is 3B           |
| Maximum size       | --          | 3+2^16B      | Uses linked lists                   | Item described len is 16bit          |
| 8-32bit int size   | 4+4B        | 3+1-4B       | Only holds one int32                | Holds either uint8, uint16, or int32 |
| String size        | 4+(len\*2)  | 3+len        | Stores characters in 2/6B of object | Stores strings whole                 |
| List/vector size   | 4+(len\*6)  | 3+(len\*3)+2 | Data is 2B, each link is 4/8B       | Serialised item descriptors + len    |
| Function reference |             | 3+1-2        |                                     | Uses op code or function ID          |

### Programs & functions

| What | uLisp 8/16b   | uLisp 32b     | Chika   |
| ---- | ------------- | ------------- | ------- |
| sq   | 4B\*12 (48B)  | 8B\*13 (104B) | **10B** |
| fib  | 4B\*37 (148B) | 8B\*36 (288B) | **45B** |
| tak  | 4B\*48 (192B) | 8B\*44 (352B) | **60B** |