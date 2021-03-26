// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    Midway DCS Audio Board

****************************************************************************/

#ifndef MAME_AUDIO_DCS_H
#define MAME_AUDIO_DCS_H

#pragma once

#include "cpu/adsp2100/adsp2100.h"
#include "sound/dmadac.h"
#include "machine/bankdev.h"
#include "machine/timer.h"


class dcs_audio_device : public device_t
{
public:
	// for dcs2 (int dram_in_mb, offs_t polling_offset)
	void set_dram_in_mb(int dram_in_mb) { m_dram_in_mb = dram_in_mb; }
	void set_polling_offset(offs_t polling_offset) { m_polling_offset = polling_offset; }

	void set_auto_ack(int state);

	void set_fifo_callbacks(read16smo_delegate fifo_data_r, read16mo_delegate fifo_status_r, write_line_delegate fifo_reset_w);
	void set_io_callbacks(write_line_delegate output_full_cb, write_line_delegate input_empty_cb);

	uint16_t data_r();
	void ack_w();
	int data2_r();
	int control_r();

	void data_w(uint16_t data);
	void reset_w(int state);

	void fifo_notify(int count, int max);

	void dsio_idma_addr_w(uint32_t data);
	void dsio_idma_data_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t dsio_idma_data_r();
	void dmovlay_remap_memory();
	void dmovlay_callback(uint32_t data);
	void denver_postload(void);
	void install_speedup(void);

	// non public
	void dcs_boot();
	void dcs_reset();
	void dcs_register_state();
	uint16_t dcs_dataram_r(offs_t offset);
	void dcs_dataram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	void dcs_data_bank_select_w(uint16_t data);
	void dcs_data_bank_select2_w(uint16_t data);
	inline void sdrc_update_bank_pointers();
	void sdrc_remap_memory();
	void sdrc_reset();
	uint16_t sdrc_r(offs_t offset);
	void sdrc_w(offs_t offset, uint16_t data);
	void dsio_reset();
	uint16_t dsio_r(offs_t offset);
	void dsio_w(offs_t offset, uint16_t data);
	void denver_alloc_dmadac(void);
	void denver_reset();
	uint16_t denver_r(offs_t offset);
	void denver_w(offs_t offset, uint16_t data);
	uint16_t latch_status_r(address_space &space);
	uint16_t fifo_input_r();
	void dcs_delayed_data_w(uint16_t data);
	void input_latch_ack_w(uint16_t data);
	uint16_t input_latch_r();
	uint32_t input_latch32_r();
	void latch_delayed_w();
	void output_latch_w(uint16_t data);
	void output_latch32_w(uint32_t data);
	void delayed_ack_w();
	void output_control_delayed_w(uint16_t param);
	void output_control_w(uint16_t data);
	uint16_t output_control_r();
	void update_timer_count();
	TIMER_DEVICE_CALLBACK_MEMBER( internal_timer_callback );
	void reset_timer();
	DECLARE_WRITE_LINE_MEMBER(timer_enable_callback);
	uint16_t adsp_control_r(offs_t offset);
	void adsp_control_w(offs_t offset, uint16_t data);
	TIMER_DEVICE_CALLBACK_MEMBER( dcs_irq );
	TIMER_DEVICE_CALLBACK_MEMBER( sport0_irq );
	void recompute_sample_rate();
	void sound_tx_callback(offs_t offset, uint32_t data);
	uint16_t dcs_polling_r(address_space &space);
	void dcs_polling_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint32_t dcs_polling32_r(address_space &space);
	void dcs_polling32_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	TIMER_DEVICE_CALLBACK_MEMBER( transfer_watchdog_callback );
	void s1_ack_callback2();
	void s1_ack_callback1(uint16_t param);
	int preprocess_stage_1(uint16_t data);
	void s2_ack_callback(uint16_t param);
	int preprocess_stage_2(uint16_t data);
	int preprocess_write(uint16_t data);

