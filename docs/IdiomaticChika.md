
# Idiomatic Chika

## Function

### Destructuring

```clj
[var1= 0
 var2= 0]

(fn heartbeat state
  (binds (.. state)
  	(print var1 " " var2)
	  var1= (+ var1 1)
		var2= (+ var2 2)))```

### Closures

```clj
(fn close f
  [f (sect args)])

(fn open c ;
  (do f= (nth 0 c)
	  (f (.. (nth 1 c)) (.. (sect args)))))

(open
  (close {+ #1 #} 15) //Enclose a lambda with arg 0 as `15`
  3)
```

## Style

### Parameters

;; internal params
params ;; internal params
params ; optional params ; internal params