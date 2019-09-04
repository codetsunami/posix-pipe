const pipe = require('../')
const readline = require('readline')
const fs = require('fs')

console.log(pipe)

var pipes = pipe.pipe()
console.log(pipes)


fs.writeSync(pipes[1], "testing\n")
console.log(Buffer.from(pipe.getfdbytes(pipes[0])).toString())