	void dcs2_2104_data_map(address_map &map);
	void dcs2_2104_program_map(address_map &map);
	void dcs2_2115_data_map(address_map &map);
	void dcs2_2115_program_map(address_map &map);
	void dcs_2k_data_map(address_map &map);
	void dcs_2k_program_map(address_map &map);
	void dcs_2k_uart_data_map(address_map &map);
	void dcs_8k_data_map(address_map &map);
	void dcs_8k_program_map(address_map &map);
	void dcs_wpc_program_map(address_map &map);
	void denver_data_map(address_map &map);
	void denver_io_map(address_map &map);
	void denver_program_map(address_map &map);
	void denver_rambank_map(address_map &map);
	void dsio_data_map(address_map &map);
	void dsio_io_map(address_map &map);
	void dsio_program_map(address_map &map);
	void dsio_rambank_map(address_map &map);

	uint8_t get_rev() { return m_rev; } // TODO(RH): This can be done better, and shouldn't be necessary.
	cpu_device *get_cpu() { return m_cpu; } // TODO(RH): Same.

	enum { REV_DCS1, REV_DCS1P5, REV_DCS2, REV_DSIO, REV_DENV };

protected:
	// construction/destruction
	dcs_audio_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock, int rev);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	void add_mconfig_dcs(machine_config &config);

	static constexpr const char *const denver_regname[4] =
	{ "SDRC_ROM", "SDRC_IO", "RAM_PAGE", "VER/FIFO_RESET" };

	struct sdrc_state
	{
		uint16_t      reg[4];
		uint8_t       seed;
	};


	struct dsio_state
	{
		uint16_t      reg[4];
		uint8_t       start_on_next_write;
		uint16_t      channelbits;
	};


	struct hle_transfer_state
	{
		uint8_t       hle_enabled;
		int32_t       dcs_state;
		int32_t       state;
		int32_t       start;
		int32_t       stop;
		int32_t       type;
		int32_t       temp;
		int32_t       writes_left;
		uint16_t      sum;
		int32_t       fifo_entries;
		timer_device *watchdog;
	};

	adsp21xx_device *m_cpu;
	address_space *m_program;
	address_space *m_data;
	uint8_t       m_rev;
	offs_t      m_polling_offset;
	uint32_t      m_polling_count;
	/* sound output */
	uint8_t       m_channels;
	uint16_t      m_size;
	uint16_t      m_incs;
	dmadac_sound_device *m_dmadac[6];
	timer_device *m_reg_timer;
	timer_device *m_sport0_timer;
	timer_device *m_internal_timer;
	int32_t       m_ireg;
	uint16_t      m_ireg_base;
	uint16_t      m_control_regs[32];

	/* memory access/booting */
	uint16_t *    m_bootrom;
	uint32_t      m_bootrom_words;
	uint16_t *    m_sounddata;
	std::unique_ptr<uint16_t[]> m_sounddata_ptr;
	uint32_t      m_sounddata_words;
	uint32_t      m_sounddata_banks;
	uint16_t      m_sounddata_bank;

	optional_device<address_map_bank_device> m_ram_map;
	optional_memory_bank    m_data_bank;
	memory_bank_creator     m_rom_page;
	memory_bank_creator     m_dram_page;

	/* I/O with the host */
	uint8_t       m_auto_ack;
	uint16_t      m_latch_control;
	uint16_t      m_input_data;
	uint16_t      m_output_data;
	uint16_t      m_pre_output_data;
	uint16_t      m_output_control;
	uint64_t      m_output_control_cycles;
	uint8_t       m_last_output_full;
	uint8_t       m_last_input_empty;
	uint16_t      m_progflags;

	write_line_delegate m_output_full_cb;
	write_line_delegate m_input_empty_cb;

	read16smo_delegate m_fifo_data_r;
	read16mo_delegate m_fifo_status_r;
	write_line_delegate m_fifo_reset_w;

	/* timers */
	uint8_t       m_timer_enable;
	bool          m_timer_ignore;
	uint64_t      m_timer_start_cycles;
	uint32_t      m_timer_start_count;
	uint32_t      m_timer_scale;
	uint32_t      m_timer_period;
	uint32_t      m_timers_fired;

	transient_timer_factory m_dcs_reset;
	transient_timer_factory m_dcs_delayed_data_w;
	transient_timer_factory m_latch_delayed_w;
	transient_timer_factory m_delayed_ack_w;
	transient_timer_factory m_output_control_delayed_w;
	transient_timer_factory m_s1_ack_callback2;
	transient_timer_factory m_s1_ack_callback1;
	transient_timer_factory m_s2_ack_callback;

	std::unique_ptr<uint16_t[]> m_sram;
	uint16_t m_polling_value;
	uint32_t m_polling32_value;
	uint32_t *m_internal_program_ram;
	uint32_t *m_external_program_ram;
	uint32_t *m_internal_data_ram;

	int m_dmovlay_val;

	sdrc_state m_sdrc;
	dsio_state m_dsio;
	hle_transfer_state m_transfer;

	int m_dram_in_mb;

	optional_shared_ptr<uint16_t> m_iram;
	optional_device<device_execute_interface> m_maincpu;
};


