#ifndef NET_CANFrame_h__
#define NET_CANFrame_h__

#include <cstdint>

namespace NET
{
	// TODO This classes are just simple prototypes for now
	// TODO You must not forget to initialize the CAN structs!

	//! Represents any possible CAN id or mask
	struct can_id
	{
		uint32_t value : 29;
		uint32_t error_flag : 1;
		uint32_t rtr_flag : 1;
		uint32_t ext_format : 1;
	};

	//! Structure to represent a CAN 2.0 Frame.
	struct can_frame
	{
		can_id id;
		uint8_t dlc;
		uint8_t _pad_[3];
		uint8_t data[8];

		//! Cast-Operator for use with sockets
		operator void* () { return this; }

		//! Constant Cast-Operator for use with sockets
		operator const void* () const { return this; }

		//! Access to all data bytes
		uint8_t& operator[]( unsigned index) { return data[index]; }
	};

	//! Structure of id and mask used by filters
	struct can_filter
	{
		can_id id;
		can_id mask;
	};

} // namespace NET

#endif // NET_CANFrame_h__
