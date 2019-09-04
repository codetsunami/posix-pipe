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

var a = new Uint8Array(7)
var s = 'hello'
for (var i = 0 ; i < s.length; ++i)
    a[i] = s.charCodeAt(i)-1

a[5] = 10;
a[6] = 0;

//var input = 'bcdefg';

var input = a;

var pid = pipe.rawforkexecclose(childpipes, parentpipes, ['./test/b.out', '3', '4'], input, ".")

console.log("pid = " + pid)



