/**
* @file target_access_remote.h
* @brief Target memory and register access definitions for remote
* environment
*
* Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/ 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*  Redistributions of source code must retain the above copyright 
*  notice, this list of conditions and the following disclaimer.
*
*  Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the 
*  documentation and/or other materials provided with the   
*  distribution.
*
*  Neither the name of Texas Instruments Incorporated nor the names of
*  its contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
#ifndef __TARGET_ACCESS_REMOTE_H
#define __TARGET_ACCESS_REMOTE_H

#include <limits>
#include "TargetAdapter.h"
#include "Address.h"
#include "TargetAdapter64.h"
#include "td_error.h"
#include "debug_log.h"

using namespace Ti::Sds::Ctools;

/**
* @brief Target memory and register access definition.

* Use address size of 64-bits so we can support both 64-bit and 32-bit
* targets.
*
* The implementation uses template functors for memory/register read and write operation.
* The purpose of the templatization is to avoid writing duplicate code to handle
* 8/16/32/64 bit data accesses.
*
* NOTE: The implementation uses 32-bit version of Target Adapter. 64-bit version
* of Target Adapter is still under design and implementation will be added when
* it becomes available.
*/

/**
* @brief Memory read/write definition
*/
template <typename DATA_WIDTH>
class mem_read {
public:
    mem_read(TargetAdapter *ta, uint8_t page, int *ta_error) :p_ta(ta), mem_page(page), p_ta_error(ta_error){};
    int operator()(uint64_t address, DATA_WIDTH* value) {
        int error = e_ERR_NONE;

        // If the address fits in a 32 bit unsigned and the data size is not 64-bit
        // then use the 32-bit version of the target adapter API
        if (address <= std::numeric_limits<uint32_t>::max() && sizeof(DATA_WIDTH) != sizeof(uint64_t))    {
            unsigned int temp_value = 0;
            // Issue target adapter memory read.
            error = p_ta->MemoryRead(&temp_value, (uint32_t)(address), sizeof(DATA_WIDTH), mem_page);
            *value = (DATA_WIDTH)temp_value;
        }
        else  {    // Use 64-bit version of the target adapter API
            TargetAdapter64* p_ta_64 = reinterpret_cast< TargetAdapter64* >(p_ta->QueryInterface(eTargetAdapter64));
            if (p_ta_64)  {
                uint64_t temp_value[2] = { 0, 0 };
                error = p_ta_64->MemoryRead(temp_value, (uint64_t)(address), sizeof(uint64_t), mem_page);
                *value = (DATA_WIDTH)((temp_value[0] & 0xFFFFFFFF) | ((temp_value[1] & 0xFFFFFFFF) << 32));
            }
            else  { // 64-bit target adapter could not be instantiated 
                error = e_ERR_BAD_PARAM;
            }
        }
        *p_ta_error = error;
        return error;
    };
private:
    TargetAdapter *p_ta;
    uint8_t mem_page;
    int *p_ta_error;
};

template <typename DATA_WIDTH>
class mem_write {
public:
    mem_write(TargetAdapter *ta, uint8_t page, int *ta_error) :p_ta(ta), mem_page(page), p_ta_error(ta_error) {};
    int operator()(uint64_t address, DATA_WIDTH* value) {
        int error = e_ERR_NONE;

        // If the address fits in a 32 bit unsigned and the data size is not 64-bit
        // then use the 32-bit version of the target adapter API
        if (address <= std::numeric_limits<uint32_t>::max() && sizeof(DATA_WIDTH) != sizeof(uint64_t)) {
            // Issue 32-bit target adapter memory write
            unsigned int temp_value = (unsigned int)*value;
            error = p_ta->MemoryWrite(&temp_value, (uint32_t)address, sizeof(DATA_WIDTH), mem_page);
        }
        else { // Use 64-bit version of the target adapter API
               // Issue target adapter 64 bit memory write
            TargetAdapter64* p_ta_64 = reinterpret_cast< TargetAdapter64* >(p_ta->QueryInterface(eTargetAdapter64));
            if (p_ta_64) {
                uint64_t temp_value[2];
                // Target(DAP, Cortex) word size is 32-bit words. To be able to write a 64-bit value, the
                // data needs to be split into two 64-bit accesses where the first 64-bit unit has the
                // lower 32-bit and next 64-bit unit the upper 32-bits.
                temp_value[0] = ((uint64_t)*value & 0xFFFFFFFF);
                temp_value[1] = (((uint64_t)*value >> 32) & 0xFFFFFFFF);
                error = p_ta_64->MemoryWrite(temp_value, (uint64_t)address, sizeof(DATA_WIDTH), mem_page);
            }
            else { // 64-bit target adapter could not be instantiated
                error = e_ERR_BAD_PARAM;
            }
        }
        *p_ta_error = error;
        return error;
    };

private:
    TargetAdapter *p_ta;
    uint8_t mem_page;
    int *p_ta_error;
};


