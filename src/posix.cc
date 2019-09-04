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
  String[] exec    // exec binary in exec[0] pass args exec[>0],
  String input_data // data to feed into stdin of the child
  String working_directory // the working directory the child should operate under
  )
 **/
void RawForkExecClose(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
#ifdef WIN
    args.GetReturnValue().Set(Undefined(isolate));
#else

    if (args.Length() != 5 ||
            !args[0]->IsArray() ||
            !args[1]->IsArray() ||
            !args[2]->IsArray() ||
            !args[3]->IsString() ||
            !args[4]->IsString()
       ) {
        args.GetReturnValue().Set(Undefined(isolate));
        return;
    }

    // set up two additional pipes to redirect input and output from the child

    int child_to_parent[2];
    int parent_to_child[2];

    if (!pipe(child_to_parent) && !pipe(parent_to_child)) {
        // child_to_parent[0] = childread
        // child_to_parent[1] = parentwrite
        // parent_to_child[0] = parentread
        // parent_to_child[1] = childwrite
    }

    int pid = fork();
    if (pid) {
        // parent
        close(child_to_parent[0]);
        close(parent_to_child[1]);


        Local<Array> childfds = Local<Array>::Cast(args[0]);
        int count = childfds->Length();
        for (int i = 0; i < count; ++i) {
            int fd = (int)Nan::Get(childfds, i).ToLocalChecked().As<Number>()->Value();
            close(fd);
        }

        FILE* childinp = fdopen(child_to_parent[1], "wb");
        fprintf(childinp, "%s", *Nan::Utf8String(args[3]));
        fflush(childinp);
        fclose(childinp);

        FILE* childout = fdopen(parent_to_child[0], "rb");
        std::string output;
        char buffer[1024];
        int status = 0;
        waitpid(pid, &status, 0);
 
        while (fgets(buffer, 1023, childout)) {
            buffer[1023] = '\0';
            output += std::string(buffer);
        }

        args.GetReturnValue().Set(String::NewFromUtf8(
            isolate, output.c_str(), NewStringType::kNormal).ToLocalChecked());

    } else {
        // child
        close(child_to_parent[1]);
        close(parent_to_child[0]);

        dup2(child_to_parent[0], 0);
        dup2(parent_to_child[1], 1);
        
        close(child_to_parent[0]);
        close(parent_to_child[1]);

        chdir(*Nan::Utf8String(args[3]));

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

#endif    
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "rawforkexecclose", RawForkExecClose);
    NODE_SET_METHOD(exports, "pipe", Pipe);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

