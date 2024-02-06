#pragma once

#include <sys/types.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/system_properties.h>

#include "../include/nullutils.h"
#include "../include/pnull.h"
#include "../../NullElf/include/nullelf.h"

namespace NullProcess {

    constexpr int READ    = 0x01;
    constexpr int WRITE   = 0x02;
    constexpr int EXECUTE = 0x04;

    typedef struct {
        uintptr_t start;
        uintptr_t end;
        size_t length;
        char perms;
        uintptr_t offset;
        std::string device;
        int inode;
        std::string pathName;
        int arch;
    } Map;


    typedef struct {
        uintptr_t remote_malloc;
        uintptr_t remote_free;
        uintptr_t remote_mmap;
    } Libc;

    typedef struct {
        uintptr_t remote_dlopen;
        uintptr_t remote_dlerror;
        uintptr_t remote_dlsym;

    } Libdl;


    class Process {
    private:
        std::string pkgName;
        pid_t pid;
        std::vector<NullProcess::Map> maps;
        NullProcess::Libdl libdl{};
        NullProcess::Libc libc{};


        static std::vector<NullProcess::Map> getMaps(const pid_t &pid);

        static NullProcess::Map parseMap(const std::string &line);

        static bool processExists(pid_t pid);

        void locateSymbols();

        bool injectLibNB(void* pathAddr, uintptr_t libLR_base);

        void* remoteString(std::string str);

        NullProcess::Map findMap(std::string mapname);

    public:
        Process();

        bool setProcByName(const std::string& pkg);

        bool setProcByPid(pid_t procID);

        bool writeProcessMemory(std::string hex, uintptr_t address);

        std::string readProcessMemory(uintptr_t address, size_t len);

        bool injectLibrary(std::string path);


        template <typename RetT, typename... Args>
        RetT call(uintptr_t address, Args... args) {
            std::vector<uintptr_t> argVec = {NullUtils::handleArg(args)...};


            if(ptrace(PTRACE_ATTACH, this->pid, nullptr, nullptr) == -1) {
                if constexpr (!std::is_void<RetT>::value)
                    return 0;
                else
                    return;
            }
            waitpid(this->pid, nullptr, WUNTRACED);

            uintptr_t inpArgs[argVec.size()];
            for(int i = 0; i < argVec.size(); i++)
                inpArgs[i] = argVec[i];

            uintptr_t retVal = NullTrace::ptraceRemoteCall(this->pid, address, inpArgs, argVec.size());

            if(ptrace(PTRACE_DETACH, this->pid, nullptr, nullptr) == -1) {
                if constexpr (!std::is_void<RetT>::value)
                    return 0;
                else
                    return;
            }
            if constexpr(std::is_pointer<RetT>::value)
                return reinterpret_cast<RetT>(retVal);
            else if constexpr (std::is_void<RetT>::value)
                return;
            else
                return static_cast<RetT>(retVal);
        }

        template <typename RetT, typename... Args>
        RetT callR(uintptr_t retAddr, uintptr_t address, Args... args) {
            std::vector<uintptr_t> argVec = {NullUtils::handleArg(args)...};


            if(ptrace(PTRACE_ATTACH, this->pid, nullptr, nullptr) == -1) {
                if constexpr (!std::is_void<RetT>::value)
                    return 0;
                else
                    return;
            }
            waitpid(this->pid, nullptr, WUNTRACED);

            uintptr_t inpArgs[argVec.size()];
            for(int i = 0; i < argVec.size(); i++)
                inpArgs[i] = argVec[i];

            uintptr_t retVal = NullTrace::ptraceRemoteCall(this->pid, address, inpArgs, argVec.size(), retAddr);

            if(ptrace(PTRACE_DETACH, this->pid, nullptr, nullptr) == -1) {
                if constexpr (!std::is_void<RetT>::value)
                    return 0;
                else
                    return;
            }
            if constexpr(std::is_pointer<RetT>::value)
                return reinterpret_cast<RetT>(retVal);
            else if constexpr (std::is_void<RetT>::value)
                return;
            else
                return static_cast<RetT>(retVal);
        }

        template <typename T>
        T readProcessMemory(uintptr_t address) {
            if(ptrace(PTRACE_ATTACH, this->pid, nullptr, nullptr) == -1)
                return T();

            waitpid(this->pid, nullptr, WUNTRACED);

            T readData;

            if(!NullTrace::ptraceRead(this->pid, address, reinterpret_cast<uint8_t*>(&readData), sizeof(T))) {
                return T();
            }

            if(ptrace(PTRACE_DETACH, this->pid, nullptr, nullptr) == -1)
                return T();

            return readData;
        }

    };

}

