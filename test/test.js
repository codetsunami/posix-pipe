const pipe = require('../')
const readline = require('readline')

console.log(pipe)

var pipes = pipe.PipeDuplex()
console.log(pipes)

var childpipes = [ pipes.childread, pipes.childwrite ]
var parentpipes = [ pipes.parentread, pipes.parentwrite ]

var parentsockets = pipe.PipeSockets( [ pipes.parentread, pipes.parentwrite ] )
const rl = readline.createInterface({
    input: parentsockets[0],
    output: parentsockets[1]
});
rl.on('line', (line) => {
  console.log(`Received: ${line}`);
});

parentsockets[1].write("test msg\n")
var pid = pipe.rawforkexecclose(childpipes, parentpipes, ['./test/a.out', ''+(pipes.childread), '' + (pipes.childwrite)], "", ".")

console.log("pid = " + pid)



