;; This test may well fail depending on how they implemented
;; load, try again by hand if they said they did it
(define y 12)
(load "test71.scm")
(+ x y)
