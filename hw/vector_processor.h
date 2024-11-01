/** @brief Vector Processor (derived from Xilinx DebugDev)
 * 
 */


/* address space definition (offsets within target) */
#define MMR_TRACE 0xC4

class vector_processor
	: public sc_core::sc_module
{
public:
	tlm_utils::simple_target_socket<vector_processor> socket;
	sc_out<bool> irq;
	uint32_t CSR = 0;  // Control Status Register initialized to 0

	sc_core::sc_event start;	// Start event from issue 4

	SC_HAS_PROCESS(vector_processor);

	// Vectors initialization - may need to make this one array later on
	uint32_t VA[16];
	uint32_t VB[16];
	uint32_t VC[16];

	vector_processor(sc_core::sc_module_name name);
	void op_thread();
	virtual void b_transport(tlm::tlm_generic_payload &trans,
							 sc_time &delay);
	virtual unsigned int transport_dbg(tlm::tlm_generic_payload &trans);
};