/**
* @brief Register read/write definition
*/
template <typename DATA_WIDTH>
class reg_read {
public:
    reg_read(TargetAdapter *ta, int *ta_error) :p_ta(ta), p_ta_error(ta_error){};
    int operator()(const char* reg_name, DATA_WIDTH* value) {
        int error = e_ERR_NONE;

        // If the address fits in a 32 bit unsigned and the data size is not 64-bit
        // then use the 32-bit version of the target adapter API
#ifdef WIN32    // Suppress Visual studio "Conditional expresssion is constant" compiler warning
__pragma(warning(push))
__pragma(warning(disable:4127))
#endif
        if (sizeof(DATA_WIDTH) != sizeof(uint64_t))    {
#ifdef WIN32
__pragma(warning(pop))
#endif
            // Get the register ID
            unsigned int reg_id;
            error = p_ta->GetRegisterId(reg_name, &reg_id);
            if (0 == error) {
                // Read the register
                unsigned int reg_value;
                error = p_ta->RegisterRead(reg_id, &reg_value);
                *value = (DATA_WIDTH)reg_value;
            }
        }
        else { // Use 64-bit version of the target adapter API
            TargetAdapter64* p_ta_64 = reinterpret_cast<TargetAdapter64*>(p_ta->QueryInterface(eTargetAdapter64));
            if (p_ta_64)  {
                TargetAdapter64::RegisterId reg_id;
                error = p_ta_64->GetRegisterId(reg_name, reg_id);
                if (0 == error) {
                    // Write register
                    uint64_t reg_value;
                    error = p_ta_64->RegisterRead(reg_id, reg_value);
                    *value = (DATA_WIDTH)reg_value;
                }
            }
            else { // 64-bit target adapter could not be instantiated
                error = e_ERR_BAD_PARAM;
            }
        }
        *p_ta_error = error;
        return error;
    };
private:
    TargetAdapter *p_ta;
    int *p_ta_error;
};

template <typename DATA_WIDTH>
class reg_write {
public:
    reg_write(TargetAdapter *ta, int *ta_error) :p_ta(ta), p_ta_error(ta_error){};
    int operator()(const char* reg_name, DATA_WIDTH* value) {
        int error = e_ERR_NONE;

        // If the address fits in a 32 bit unsigned and the data size is not 64-bit
        // then use the 32-bit version of the target adapter API
    
#ifdef WIN32 // Suppress Visual Studio "Conditional expresssion is constant" warning
__pragma(warning(push))
__pragma(warning(disable:4127))
#endif
        if (sizeof(DATA_WIDTH) != sizeof(uint64_t))  {
#ifdef WIN32
__pragma(warning(pop))
#endif
            // Get register ID
            unsigned int reg_id;
            error = p_ta->GetRegisterId(reg_name, &reg_id);
            if (0 == error) {
                // Write register
                unsigned int reg_value = (unsigned int)*value;
                error = p_ta->RegisterWrite(reg_id, reg_value);
            }
        }
        else { // Use 64-bit version of the target adapter API
            TargetAdapter64* p_ta_64 = reinterpret_cast<TargetAdapter64*>(p_ta->QueryInterface(eTargetAdapter64));
            if (p_ta_64)  {
                TargetAdapter64::RegisterId reg_id;
                error = p_ta_64->GetRegisterId(reg_name, reg_id);
                if (0 == error) {
                    // Write register
                    uint64_t reg_value = (uint64_t)*value;
                    error = p_ta_64->RegisterWrite(reg_id, reg_value);
                }
            }
            else { // 64-bit target adapter could not be instantiated
                error = e_ERR_BAD_PARAM;
            }
        }
        *p_ta_error = error;
        return error;
    };
private:
    TargetAdapter *p_ta;
    int *p_ta_error;
};


