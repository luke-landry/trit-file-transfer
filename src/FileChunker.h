#ifndef FILE_CHUNKER_H
#define FILE_CHUNKER_H

#include <vector>
#include <filesystem>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"
#include "utils.h"
#include "TransferRequest.h"

class FileChunker {
    public:
        void start(const TransferRequest& transfer_request, BoundedThreadSafeQueue<Chunk>& out_queue);
        
    private:
    


};

#endif