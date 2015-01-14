/*
 * Standard Streamer Plugin for Mod Loader
 * Copyright (C) 2014  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under GNU GPL v3, see LICENSE at top level directory.
 *
 */
#include <stdinc.hpp>
#include "streaming.hpp"

// TODO for MODELFILE, TEXDICTION and other entries related to the files the streaming handles

extern "C"
{
    void* (*ColModelPool_new)(int)  = nullptr;
    extern void HOOK_LoadColFileFix(int);
};

static void FixColFile();


void CAbstractStreaming::DataPatch()
{
    if(gvm.IsSA()) FixColFile(); // Fix broken COLFILE in GTA SA
}


/*
 *  FixColFile
 *      Fixes the COLFILE from gta.dat not working properly, crashing the game.
 */
void FixColFile()
{
    static bool using_colbuf;           // Is using colbuf or original buffer?
    static bool empty_colmodel;         // Is this a empty colmodel?
    static std::vector<char> colbuf;    // Buffer for reading col file into

    // Prototype for patches
    using rcolinfo_f  = int(void*, uint32_t*, size_t);
    using rcolmodel_f = int(void*, char*, size_t);
    using lcol_f      = int(char*, int, void*, char*);
    using rel_f       = int(void*);

    // Fixes the crash caused by using COLFILE for a building etc
    ColModelPool_new = MakeCALL(0x5B4F2E, HOOK_LoadColFileFix).get();

    // Reads collision info and check if we need to use our own collision buffer
    auto ReadColInfo = [](std::function<rcolinfo_f> Read, void*& f, uint32_t*& buffer, size_t& size)
    {
        auto r    = Read(f, buffer, size);
        empty_colmodel = (buffer[1] <= 0x18);
        if(using_colbuf = !empty_colmodel)
            colbuf.resize(buffer[1]);
        return r;
    };

    // Replace buffer if necessary
    auto ReadColModel = [](std::function<rcolmodel_f> Read, void*& f, char*& buffer, size_t& size)
    {
        if(!empty_colmodel)
            return Read(f, using_colbuf? colbuf.data() : buffer, size);
        return 0;
    };

    // Replace buffer if necessary
    auto LoadCollisionModel = [](std::function<lcol_f> LoadColModel, char*& buf, int& size, void*& colmodel, char*& modelname)
    {
        return LoadColModel(using_colbuf? colbuf.data() : buf, size, colmodel, modelname);
    };

    // Frees our buffer
    auto ReleaseBuffer = [](std::function<rel_f> fclose, void*& f)
    {
        colbuf.clear(); colbuf.shrink_to_fit();
        return fclose(f);
    };

    // Patches
    make_static_hook<function_hooker<0x5B4EF4, rcolmodel_f>>(ReadColModel);
    make_static_hook<function_hooker<0x5B4E92, rcolinfo_f>>(ReadColInfo);
    make_static_hook<function_hooker<0x5B4FCC, rcolinfo_f>>(ReadColInfo);
    make_static_hook<function_hooker<0x5B4FA0, lcol_f>>(LoadCollisionModel);
    make_static_hook<function_hooker<0x5B4FB5, lcol_f>>(LoadCollisionModel);
    make_static_hook<function_hooker<0x5B4F83, lcol_f>>(LoadCollisionModel);
    make_static_hook<function_hooker<0x5B92F9, rel_f>>(ReleaseBuffer);
}