template <typename DATA_WIDTH>
class buffer_read {
public:
    buffer_read(TargetAdapter *ta, uint8_t page, int *ta_error) :p_ta(ta), mem_page(page), p_ta_error(ta_error) {};
    int operator()(uint64_t address, DATA_WIDTH* buffer, uint32_t count) {
        int error = e_ERR_NONE;

        // If the address fits in a 32 bit unsigned and the data size is not 64-bit
        // then use the 32-bit version of the target adapter API
        if (address <= std::numeric_limits<uint32_t>::max() && sizeof(DATA_WIDTH) != sizeof(uint64_t)) {
            error = p_ta->MemoryRead((unsigned int *)buffer, (uint32_t)(address), count * sizeof(DATA_WIDTH), mem_page);
        }
        else {    // Use 64-bit version of the target adapter API
            TargetAdapter64* p_ta_64 = reinterpret_cast< TargetAdapter64* >(p_ta->QueryInterface(eTargetAdapter64));
            if (p_ta_64) {
                error = p_ta_64->MemoryRead((uint64_t *)buffer, (uint64_t)(address), count * sizeof(DATA_WIDTH), mem_page);
            }
            else { // 64-bit target adapter could not be instantiated 
                error = e_ERR_BAD_PARAM;
            }
        }
        *p_ta_error = error;
        return error;
    };
private:
    TargetAdapter *p_ta;
    uint8_t mem_page;
    int *p_ta_error;
};

/**
* @brief Target Access Helper
*
* The target_access_helper is a collection of mem/reg access functor instances.
* Each IP driver will be handed the target access helper as part of the target
* access handle and will be consumers of the helper object instance. The helper
* object encapsulates the TargetAdapter and decouples the IP driver from any
* Target Adapter knowledge.
*/
class target_access_helper {
public:
    target_access_helper(TargetAdapter* ta, uint8_t page=0):
                                             mem_read_64(ta, page, &last_error),
                                             mem_read_32(ta, page, &last_error),
                                             mem_read_16(ta, page, &last_error),
                                             mem_read_8(ta, page, &last_error),
                                             mem_write_64(ta, page, &last_error),
                                             mem_write_32(ta, page, &last_error),
                                             mem_write_16(ta, page, &last_error),
                                             mem_write_8(ta, page, &last_error),
                                             reg_read_64(ta, &last_error), 
                                             reg_read_32(ta, &last_error),
                                             reg_read_16(ta, &last_error),
                                             reg_read_8(ta, &last_error),
                                             reg_write_64(ta, &last_error), 
                                             reg_write_32(ta, &last_error),
                                             reg_write_16(ta, &last_error),
                                             reg_write_8(ta, &last_error),
                                             buffer_read_64(ta, page, &last_error),
                                             buffer_read_32(ta, page, &last_error),
                                             buffer_read_16(ta, page, &last_error),
                                             buffer_read_8(ta, page, &last_error),
                                             page_num(page),
                                             last_error(0){};

    int get_last_error(){ return last_error;};
    uint8_t get_page(){ return page_num; };

    mem_read<uint64_t>      mem_read_64;
    mem_read<uint32_t>      mem_read_32;
    mem_read<uint16_t>      mem_read_16;
    mem_read<uint8_t>       mem_read_8;

    mem_write<const uint64_t>     mem_write_64;
    mem_write<const uint32_t>     mem_write_32;
    mem_write<const uint16_t>     mem_write_16;
    mem_write<const uint8_t>      mem_write_8;

    reg_read<uint64_t>      reg_read_64;
    reg_read<uint32_t>      reg_read_32;
    reg_read<uint16_t>      reg_read_16;
    reg_read<uint8_t>       reg_read_8;

