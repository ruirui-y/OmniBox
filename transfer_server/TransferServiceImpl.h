#ifndef TRANSFER_SERVICE_IMPL_H
#define TRANSFER_SERVICE_IMPL_H

#include "transfer.pb.h"
#include <string>

class TransferServiceImpl : public omnibox::TransferService
{
public:
	// ÉÏ´«ËéÆ¬
    void UploadChunk(::google::protobuf::RpcController* controller,
        const ::omnibox::FileChunkUploadRequest* request,
        ::omnibox::FileChunkUploadResponse* response,
        ::google::protobuf::Closure* done) override;
};

#endif