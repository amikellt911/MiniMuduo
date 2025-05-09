#include"MiniMuduo/net/Buffer.h"
#include<sys/uio.h>
#include<unistd.h>
namespace MiniMuduo
{
    namespace net
    {
        size_t Buffer::readFd(int fd,int* saveErrno)
        {
            char extraBuf[65536]={0};

            struct iovec vec[2];
            const size_t writable=writableBytes();

            vec[0].iov_base=begin()+writerIndex_;
            vec[0].iov_len=writable;

            vec[1].iov_base=extraBuf;
            vec[1].iov_len=sizeof(extraBuf);

            const ssize_t n=::readv(fd,vec,2);

            if(n<0){
                *saveErrno=errno;
            }
            else if(n<=writable){
                writerIndex_+=n;
            }
            else{
                writerIndex_=buffer_.size();
                append(extraBuf,n-writable);

            }
            return n;
        }

        size_t Buffer::writeFd(int fd,int* saveErrno)
        {
            size_t readable=readableBytes();
            ssize_t n=::write(fd,peek(),readable);
            if(n<0){
                *saveErrno=errno;
                return 0;
            }
            //retrieve(n);
            //数据流更新的决策应该在handwrite中进行
            return n;
        }
    }

}