    reg_write<const uint64_t>     reg_write_64;
    reg_write<const uint32_t>     reg_write_32;
    reg_write<const uint16_t>     reg_write_16;
    reg_write<const uint8_t>      reg_write_8;

    buffer_read<uint8_t> buffer_read_64;
    buffer_read<uint8_t> buffer_read_32;
    buffer_read<uint8_t> buffer_read_16;
    buffer_read<uint8_t> buffer_read_8;


private:
    uint8_t         page_num;
    int32_t         last_error;
    static const uint32_t MAGIC_NUMBER = 0x7AACCE55; //TAACCESS
};


/**
* @brief Register and memory read/write APIs
*
* It is expected that a target access helper object is handed to the IP driver as
* a void* as part of the setup properties and saved in ta_helper handle.
*
*/

/**
* Separate section for windows to suppress Visual Studio "Conditional expression is
* a constant" warning
*/
#ifdef _WIN32
/**
* @brief Memory read/write APIs
*/
#ifdef _REMOTE_HOST
#define READ_BUFFER_64(ta_handle, name, offset, buffer, count)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->buffer_read_64((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_64: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,%d)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count * 8) \
__pragma(warning(pop))

#define READ_BUFFER_32(ta_handle, name, offset, buffer, count)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->buffer_read_32((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_32: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,%d)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count * 4) \
__pragma(warning(pop))

#define READ_BUFFER_16(ta_handle, name, offset, buffer, count)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->buffer_read_16((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_16: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,%d)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count * 2) \
__pragma(warning(pop))

#define READ_BUFFER_8(ta_handle, name, offset, buffer, count)            \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_8((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_8: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%hhx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,%d)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count) \
__pragma(warning(pop))
#endif

#define READ_MEM_64(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_64((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_64: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%lx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset) \
__pragma(warning(pop))

#define READ_MEM_32(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_32((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_32: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%x  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset) \
__pragma(warning(pop))

#define READ_MEM_16(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_16((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_16: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,16)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset) \
__pragma(warning(pop))

#define READ_MEM_8(ta_handle, name, offset, value)            \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_8((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_8: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hhx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,8)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset) \
__pragma(warning(pop))

#define WRITE_MEM_64(ta_handle, name, offset, value)          \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_64((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_64: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%lx  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%lx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value) \
__pragma(warning(pop))

#define WRITE_MEM_32(ta_handle, name, offset, value)          \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_32((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_32: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%x  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%x,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value) \
__pragma(warning(pop))

#define WRITE_MEM_16(ta_handle, name, offset, value)          \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_16((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_16: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hx  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%hx,16)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value) \
__pragma(warning(pop))


#define WRITE_MEM_8(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_8((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_8: mem.taerror= %d  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hhx  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%hhx,8)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value) \
__pragma(warning(pop))

/**
* @brief Register read/write APIs
*/
#define READ_REG_64(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_64(reg, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_64: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%lx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg) \
__pragma(warning(pop))

#define READ_REG_32(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_32(reg, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_32: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%x  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg) \
__pragma(warning(pop))

#define READ_REG_16(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_16(reg, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_16: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%hx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg) \
__pragma(warning(pop))
#define READ_REG_8(ta_handle, name, reg, value)                \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_8(reg, value)?e_ERR_READ:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_8: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%hhx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg) \
__pragma(warning(pop))

#define WRITE_REG_64(ta_handle, name, reg, value)              \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_64(reg, value)?e_ERR_WRITE:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_64: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%lx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%lx\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value) \
__pragma(warning(pop))

#define WRITE_REG_32(ta_handle, name, reg, value)              \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_32(reg, value)?e_ERR_WRITE:e_ERR_NONE); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_32: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%x  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%x\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value) \
__pragma(warning(pop))

#define WRITE_REG_16(ta_handle, name, reg, value)              \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_16(reg, value)?e_ERR_WRITE:e_ERR_NONE) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_16: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%hx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%hx\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value) \
__pragma(warning(pop))

#define WRITE_REG_8(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_8(reg, value)?e_ERR_WRITE:e_ERR_NONE) \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_32: reg.taerror= %d  reg.name=%s  reg.ccsname=%s  reg.value=0x%hhx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%hhx\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value) \
__pragma(warning(pop))

