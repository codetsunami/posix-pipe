#if defined(_WIN32) || defined(_MSC_VER)
#define WIN
#endif

#include <sys/ioctl.h>
#include <node.h>
#ifdef WIN
#include <process.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include <v8.h>
#include <nan.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>

#include <map>

using namespace v8;


void Pipe(const FunctionCallbackInfo<Value>& args);

void Pipe(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
#ifdef WIN
    args.GetReturnValue().Set(Undefined(isolate));
#else
    int pipefd[2];
    int ret = pipe(pipefd);
    if (ret == -1) {
        args.GetReturnValue().Set(Undefined(isolate));
        return;
    }
    Local<Array> array = Array::New(isolate, 2);
    Nan::Set(array, 0,Integer::New(isolate,pipefd[0]));
    Nan::Set(array, 1,Integer::New(isolate,pipefd[1]));
    args.GetReturnValue().Set(array);
#endif
}

void _GetFdBytes(int fd, const FunctionCallbackInfo<Value>& args) {

    Isolate* isolate = args.GetIsolate();
#ifdef WIN
    args.GetReturnValue().Set(Undefined(isolate));
#else
 
    int bytes_available = 0;
    ioctl(fd, FIONREAD, &bytes_available);

    if (bytes_available > 0) {
        Local<ArrayBuffer> abuf = ArrayBuffer::New(isolate, bytes_available);
        char* data = (char*)(abuf->GetContents().Data());
        read(fd, data, bytes_available);
        args.GetReturnValue().Set(abuf);
        return; 
    } 

    Local<ArrayBuffer> abuf = ArrayBuffer::New(isolate, 0);
    args.GetReturnValue().Set(abuf);
    return;

#endif

}


/**
    Helper function for non-blocking reads.
    Given an FD check if any bytes are available and return them as a
    Uint8Array if they are available
 
    GetFdBytes(fd)
**/
void GetFdBytes(const FunctionCallbackInfo<Value>& args) {
    if (args.Length() != 1) {
        args.GetReturnValue().Set(Undefined(args.GetIsolate()));
        return;
    }     
    int fd = (int)(args[0].As<Number>()->Value());

    _GetFdBytes(fd, args);
    return;
}


// helper function to hoover up any stdout and set args return value to it
void CollectOutput(int fd, const FunctionCallbackInfo<Value>& args) {
}

/**
  Fork, then exec, then close appropriate FDs based on fork side

  RawForkExecClose(
  int[] childfds,  // to be closed in the parent branch 
  int[] parentfds, // to be clsoed in the child branch
  String[] exec    // exec binary in exec[0] pass args exec[>0],
  String input_data // data to feed into stdin of the child
  String working_directory // the working directory the child should operate under
  int reentrant_pid // optional 
       
        reentrant_pid if specified can be 0 or a pid
        if it is 0 then the function will fork and exec returning the pid of the child immediately
        to harvest the output of the child you will need to call the function again with this pid
        if the child has not yet ended the function will return the same pid again
        if the child has exited then the function will return a string containing its stdout
        this complicated scheme is to allow single threaded applications to poll the status
        of their children rather than maintain a blocking call
  )
 **/
