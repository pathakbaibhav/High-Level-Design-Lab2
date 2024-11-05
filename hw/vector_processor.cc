/*
 * Vector Processor 
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <inttypes.h>

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

using namespace sc_core;
using namespace std;

#include "vector_processor.h"
#include <sys/types.h>
#include <time.h>

#include <systemc>

vector_processor::vector_processor(sc_module_name name)
	: sc_module(name), socket("socket"), CSR(0)
{
	socket.register_b_transport(this, &vector_processor::b_transport);
	socket.register_transport_dbg(this, &vector_processor::transport_dbg);
	SC_THREAD(op_thread);	// New SC Thread from issue 4

	VA = (uint32_t*)malloc(16*sizeof(uint32_t));
	VB = (uint32_t*)malloc(16*sizeof(uint32_t));
	VC = (uint32_t*)malloc(16*sizeof(uint32_t));

}

// Processing thread function to simulate operation delay
void vector_processor::op_thread()
{
	const sc_time delay = sc_time(5, SC_MS);

	for(;;) {
        wait(start);    // Wait for the start event to trigger processing

        switch (CSR){
            case 0x1:   // Add
                for (int i = 0; i < 16; i++) {
                    VC[i] = VA[i] + VB[i];
                }
                break;
            case 0x2:   // Subtract
                for (int i=0; i<16;i++) {
                    VC[i] = VA[i] - VB[i];
                }
                break;
            case 0x3:   // Multiply
                for (int i=0; i<16;i++) {
                    VC[i] = VA[i] * VB[i];
                }
                break;
            case 0x4:   // Divide
                for (int i=0; i<16;i++) {
                    VC[i] = VA[i] / VB[i];
                }
                break;
            case 0x5:   // Multiply AND Accumulate
                for (int i=0; i<16; i++) {
                    VC[i] = VA[i] * VB[i] + VC[i];
                }
                break;
            default:
                break;
        }

        wait(sc_time(5, SC_MS));  // Simulate processing delay
        
        CSR = 0x0;  // Operation concluded, reset CSR to 0
    }
}



// called when a TLM transaction arrives for this target
void vector_processor::b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
{
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64 addr = trans.get_address();
    unsigned char *data = trans.get_data_ptr();
    unsigned int len = trans.get_data_length();
    unsigned char *byt = trans.get_byte_enable_ptr();
    unsigned int wid = trans.get_streaming_width();

    // Transactions with separate byte lanes are not supported
    if (byt != 0) {
        trans.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
        return;
    }

    // Bursts not supported
    if (len > 4 || wid < len) {
        trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }

    // Besides that, let everything pass
    // Note: even an access to a non-existing MMR passes
    trans.set_response_status(tlm::TLM_OK_RESPONSE);

    // Annotate that this target needs 1us to think 
    // about how to answer an MMR request (not processing)
    // This delay is on top of transport delay (which the iconnect should model).
    delay += sc_time(1, SC_US);

    // Force to catch up any quantum delay offset (to make it easier for now)
    wait(delay);
    delay = sc_time(0, SC_US);

    // Compute current time (including any quantum offset if no sync above)
    sc_time now = sc_time_stamp() + delay;

    // Variables for indexing into vectors
    uint32_t index = 0;  
    uint32_t v = 0;      

    // Handle read commands
    if (cmd == tlm::TLM_READ_COMMAND) {
        static sc_time old_ts = SC_ZERO_TIME, diff;
        
        switch (addr) {
            case MMR_TRACE:
                diff = now - old_ts;  // Time difference for TRACE reads
                v = now.to_seconds() * 1000000000;  // Convert to ns
                cout << "TRACE: " << now << " diff=" << diff << "\n";
                old_ts = now;
                break;

            case 0x0:  // CSR read operation
                v = CSR;
                cout << "CSR: " << v << "\n";
                break;

            // Handle reads for vector arrays VA, VB, VC
            default:
                if (addr >= 0x04 && addr < 0x44) {  // VA range
                    index = (addr - 0x04) / 4;
                    v = VA[index];
                } else if (addr >= 0x44 && addr < 0x84) {  // VB range
                    index = (addr - 0x44) / 4;
                    v = VB[index];
                } else if (addr >= 0x84 && addr < 0xC4) {  // VC range
                    index = (addr - 0x84) / 4;
                    v = VC[index];
                } else {
                    trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
                    return;
                }
                break;
        }
        memcpy(data, &v, len);  // Copy read value to data

    // Handle write commands
    } else if (cmd == tlm::TLM_WRITE_COMMAND) {
        static sc_time old_ts = SC_ZERO_TIME, diff;
        
        switch (addr) {
            case MMR_TRACE:
                diff = now - old_ts;  // Time difference for TRACE writes
                cout << "TRACE: " << hex << *(uint32_t *)data << ", " << now << " diff=" << diff << "\n";
                old_ts = now;
                break;

            case 0x0:  // CSR write operation
                CSR = *(uint32_t *)data;  // Update CSR
                if (CSR >= 0x1 && CSR <= 0x5) {
                    start.notify();  // Trigger start event if LSB is set
                    cout << "Starting event\n";
                }
                break;

            // Handle writes to vector arrays VA, VB, VC
            default:
                if (addr >= 0x04 && addr < 0x44) {  // VA range
                    index = (addr - 0x04) / 4;
                    VA[index] = *(uint32_t *)data;
                } else if (addr >= 0x44 && addr < 0x84) {  // VB range
                    index = (addr - 0x44) / 4;
                    VB[index] = *(uint32_t *)data;
                } else if (addr >= 0x84 && addr < 0xC4) {  // VC range
                    index = (addr - 0x84) / 4;
                    VC[index] = *(uint32_t *)data;
                } else {
                    trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
                    return;
                }
                break;
        }
    } else {
        // Unsupported commands
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}


unsigned int vector_processor::transport_dbg(tlm::tlm_generic_payload &trans)
{
	unsigned int len = trans.get_data_length();
	return len;
}