#else
/**
* @brief Memory read/write APIs
*/

#ifdef _REMOTE_HOST

#define READ_BUFFER_64(ta_handle, name, offset, buffer, count)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->buffer_read_64((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_64: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,%d)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count)

#define READ_BUFFER_32(ta_handle, name, offset,  buffer, count)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->buffer_read_32((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_32: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count)

#define READ_BUFFER_16(ta_handle, name, offset,  buffer, count)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)-buffer_read_16((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_16: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,16)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count)

#define READ_BUFFER_8(ta_handle, name, offset,  buffer, count)            \
    (static_cast<target_access_helper *>((ta_handle)->helper)->buffer_read_8((ta_handle)->base_address + offset, buffer, count)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_BUFFER_8: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.count=0x%d  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,8)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      count, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, count)

#endif

#define READ_MEM_64(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_64((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_64: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%lx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset)

#define READ_MEM_32(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_32((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_32: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%x  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset)

#define READ_MEM_16(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_16((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_16: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,16)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset)

#define READ_MEM_8(ta_handle, name, offset, value)            \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_read_8((ta_handle)->base_address + offset, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_MEM_8: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hhx  " \
                                      "ccs.script=activeDS.memory.readData(%hhu,0x%llx,8)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset)

#define WRITE_MEM_64(ta_handle, name, offset, value)          \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_64((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_64: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%lx  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%lx,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value)

#define WRITE_MEM_32(ta_handle, name, offset, value)          \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_32((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_32: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%x  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%x,32)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value)

#define WRITE_MEM_16(ta_handle, name, offset, value)          \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_16((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_16: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hx  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%hx,16)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value)

#define WRITE_MEM_8(ta_handle, name, offset, value)           \
    (static_cast<target_access_helper *>((ta_handle)->helper)->mem_write_8((ta_handle)->base_address + offset, value)?e_ERR_WRITE:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_MEM_8: mem.taerror= %ld  mem.name=%s  mem.address=0x%llx  mem.page=%hhu  mem.value=0x%hhx  " \
                                      "ccs.script=activeDS.memory.writeData(%hhu,0x%llx,0x%hhx,8)\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, (ta_handle)->base_address + offset, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), \
                                      *value, static_cast<target_access_helper *>((ta_handle)->helper)->get_page(), (ta_handle)->base_address + offset, *value)


/**
* @brief Register read/write APIs
*/
#define READ_REG_64(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_64(reg, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_64: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%lx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg) \

#define READ_REG_32(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_32(reg, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_32: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%x  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg) \

#define READ_REG_16(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_16(reg, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_16: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%hx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg)

#define READ_REG_8(ta_handle, name, reg, value)                \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_read_8(reg, value)?e_ERR_READ:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "READ_REG_8: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%hhx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg)

#define WRITE_REG_64(ta_handle, name, reg, value)              \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_64(reg, value)?e_ERR_WRITE:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_32: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%lx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%lx\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value)

#define WRITE_REG_32(ta_handle, name, reg, value)              \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_32(reg, value)?e_ERR_WRITE:e_ERR_NONE); \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_32: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%x  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%x\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value)

#define WRITE_REG_16(ta_handle, name, reg, value)              \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_16(reg, value)?e_ERR_WRITE:e_ERR_NONE) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_16: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%hx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%hx\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value)

#define WRITE_REG_8(ta_handle, name, reg, value)               \
    (static_cast<target_access_helper *>((ta_handle)->helper)->reg_write_8(reg, value)?e_ERR_WRITE:e_ERR_NONE) \
    if (name) LOGMSG(LOG_MSG_TARGET, "WRITE_REG_32: reg.taerror= %ld  reg.name=%s  reg.ccsname=%s  reg.value=0x%hhx  " \
                                     "ccs.script=activeDS.expression.evaluate(\"%s=0x%hhx\")\n", static_cast<target_access_helper *>((ta_handle)->helper)->get_last_error(), \
                                      name, reg, *value, reg, *value)

#endif
#endif //__TARGET_ACCESS_REMOTE_H
