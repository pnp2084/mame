// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/**************************************************************************************************

	NEC PC8801-31 CD-ROM I/F

**************************************************************************************************/

#ifndef MAME_BUS_PC8801_31_H
#define MAME_BUS_PC8801_31_H

#pragma once

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class pc8801_31_device : public device_t
{
public:
	// construction/destruction
	pc8801_31_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto rom_bank_cb() { return m_rom_bank_cb.bind(); }

	// I/O operations
	void amap(address_map &map);

protected:
	// device-level overrides
	virtual void device_resolve_objects() override;
	virtual void device_add_mconfig(machine_config &config) override;
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	devcb_write_line m_rom_bank_cb;
	
	u8 clock_r();
	void volume_control_w(u8 data);
	u8 id_r();
	void rom_bank_w(u8 data);

	bool m_clock_hb;
};


// device type definition
DECLARE_DEVICE_TYPE(PC8801_31, pc8801_31_device)

#endif // MAME_BUS_PC8801_31_H
