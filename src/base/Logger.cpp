#pragma once 

#include "llt_muduo/base/Logger.h"
#include "llt_muduo/base/Timestamp.h"
#include <thread>
#include <sstream>
#include "llt_muduo/base/CurrentThread.h"
namespace llt_muduo{    
    Logger::Logger(){
        globalLogLevel_ = LogLevel::INFO;
        logLock_.clear();
        logFilePath_ = "../build/log/";
        logFileName_ = "log_" + Timestamp::now().toString() + ".txt";
        logFile_.open(logFilePath_ + logFileName_, std::ios::app);
        if(!logFile_.is_open()){
            std::cerr << "Failed to open log file: " << logFileName_ << std::endl;
        }
    }
    Logger::~Logger(){
        logFile_.close();
    }
    void Logger::setGlobalLogLevel(LogLevel level){
        globalLogLevel_ = level;
    }
    void Logger::log(const std::string& msg, LogLevel level){
        while(logLock_.test_and_set(std::memory_order_acquire)){
            {
                std::this_thread::yield();
            }
            if(!logFile_.is_open()){
                std::cerr << "Log file is not open" << std::endl;
                logLock_.clear(std::memory_order_release);
                return;
            }
                    std::string levelStr;
        switch(level){
            case LogLevel::INFO:
                levelStr = "INFO";
                break;
            case LogLevel::FATAL:
                levelStr = "FATAL";
                break;
            case LogLevel::ERROR:
                levelStr = "ERROR";
                break;
            case LogLevel::DEBUG:
                levelStr = "DEBUG";
                break;
        }
        std::string threadId = "";
        std::stringstream ss;
        ss << "ThreadID:[" << CurrentThread::tid() << "] ";
        threadId = ss.str();

        std::string logMessage =" [" + levelStr + "] " + " : [" + Timestamp::now().toString() + "] : " + threadId  + msg + "\n";
        std::cout << logMessage;
        logFile_ << logMessage;
        logFile_.flush();
        logLock_.clear(std::memory_order_release);
    }
}