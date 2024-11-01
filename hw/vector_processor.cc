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

#include <systemc.h>

vector_processor::vector_processor(sc_module_name name)
	: sc_module(name), socket("socket")
{
	socket.register_b_transport(this, &vector_processor::b_transport);
	socket.register_transport_dbg(this, &vector_processor::transport_dbg);
	SC_THREAD(op_thread);	// New SC Thread from issue 4
}

// SCThread op
void vector_processor::op_thread()
{
	const sc_time delay = sc_time(5, SC_MS);

	for(;;) {
		wait(start);	// Wait on event start
		wait(delay);	// Wait for 5ms

		CSR = 0x0;		// opperation concluded, set to 0x0
	}
}

// Processing thread function to simulate operation delay
void vector_processor::op_thread()
{
	const sc_time delay = sc_time(5, SC_MS);

	for(;;) {
		wait(start);	// Wait on event start
		wait(delay);	// Simulate 5 ms processing delay

		CSR = 0x0;		// Operation concluded, set to 0x0
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

	// transactions with separate byte lanes are not supported
	if (byt != 0) {
		trans.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
		return;
	}

	// bursts not supported
	if (len > 4 || wid < len) {
		trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
		return;
	}
	// besides that, let everything pass 
	// note: even an access to a non existing MMR passes
	trans.set_response_status(tlm::TLM_OK_RESPONSE);

	// Annotate that this target needs 1us to think 
	// about how to answer an MMR request (not processing)
	// This delay is on top of transport delay (which the iconnect should model).
	delay += sc_time(1, SC_US);

	// force to catch up any quantum delay offset (to make it easier for now)
	wait(delay);
	delay = sc_time(0, SC_US);

	// compute current time (incl. any quantum offset if no sync above)
	sc_time now = sc_time_stamp() + delay;

	// handle reads commands
	if (cmd == tlm::TLM_READ_COMMAND) {
		static sc_time old_ts = SC_ZERO_TIME, diff;
		uint32_t v = 0;
		switch (addr)
		{
		case MMR_TRACE:
			diff = now - old_ts; // diff to last TRACE read call
			v = now.to_seconds() * 1000 * 1000 * 1000; // ns 
			cout << "TRACE: "
				 << " " << now << " diff=" << diff << "\n";
			old_ts = now;
			break;
		case 0x0:  // CSR read operation
            v = CSR;  // Read the CSR value
			cout << "CSR: " << v << "\n";
			break;
		case 0x04:	// VA base address is in data	-- might have to deal with length here
			for (int i=0;i<16;i++) {
				VA[i]= data + (4*i);	// Read from VA
			}
			break;
		case 0x44:	// VB base addresss is in data
			for (int i=0;i<16;i++) {
				VB[i]= data + (4*i);	// Read from VB
			}
			break;
		case 0x84:	// VC base address is in data
			for (int i=0;i<16;i++) {
				VC[i]= data + (4*i);	// Read from VC
			}
			break;
		default:
			break;
		}


		memcpy(data, &v, len);

	// handle write commands
	} else if (cmd == tlm::TLM_WRITE_COMMAND) {
		static sc_time old_ts = SC_ZERO_TIME, diff;
		switch (addr) {
		case MMR_TRACE:
			diff = now - old_ts; // diff to last TRACE write call
			cout << "TRACE: "
				 << " "
				 << hex << *(uint32_t *)data
				 << ", " << now << " diff=" << diff << "\n";
			old_ts = now;
		case 0x0:  // CSR write operation
            CSR = *(uint32_t*)data;  // Write data to CSR
			// Check if LSB is set
			if (CSR & 0x1) {
				// Set the start event
				start.notify();
				// start.notify(SC_ZERO_TIME);	// need the SC_ZERO_TIME?
			}
			break;
		case 0x04:	// VA base address is in data	-- might have to deal with length here
			for (int i=0;i<16;i++) {
				*(uint32_t*)(data + (4*i)) = VA[i];	// Write to VA
			}
			break;
		case 0x44:	// VB base addresss is in data
			for (int i=0;i<16;i++) {
				*(uint32_t*)(data + (4*i)) = VB[i];	// Write to VA
			}
			break;
		case 0x84:	// VC base address is in data
			for (int i=0;i<16;i++) {
				*(uint32_t*)(data + (4*i)) = VC[i];	// Write to VA
			}
			break; 

		default:
			break;
		}


	} else {
		// no other commands supported
		trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
	}
}

unsigned int vector_processor::transport_dbg(tlm::tlm_generic_payload &trans)
{
	unsigned int len = trans.get_data_length();
	return len;
}