// dcs_audio_2k_device

class dcs_audio_2k_device : public dcs_audio_device
{
public:
	// construction/destruction
	dcs_audio_2k_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;

};

// device type definition
DECLARE_DEVICE_TYPE(DCS_AUDIO_2K, dcs_audio_2k_device)

// dcs_audio_2k_uart_device

class dcs_audio_2k_uart_device : public dcs_audio_device
{
public:
	// construction/destruction
	dcs_audio_2k_uart_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS_AUDIO_2K_UART, dcs_audio_2k_uart_device)

// dcs_audio_8k_device

class dcs_audio_8k_device : public dcs_audio_device
{
public:
	// construction/destruction
	dcs_audio_8k_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS_AUDIO_8K, dcs_audio_8k_device)

// dcs_audio_wpc_device

class dcs_audio_wpc_device : public dcs_audio_device
{
public:
	// construction/destruction
	dcs_audio_wpc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	void dcs_wpc_data_map(address_map &map);
protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS_AUDIO_WPC, dcs_audio_wpc_device)


// dcs2_audio_device

class dcs2_audio_device : public dcs_audio_device
{
protected:
	// construction/destruction
	dcs2_audio_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	// device-level overrides
	virtual void device_start() override;
	void add_mconfig_dcs2(machine_config &config);
};

// dcs2_audio_2115_device

class dcs2_audio_2115_device : public dcs2_audio_device
{
public:
	// construction/destruction
	dcs2_audio_2115_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS2_AUDIO_2115, dcs2_audio_2115_device)

// dcs2_audio_2104_device

class dcs2_audio_2104_device : public dcs2_audio_device
{
public:
	// construction/destruction
	dcs2_audio_2104_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS2_AUDIO_2104, dcs2_audio_2104_device)

// dcs2_audio_dsio_device

class dcs2_audio_dsio_device : public dcs2_audio_device
{
public:
	// construction/destruction
	dcs2_audio_dsio_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS2_AUDIO_DSIO, dcs2_audio_dsio_device)

// dcs2_audio_denver_device types
class dcs2_audio_denver_device : public dcs2_audio_device
{
public:
	// construction/destruction
	dcs2_audio_denver_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);
protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

class dcs2_audio_denver_5ch_device : public dcs2_audio_denver_device
{
public:
	// construction/destruction
	dcs2_audio_denver_5ch_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

class dcs2_audio_denver_2ch_device : public dcs2_audio_denver_device
{
public:
	// construction/destruction
	dcs2_audio_denver_2ch_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
protected:
	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

// device type definition
DECLARE_DEVICE_TYPE(DCS2_AUDIO_DENVER_5CH, dcs2_audio_denver_5ch_device)

DECLARE_DEVICE_TYPE(DCS2_AUDIO_DENVER_2CH, dcs2_audio_denver_2ch_device)

#endif // MAME_AUDIO_DCS_H
