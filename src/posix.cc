#if defined(_WIN32) || defined(_MSC_VER)
#define WIN
#endif

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


/**
  Fork, then exec, then close appropriate FDs based on fork side

  RawForkExecClose(
  int[] childfds,  // to be closed in the parent branch 
  int[] parentfds, // to be clsoed in the child branch
  String[] exec    // exec binary in exec[0] pass args exec[>0]
  )
 **/
void RawForkExecClose(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
#ifdef WIN
    args.GetReturnValue().Set(Undefined(isolate));
#else

    if (args.Length() != 3 ||
            !args[0]->IsArray() ||
            !args[1]->IsArray() ||
            !args[2]->IsArray()
       ) {
        args.GetReturnValue().Set(Undefined(isolate));
        return;
    }


    int pid = fork();
    if (pid) {
        // parent
        Local<Array> childfds = Local<Array>::Cast(args[0]);
        int count = childfds->Length();
        for (int i = 0; i < count; ++i) {
            int fd = (int)Nan::Get(childfds, i).ToLocalChecked().As<Number>()->Value();
            close(fd);
        }
    } else {
        // child
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
            //printf("%s\n", eargv[i]);
        } 

        eargv[count] = 0;

        execv(eargv[0], eargv);
        printf("ERROR: %d\n", errno);    
    }

    args.GetReturnValue().Set(Integer::New(isolate, pid));
#endif    
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "rawforkexecclose", RawForkExecClose);
    NODE_SET_METHOD(exports, "pipe", Pipe);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

