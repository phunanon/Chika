
# Idiomatic Chika

## Functional examples

### Destructuring

[var1= 0
 var2= 0]

(fn heartbeat state
  (do (burst state)
    (print var1 " " var2)
    [var1= (+ var1 1)
     var2= (+ var2 2)]))

## Style examples

### Optional args

;  
TODO

### Internal parameters

;; internal params
params ;; internal params
params ; optional params ; internal params