void RawForkExecClose(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
#ifdef WIN
    args.GetReturnValue().Set(Undefined(isolate));
#else

    if (args.Length() < 5 ||
            !args[0]->IsArray() ||
            !args[1]->IsArray() ||
            !args[2]->IsArray() ||
            !(args[3]->IsString() || args[3]->IsUint8Array()) ||
            !args[4]->IsString()
       ) {
        args.GetReturnValue().Set(Undefined(isolate));
        return;
    }

    int reentrant = ( args.Length() == 6 );
    int reentry_pid =  ( reentrant ? args[5].As<Number>()->Value() : 0 );

    int status = 0;
    int pid = -1;

    // this is a map for re-entrant calls
    // pid -> parentread of child process
    static std::map<int, int> reentry_pid_map;

    // set up two additional pipes to redirect input and output from the child
    int child_to_parent[2];
    int parent_to_child[2];

    if (reentrant && reentry_pid) {

        pid = reentry_pid;

        if (reentry_pid_map.find(pid) == reentry_pid_map.end()) {
            args.GetReturnValue().Set(Undefined(isolate));
            return;
        }

        parent_to_child[0] = reentry_pid_map[pid];
        child_to_parent[1] = -1;
        parent_to_child[1] = -1;
        child_to_parent[0] = -1;

        if (!waitpid(pid, &status, WNOHANG)) {
            args.GetReturnValue().Set(Integer::New(isolate, pid));
            return;
        } 

        // execution to here means the process is actually finished, so collect and return output 
        
        // first clean up the map
        reentry_pid_map.erase(pid);

        // now collect output, and store it in the args return val
        _GetFdBytes(parent_to_child[0], args);
        close(parent_to_child[0]);

        // we're done!
        return;
    } 

    if (!pipe(child_to_parent) && !pipe(parent_to_child)) {
        // child_to_parent[0] = childread
        // child_to_parent[1] = parentwrite
        // parent_to_child[0] = parentread
        // parent_to_child[1] = childwrite
    }

    //todo: place an error if pipes don't fire
   

    pid = fork();
    
    if (pid) {
        // parent
        close(child_to_parent[0]);
        close(parent_to_child[1]);

        // if the function is being called as reentrant then
        // store the parent ends of the pipe for next call
        if (reentrant) 
            reentry_pid_map[pid] = parent_to_child[0];


        Local<Array> childfds = Local<Array>::Cast(args[0]);
        int count = childfds->Length();
        for (int i = 0; i < count; ++i) {
            int fd = (int)Nan::Get(childfds, i).ToLocalChecked().As<Number>()->Value();
            close(fd);
        }

        FILE* childinp = fdopen(child_to_parent[1], "wb");
        if (args[3]->IsString()) {
            fprintf(childinp, "%s", *Nan::Utf8String(args[3]));
        } else {
            Local<Uint8Array> input = args[3].As<Uint8Array>();
            char* data = (char*)(input->Buffer()->GetContents().Data());
            int len = input->Length();
            fwrite(data, 1, len, childinp);
        }
        fflush(childinp);
        fclose(childinp);

        // if the function is being called as reentrant we will always return here
        // the user will need to make a second call to collect output!
        if (reentrant) {
            // return the pid
            args.GetReturnValue().Set(Integer::New(isolate, pid));
            return;            
        }
            
        // execution to here means this is a blocking call, not a reentrant call

        waitpid(pid, &status, 0);

        _GetFdBytes(parent_to_child[0], args);
        close(parent_to_child[0]);
        
        return;

    } else {
        // child
        close(child_to_parent[1]);
        close(parent_to_child[0]);

        dup2(child_to_parent[0], 0);
        dup2(parent_to_child[1], 1);
        
        close(child_to_parent[0]);
        close(parent_to_child[1]);

        chdir(*Nan::Utf8String(args[4]));

        Local<Array> parentfds = Local<Array>::Cast(args[1]);
        int count = parentfds->Length();
        for (int i = 0; i < count; ++i) {
            int fd = (int)Nan::Get(parentfds, i).ToLocalChecked().As<Number>()->Value();
            close(fd);
        }

        Local<Array> execargv = Local<Array>::Cast(args[2]);
        count = execargv->Length();

        char** eargv = (char**)malloc(sizeof(char*)*(count+1));

        for (int i = 0; i < count; ++i) {
            char* x = *Nan::Utf8String(Nan::Get(execargv, i).ToLocalChecked());
            eargv[i] = (char*)malloc(strlen(x));
            strcpy(eargv[i], x);
        } 

        eargv[count] = 0;

        execv(eargv[0], eargv);
        printf("ERROR: %d\n", errno);    

        return;
    }

#endif    
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "rawforkexecclose", RawForkExecClose);
    NODE_SET_METHOD(exports, "pipe", Pipe);
    NODE_SET_METHOD(exports, "getfdbytes", GetFdBytes);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

