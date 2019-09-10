const pipe = require('../')
const readline = require('readline')
const fs = require('fs')

function run_test() {
    var pipes = pipe.PipeDuplex()
    console.log(pipes)

    var childpipes = [ pipes.childread, pipes.childwrite ]
    var parentpipes = [ pipes.parentread, pipes.parentwrite ]
    fs.writeSync(pipes.parentwrite, 'hello world\n')
    input = "world"
    var pid = pipe.rawforkexecclose(childpipes, parentpipes, ['a.out', ''+(pipes.childread), '' + (pipes.childwrite)], input, "./test", 0)
    console.log("pid = " + pid)
    while (typeof(pid) == 'number') {
        //console.log('waiting on pid ' + pid)
        pid = pipe.rawforkexecclose(childpipes, parentpipes, ['a.out', ''+(pipes.childread), '' + (pipes.childwrite)], input, "./test", pid)
    }

    console.log('stdout = ' + pid)
    console.log("returned bytes on " + pipes.parentread + ": " + Buffer.from(pipe.getfdbytes(pipes.parentread)).toString())
    fs.closeSync(pipes.parentread)
    fs.closeSync(pipes.parentwrite)

    setTimeout(run_test, 1000)
}

run_test()
