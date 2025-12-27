module test_top;
    import vortex_config_pkg::*;

    // DPI Imports
    import "DPI-C" context function int simx_init(string kernel, int nc, int nw, int nt);
    import "DPI-C" context function void simx_write_mem(longint addr, int size, input byte data[]);
    import "DPI-C" context function void simx_read_memory(longint addr, int size, inout byte data[]);
    import "DPI-C" context function int simx_step(int cycles);
    import "DPI-C" context function void simx_cleanup();

    byte fake_program[16] = '{8'h13, 8'h0, 8'h0, 8'h0, 8'h13, 8'h0, 8'h0, 8'h0, 
                              8'h13, 8'h0, 8'h0, 8'h0, 8'h13, 8'h0, 8'h0, 8'h0};
    byte read_back[16];

    initial begin
        $display("--- Starting SimX DPI Connection Test ---");

        // 1. Initialize (passing empty string since we write memory manually)
        if (simx_init("", 1, 1, 4) == 0) begin
            $display("[SV] SimX Object created successfully.");
        end

        // 2. Write the NOPs to the startup address
        // 0x80000000 is the typical RISC-V start address in Vortex
        simx_write_mem(64'h80000000, 16, fake_program);

        // 3. Step the model (Run for a few cycles)
        void'(simx_step(10));

        // 4. Read back to verify
        simx_read_memory(64'h80000000, 16, read_back);
        
        foreach(read_back[i]) begin
            $display("[SV] Addr 0x%x : Data 0x%x", 64'h80000000 + i, read_back[i]);
        end

        simx_cleanup();
        $display("--- Test Finished ---");
        $finish;
    end
endmodule
