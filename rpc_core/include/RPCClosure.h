#ifndef RPCCLOSURE_H
#define RPCCLOSURE_H

#include <functional>
#include <google/protobuf/stubs/common.h>
class RPCClosure : public google::protobuf::Closure
{
public:
	explicit RPCClosure(std::function<void()> cb) : cb_(std::move(cb)) {}

    void Run() override 
    {
        cb_();      
        delete this;
    }

private:
    std::function<void()> cb_;
};

#endif  // RPCCLOSURE_H