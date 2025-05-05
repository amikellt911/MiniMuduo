#pragma once

#include <vector>
#include <string>
namespace llt_muduo {
    namespace net {
        class Buffer{
            public:
                static const size_t kCheapPrepend = 8;//初始预留的元数据空间大小
                static const size_t kInitialSize = 1024;//真正的内存大小

                explicit Buffer(size_t initialSize = kInitialSize)
                    : buffer_(kCheapPrepend + initialSize)
                    , readerIndex_(kCheapPrepend)
                    , writerIndex_(kCheapPrepend)
                {

                }

                size_t readableBytes() const {return writerIndex_ - readerIndex_;}
                size_t writableBytes() const {return buffer_.size() - writerIndex_;}
                size_t prependableBytes() const {return readerIndex_;}

                const char* peek() const {
                    return begin()+readerIndex_;
                }

                void retrieve(size_t len){
                    if(len < readableBytes()){
                        readerIndex_ += len;
                    }else{
                        retrieveAll();
                    }
                }

                void retrieveAll(){
                    readerIndex_ = kCheapPrepend;
                    writerIndex_ = kCheapPrepend;
                }

                std::string retrieveAsString(size_t len){
                    std::string res(peek(),len);
                    retrieve(len);
                    return res;
                }

                std::string retrieveAllAsString(){
                    return retrieveAsString(readableBytes());
                }

                void ensureWritableBytes(size_t len){
                    if(writableBytes() < len){
                        makeSpace(len);
                    }
                }

                void append(const char* data,size_t len){
                    ensureWritableBytes(len);
                    std::copy(data,data+len,begin()+writerIndex_);
                    writerIndex_ += len;
                }

                void append(const std::string& str){
                    append(str.data(),str.length());
                }

                void append(const Buffer& buffer){
                    append(buffer.peek(),buffer.readableBytes());
                }

                size_t readFd(int fd,int* saveErrno);

                size_t writeFd(int fd,int* saveErrno);

            private:    

                char* begin(){return &*buffer_.begin();}
                const char* begin () const{return &*buffer_.begin();}

                void makeSpace(size_t len){ 
                    //当剩余空间不够时
                    //即可写和（readindex-元数据，即已读的空间）不足以存储len字节数据
                    if(writableBytes() + prependableBytes() - kCheapPrepend< len){
                        //扩充到writerIndex(元数据+读的空间)+要写的大小
                        buffer_.resize(writerIndex_ + len);
                    }else{
                        //如果空间足够
                        //那么我们需要将那些未读的空间转移到前面
                        //即将readindex到writerindex这一块空间转移到最开始的kcheapprepend处
                        size_t readable = readableBytes();
                        std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
                        readerIndex_ = kCheapPrepend;
                        writerIndex_ = readerIndex_ + readable;
                    }
                }

                std::vector<char> buffer_;
                size_t readerIndex_;
                size_t writerIndex_;
        };
        }
    }

