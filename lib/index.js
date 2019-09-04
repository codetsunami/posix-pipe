const {pipe, rawforkexecclose, getfdbytes} = require('../build/Release/posix')
const net = require('net')
const fs =  require('fs');    

function PipeDuplex() {

    /**
        child-parent pipe structure
   
        pipe 0     
        fd0 -- parentread
        fd1 -- childwrite

        pipe 1
        fd2 -- childread
        fd3 -- parentwrite
    **/


    pipefds = {}

    for (var x in [0, 1]) {
        var pipefd = pipe()
            if (!(pipefd instanceof Array)) {
                throw new TypeError('pipes must be an array')
            }
        if (pipefd.length !== 2) {
            throw new TypeError('pipes must contain two elements')
        }
        if (typeof pipefd[0] !== 'number' ||
                typeof pipefd[1] !== 'number') {
            throw new TypeError('pipes elements must be a numbers')
        }
        if (pipefd[0] < 0 ||
                pipefd[1] < 0) {
            throw new TypeError('pipes elements must be valid file descriptors')
        }

        if (x == 0) {
            pipefds['parentread']  = pipefd[0]
            pipefds['childwrite']  = pipefd[1]
        } else {
            pipefds['childread']   = pipefd[0]
            pipefds['parentwrite'] = pipefd[1]
        } 
    }

    return pipefds 
}


function PipeSockets (pipes) {
    const pipefd = (pipes !== undefined) ? pipes : pipe()
        if (!(pipefd instanceof Array)) {
            throw new TypeError('pipes must be an array')
        }
    if (pipefd.length !== 2) {
        throw new TypeError('pipes must contain two elements')
    }
    if (typeof pipefd[0] !== 'number' ||
            typeof pipefd[1] !== 'number') {
        throw new TypeError('pipes elements must be a numbers')
    }
    if (pipefd[0] < 0 ||
            pipefd[1] < 0) {
        throw new TypeError('pipes elements must be valid file descriptors')
    }
    const streams = [
        new net.Socket({
fd: pipefd[0],
allowHalfOpen: true,
readable: true,
writable: false
}),
        new net.Socket({
fd: pipefd[1],
allowHalfOpen: true,
readable: false,
writable: true
})
    ]
    return streams
    }

module.exports = {}
module.exports.PipeSockets = PipeSockets
module.exports.PipeDuplex = PipeDuplex
module.exports.pipe = pipe
module.exports.rawforkexecclose = rawforkexecclose
module.exports.getfdbytes = getfdbytes
