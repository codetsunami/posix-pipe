const pipe = require('../')
const readline = require('readline')
const fs = require('fs')

console.log(pipe)

var pipes = pipe.PipeDuplex()
console.log(pipes)

var childpipes = [ pipes.childread, pipes.childwrite ]
var parentpipes = [ pipes.parentread, pipes.parentwrite ]

fs.writeSync(pipes.parentwrite, 'hello world\n')

input = "world"
var pid = pipe.rawforkexecclose(childpipes, parentpipes, ['c.out', ''+(pipes.childread), '' + (pipes.childwrite)], input, "./test")

console.log("pid = " + pid)

console.log("returned bytes on " + pipes.parentread + ": " + Buffer.from(pipe.getfdbytes(pipes.parentread)).toString())



