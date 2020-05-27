# Chika and other platforms compared

## Benchmarks

| Which            | Arduino      | uLisp `O0` | Chika `O0` | uLisp `O3` | Chika `O3` | CircuitPython |
| ---------------- | ------------ | ---------- | ---------- | ---------- | ---------- | ------------- |
| Fibonacci 15     | Mega 2560    | 512ms      | 651ms      |            |            |               |
| Fibonacci 23     | Mega 2560    | 28s        | 38s        |            |            |               |
| Takeuchi 18 12 6 | Mega 2560    | 49s†       |            |            |            |               |
| Fibonacci 23     | MKRZero      | 12s        | 7,832ms    |            |            |               |
| Takeuchi 18 12 6 | MKRZero      | 18s†       | 9,575ms    |            |            |               |
| Fibonacci 17     | M0 Adalogger | 536ms      | 432ms      | 452ms      | 340ms      | 160ms         |
| Fibonacci 18     | M0 Adalogger | 880ms      | 699ms      | 744ms      | 549ms      | recursion err |
| Takeuchi 10 6 1  | M0 Adalogger | 1,121ms    | 894ms      | 933ms      | 702ms      | 307ms         |
| Takeuchi 18 12 6 | M0 Adalogger | 12,494ms   | 9,410ms    | 10,535ms   | 7,388ms    | recursion err |

† Used `(not (< y x))` instead.  
Note: Mega 2560 Chika benchmarks were before significant performance improvements.

**uLisp functions**

Taken from http://www.ulisp.com/show?1EO1

```cl
(defun fib (n)
  (if (< n 3) 1
    (+ (fib (- n 1)) (fib (- n 2)))))

(defun tak (x y z)
  (if (<= y x) z
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

All uLisp *objects* (either CAR/CDR or type/data) are 4B while Chika *item* descriptors (type/len) are 3B.  
Included as 3+, 4+ or 8+ is the necessary object/item for data recall.
Ranges are a-b.

| What               | uLisp 8/16b | Chika        | uLisp why                           | Chika why                            |
| ------------------ | ----------- | ------------ | ----------------------------------- | ------------------------------------ |
| Minimum size       | 4B          | 3+0B         | Each object is 4B                   | Each item descriptor is 3B           |
| Maximum size       | --          | 3+2^16B      | Uses linked lists                   | Item described len is 16bit          |
| 8-32bit int size   | 4+4B        | 3+1-4B       | Only holds one int32                | Holds either uint8, uint16, or int32 |
| String size        | 4+(len\*2)  | 3+len        | Stores characters in 2/6B of object | Stores strings whole                 |
| List/vector size   | 4+(len\*6)  | 3+(len\*3)+2 | Data is 2B, each link is 4/8B       | Serialised item descriptors + len    |
| Function reference |             | 3+1-2        |                                     | Uses op code or function ID          |

### Programs

| What | uLisp 8/16b   | uLisp 32b     | Chika |
| ---- | ------------- | ------------- | ----- |
| sq   |               | 8B\*13 (104B) | 10B   |
| fib  |               | 8B\*36 (288B) | 45B   |
| tak  | 4B\*48 (192B) | 8B\*44 (352B) | 60B   |