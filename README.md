# Posix-Pipe-Fork-Exec

[`posix-pipe-fork-exec`] is a `node.js` module providing missing POSIX pipe streams functionality.

## Usage
```javascript
var pipe = require('posix-pipe');

var streams = pipe();

// streams[0] is readable net.Socket, streams[1] is writable one
```

## Forking
```javascript
const readline = require('readline')

// create two half-duplex pipes
var pipes = pipe.PipeDuplex()

// flatten these into arrays for appropriate end-closing by the fork call
var childpipes = [ pipes.childread, pipes.childwrite ]
var parentpipes = [ pipes.parentread, pipes.parentwrite ]

// prepare parent pipe ends by wrapping them with socket class
var parentsockets = pipe.PipeSockets( [ pipes.parentread, pipes.parentwrite ] )

const rl = readline.createInterface({
    input: parentsockets[0],
    output: parentsockets[1]
});
rl.on('line', (line) => {
  console.log(`Received: ${line}`);
});

// send some test input to the child
parentsockets[1].write("test msg\n")

// run a fork exec on the specified binary
// this will close specified child pipes on the parent side of the fork and
// specified parent pipes on the child side of the fork in addition the final
// string parameter will be fed into stdin of the child and stdout will be
// collected and returned as a result of the function call
// this call is blocking
var output = pipe.rawforkexecclose(childpipes, parentpipes, ['./test/a.out', ''+(pipes.childread), '' + (pipes.childwrite)], "")

console.log("child ran, here's the output: \n" + output)
```

## Want raw FDs?
Just use it!
```javascript
var pipe = require('posix-pipe');

var streams = pipe.pipe(); // [ 23, 24 ] etc.
```
