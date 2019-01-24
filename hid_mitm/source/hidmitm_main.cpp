/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#include "hid_mitm_service.hpp"
#include "hid_mitm_iappletresource.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x300000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}


void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {

    Result rc;
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = timeInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = hidInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = socketInitializeDefault();
    if (R_FAILED(rc))
        fatalSimple(rc);
    
}

void __appExit(void) {
    /* Cleanup services. */
    hidExit();
    fsdevUnmountAll();
    fsExit();
    smExit();
    audoutExit();
    timeExit();
}

struct HidManagerOptions {
    static const size_t PointerBufferSize = 0x100;
    static const size_t MaxDomains = 4;
    static const size_t MaxDomainObjects = 0x100;
};

using HidMitmManager = WaitableManager<HidManagerOptions>;

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);


    /* TODO: What's a good timeout value to use here? */
    auto server_manager = new HidMitmManager(1);
    
    /* Create hid mitm. */
    AddMitmServerToManager<HidMitmService>(server_manager, "hid", 4);
    
    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    delete server_manager;
    return 0;
}

