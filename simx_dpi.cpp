#include "svdpi.h"
#include <iostream>
#include <vector>
#include <fstream> 

// Vortex Includes
#include "processor.h"
#include "arch.h"

// CHECK: If ram.h is missing, RAM might be defined in mem.h or sim/common
// We use a guard to check if we can include it, otherwise rely on mem.h
#if __has_include("ram.h")
#include "ram.h"
#else
#include "mem.h" 
// If RAM class is in a namespace, ensure we use it: using namespace vortex;
#endif

using namespace vortex;

// Global Handles
static Processor* g_processor = nullptr;
static RAM* g_ram       = nullptr;

extern "C" {

// 1. Unified Initialization
int simx_init(const char* kernel_file, int num_cores, int num_warps, int num_threads) {
    if (g_processor) delete g_processor;
    if (g_ram) delete g_ram;

    // config struct (adapt to your Vortex version's Arch definition)
    Arch arch;
    arch.num_cores = num_cores;
    arch.num_warps = num_warps;
    arch.num_threads = num_threads;

    // Create 4GB Memory
    // Note: In some versions, RAM constructor needs page size or block size
    g_ram = new RAM(0x100000000); 

    g_processor = new Processor(arch);
    g_processor->attach_ram(g_ram);

    // Load Kernel Binary
    std::ifstream ifs(kernel_file, std::ios::binary);
    if (!ifs) {
        std::cerr << "[SimX-DPI] Error: Cannot open " << kernel_file << "\n";
        return 1;
    }
    
    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> mem_data(size);
    if (ifs.read((char*)mem_data.data(), size)) {
         // Write program to startup address (0x80000000)
         g_ram->write(mem_data.data(), 0x80000000, size);
    }

    return 0;
}

// 2. Flexible Stepping
// cycles > 0  : Run for N cycles (On-the-Fly mode)
// cycles == 0 : Run until completion (Post-Mortem mode)
// Returns: 0 = Finished, 1 = Still Running
int simx_step(int cycles = 0) {
    if (!g_processor) return 0;

    int exitcode = 0;

    if (cycles == 0) {
        // --- POST-MORTEM MODE ---
        // Run until the processor indicates it is done
        exitcode = g_processor->run();
        return 0; // Finished
    } else {
        // --- ON-THE-FLY MODE ---
        // Step N times
        for (int i = 0; i < cycles; ++i) {
             // You need to access the internal 'tick' function.
             // If Processor::tick() is not public, you might need to modify Processor.h
             // Or usually, Processor::step() exists.
             bool active = g_processor->tick(); 
             if (!active) return 0; // Finished early
        }
        return 1; // Still running
    }
}

// Add this to your simx_dpi.cpp inside the extern "C" block
void simx_write_mem(longint addr, int size, const svOpenArrayHandle data_handle) {
    uint8_t* sv_ptr = (uint8_t*)svGetArrayPtr(data_handle);
    if (g_ram && sv_ptr) {
        // Direct write into SimX RAM
        g_ram->write(sv_ptr, addr, size);
        std::printf("[DPI] Written %d bytes to SimX at 0x%llx\n", size, addr);
    }
}

// 3. Read Memory (Universal)
void simx_read_memory(long long addr, int size, const svOpenArrayHandle data_handle) {
    uint8_t* sv_ptr = (uint8_t*)svGetArrayPtr(data_handle);
    if (g_ram && sv_ptr) {
        g_ram->read(sv_ptr, addr, size);
    }
}

// 4. Cleanup
void simx_cleanup() {
    if (g_processor) { delete g_processor; g_processor = nullptr; }
    if (g_ram)       { delete g_ram;       g_ram = nullptr; }
}

}

