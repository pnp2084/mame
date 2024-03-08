// license:BSD-3-Clause
// copyright-holders: Takahiro Nogi, David Haywood
/******************************************************************************

    Gomoku Narabe Renju
    (c)1981 Nihon Bussan Co.,Ltd.

    Driver by Takahiro Nogi 1999/11/06 -
    Updated to compile again by David Haywood 19th Oct 2002

TODO:
- Refactor sound emulation.
- BG(Go table) is generated by board circuitry, so not fully emulated.
- Couldn't figure out the method to specify palette, so I modified palette number manually.
- Couldn't figure out oneshot sound playback parameter, so I adjusted it manually.

******************************************************************************/

#include "emu.h"

#include "gomoku_a.h"

#include "cpu/z80/z80.h"
#include "machine/74259.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class gomoku_state : public driver_device
{
public:
	gomoku_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_videoram(*this, "videoram"),
		m_colorram(*this, "colorram"),
		m_bgram(*this, "bgram"),
		m_inputs(*this, { "IN0", "IN1", "DSW" }),
		m_maincpu(*this, "maincpu"),
		m_gfxdecode(*this, "gfxdecode"),
		m_screen(*this, "screen"),
		m_bg_x(*this, "bg_x"),
		m_bg_y(*this, "bg_y"),
		m_bg_d(*this, "bg_d")
	{ }

	void gomoku(machine_config &config);

protected:
	virtual void video_start() override;

private:
	required_shared_ptr<uint8_t> m_videoram;
	required_shared_ptr<uint8_t> m_colorram;
	required_shared_ptr<uint8_t> m_bgram;
	required_ioport_array<3> m_inputs;
	required_device<cpu_device> m_maincpu;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<screen_device> m_screen;
	required_region_ptr<uint8_t> m_bg_x;
	required_region_ptr<uint8_t> m_bg_y;
	required_region_ptr<uint8_t> m_bg_d;

	bool m_flipscreen = false;
	bool m_bg_dispsw = false;
	tilemap_t *m_fg_tilemap = nullptr;
	bitmap_ind16 m_bg_bitmap{};

	uint8_t input_port_r(offs_t offset);
	void videoram_w(offs_t offset, uint8_t data);
	void colorram_w(offs_t offset, uint8_t data);
	void flipscreen_w(int state);
	void bg_dispsw_w(int state);
	TILE_GET_INFO_MEMBER(get_fg_tile_info);
	void palette(palette_device &palette) const;
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void prg_map(address_map &map);
};


/******************************************************************************

    palette RAM

******************************************************************************/

void gomoku_state::palette(palette_device &palette) const
{
	const uint8_t *color_prom = memregion("proms")->base();

	for (int i = 0; i < palette.entries(); i++)
	{
		int bit0, bit1, bit2;

		// red component
		bit0 = BIT(*color_prom, 0);
		bit1 = BIT(*color_prom, 1);
		bit2 = BIT(*color_prom, 2);
		int const r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		// green component
		bit0 = BIT(*color_prom, 3);
		bit1 = BIT(*color_prom, 4);
		bit2 = BIT(*color_prom, 5);
		int const g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		// blue component
		bit0 = 0;
		bit1 = BIT(*color_prom, 6);
		bit2 = BIT(*color_prom, 7);
		int const b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette.set_pen_color(i, rgb_t(r, g, b));
		color_prom++;
	}
}


/******************************************************************************

    Tilemap callbacks

******************************************************************************/

TILE_GET_INFO_MEMBER(gomoku_state::get_fg_tile_info)
{
	int const code = (m_videoram[tile_index]);
	int const attr = (m_colorram[tile_index]);
	int const color = (attr & 0x0f);
	int const flipyx = (attr & 0xc0) >> 6;

	tileinfo.set(0, code, color, TILE_FLIPYX(flipyx));
}

void gomoku_state::videoram_w(offs_t offset, uint8_t data)
{
	m_videoram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset);
}

void gomoku_state::colorram_w(offs_t offset, uint8_t data)
{
	m_colorram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset);
}

void gomoku_state::flipscreen_w(int state)
{
	m_flipscreen = state ? 0 : 1;
}

void gomoku_state::bg_dispsw_w(int state)
{
	m_bg_dispsw = state ? 0 : 1;
}


/******************************************************************************

    Start the video hardware emulation

******************************************************************************/

void gomoku_state::video_start()
{
	m_screen->register_screen_bitmap(m_bg_bitmap);

	m_fg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(gomoku_state::get_fg_tile_info)), TILEMAP_SCAN_ROWS, 8, 8, 32, 32);

	m_fg_tilemap->set_transparent_pen(0);

	// make background bitmap
	m_bg_bitmap.fill(0x20);

	// board
	for (int y = 0; y < 256; y++)
	{
		for (int x = 0; x < 256; x++)
		{
			int const bgdata = m_bg_d[m_bg_x[x] + (m_bg_y[y] << 4)];

			int color = 0x20;                   // outside frame (black)

			if (bgdata & 0x01) color = 0x21;    // board (brown)
			if (bgdata & 0x02) color = 0x20;    // frame line (black)

			m_bg_bitmap.pix((255 - y - 1) & 0xff, (255 - x + 7) & 0xff) = color;
		}
	}

	save_item(NAME(m_flipscreen)); // set but never used?
	save_item(NAME(m_bg_dispsw));
}


/******************************************************************************

    Display refresh

******************************************************************************/

uint32_t gomoku_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// draw background layer
	if (m_bg_dispsw)
	{
		int color;

		// copy bg bitmap
		copybitmap(bitmap, m_bg_bitmap, 0, 0, 0, 0, cliprect);

		// stone
		for (int y = 0; y < 256; y++)
		{
			for (int x = 0; x < 256; x++)
			{
				int const bgoffs = ((((255 - x - 2) / 14) | (((255 - y - 10) / 14) << 4)) & 0xff);

				int const bgdata = m_bg_d[m_bg_x[x] + (m_bg_y[y] << 4) ];
				int const bgram = m_bgram[bgoffs];

				if (bgdata & 0x04)
				{
					if (bgram & 0x01)
					{
						color = 0x2f;   // stone (black)
					}
					else if (bgram & 0x02)
					{
						color = 0x22;   // stone (white)
					}
					else continue;
				}
				else continue;

				bitmap.pix((255 - y - 1) & 0xff, (255 - x + 7) & 0xff) = color;
			}
		}

		// cursor
		for (int y = 0; y < 256; y++)
		{
			for (int x = 0; x < 256; x++)
			{
				int const bgoffs = ((((255 - x - 2) / 14) | (((255 - y - 10) / 14) << 4)) & 0xff);

				int const bgdata = m_bg_d[m_bg_x[x] + (m_bg_y[y] << 4) ];
				int const bgram = m_bgram[bgoffs];

				if (bgdata & 0x08)
				{
					if (bgram & 0x04)
					{
						color = 0x2f;   // cursor (black)
					}
					else if (bgram & 0x08)
					{
						color = 0x22;   // cursor (white)
					}
					else continue;
				}
				else continue;

				bitmap.pix((255 - y - 1) & 0xff, (255 - x + 7) & 0xff) = color;
			}
		}
	}
	else
	{
		bitmap.fill(0x20);
	}

	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	return 0;
}


uint8_t gomoku_state::input_port_r(offs_t offset)
{
	int res = 0;

	// input ports are rotated 90 degrees
	for (int i = 0; i < 3; i++)
		res |= BIT(m_inputs[i]->read(), offset) << i;

	return res | 0xf8;
}


void gomoku_state::prg_map(address_map &map)
{
	map(0x0000, 0x47ff).rom();
	map(0x4800, 0x4fff).ram();
	map(0x5000, 0x53ff).ram().w(FUNC(gomoku_state::videoram_w)).share(m_videoram);
	map(0x5400, 0x57ff).ram().w(FUNC(gomoku_state::colorram_w)).share(m_colorram);
	map(0x5800, 0x58ff).ram().share(m_bgram);
	map(0x6000, 0x601f).w("gomoku", FUNC(gomoku_sound_device::sound1_w));
	map(0x6800, 0x681f).w("gomoku", FUNC(gomoku_sound_device::sound2_w));
	map(0x7000, 0x7007).w("latch", FUNC(ls259_device::write_d1));
	map(0x7800, 0x7807).r(FUNC(gomoku_state::input_port_r));
	map(0x7800, 0x7800).nopw();
}


static INPUT_PORTS_START( gomoku )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME (0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 ) // service coin
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START("DSW")
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )  PORT_DIPLOCATION("SW1:1")
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Lives ))  PORT_DIPLOCATION("SW1:2,3")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x06, "5" )
	PORT_DIPNAME( 0x08, 0x00, "Time" )  PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x08, "80" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )  PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )  PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8, 8,       // 8*8 characters
	256,        // 256 characters
	2,          // 2 bits per pixel
	{ 0, 4 },   // the two bitplanes are packed in one byte
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8        // every char takes 16 consecutive bytes
};

static GFXDECODE_START( gfx_gomoku )
	GFXDECODE_ENTRY( "chars", 0, charlayout, 0, 32 )
GFXDECODE_END


void gomoku_state::gomoku(machine_config &config)
{
	// basic machine hardware
	Z80(config, m_maincpu, XTAL(18'432'000) / 12);      // 1.536 MHz ?
	m_maincpu->set_addrmap(AS_PROGRAM, &gomoku_state::prg_map);
	m_maincpu->set_vblank_int("screen", FUNC(gomoku_state::irq0_line_hold));

	ls259_device &latch(LS259(config, "latch")); // 7J
	latch.q_out_cb<1>().set(FUNC(gomoku_state::flipscreen_w));
	latch.q_out_cb<2>().set(FUNC(gomoku_state::bg_dispsw_w));
	latch.q_out_cb<7>().set_nop(); // start LED?

	// video hardware
	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_vblank_time(ATTOSECONDS_IN_USEC(0));
	m_screen->set_size(256, 256);
	m_screen->set_visarea(0, 256-1, 16, 256-16-1);
	m_screen->set_screen_update(FUNC(gomoku_state::screen_update));
	m_screen->set_palette("palette");

	GFXDECODE(config, m_gfxdecode, "palette", gfx_gomoku);
	PALETTE(config, "palette", FUNC(gomoku_state::palette), 64);

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GOMOKU_SOUND(config, "gomoku").add_route(ALL_OUTPUTS, "mono", 1.0);
}


ROM_START( gomoku )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rj_1.7a",    0x0000, 0x1000, CRC(ed20d539) SHA1(7cbbc678cbe5c85b914ca44f82bdbd452cf694a0) )
	ROM_LOAD( "rj_2.7c",    0x1000, 0x1000, CRC(26a28516) SHA1(53d5d134cd91020fa06e380d355deb1df6b9cb6e) )
	ROM_LOAD( "rj_3.7d",    0x2000, 0x1000, CRC(d05db072) SHA1(9697c932c6dcee6f8536c9f0b3c84a719a7d3dee) )
	ROM_LOAD( "rj_4.7f",    0x3000, 0x1000, CRC(6e3d1c18) SHA1(e2f7e4c0de3c78d1b8e686152458972f996b023a) )
	ROM_LOAD( "rj_5.4e",    0x4000, 0x0800, CRC(eaf541b4) SHA1(bc7e7ec1ba68f71ab9ac86f9ae77971ddb9ce3a4) )

	ROM_REGION( 0x1000, "chars", 0 )
	ROM_LOAD( "rj_6.4r",    0x0000, 0x1000, CRC(ed26ae36) SHA1(61cb73d7f2568e88e1c2981e7af3e9a3b26797d3) )

	ROM_REGION( 0x1000, "gomoku", 0 )   // sound
	ROM_LOAD( "rj_7.3c",    0x0000, 0x1000, CRC(d1ed1365) SHA1(4ef08f26fe7df4c400f72e09e56d8825d584f55f) )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "rj_prom.1m", 0x0000, 0x0020, CRC(5da2f2bd) SHA1(4355ccf06cb09ec3240dc92bda19b1f707a010ef) )  // TEXT color
	ROM_LOAD( "rj_prom.1l", 0x0020, 0x0020, CRC(fe4ef393) SHA1(d4c63f8645afeadd13ff82087bcc497d8936d90b) )  // BG color

	ROM_REGION( 0x0100, "bg_x", 0 )    // BG draw data X
	ROM_LOAD( "rj_prom.8n", 0x0000, 0x0100, CRC(9ba43222) SHA1(a443df49d7ee9dbfd258b09731d392bf1249cbfa) )

	ROM_REGION( 0x0100, "bg_y", 0 )    // BG draw data Y
	ROM_LOAD( "rj_prom.7p", 0x0000, 0x0100, CRC(5b5464f8) SHA1(b945efb8a7233f501d67f6b1be4e9d4967dc6719) )

	ROM_REGION( 0x0100, "bg_d", 0 )    // BG character data
	ROM_LOAD( "rj_prom.7r", 0x0000, 0x0100, CRC(3004585a) SHA1(711b68140827f0f3dc71f2576fcf9b905c999e8d) )

	ROM_REGION( 0x0020, "unkprom", 0 )    // unknown
	ROM_LOAD( "rj_prom.9k", 0x0000, 0x0020, CRC(cff72923) SHA1(4f61375028ab62da46ed119bc81052f5f98c28d4) )
ROM_END

} // anonymous namespace


//    YEAR,   NAME,   PARENT,  MACHINE,  INPUT,  STATE         INIT,       MONITOR, COMPANY,      FULLNAME,              FLAGS
GAME( 1981,   gomoku, 0,       gomoku,   gomoku, gomoku_state, empty_init, ROT90,   "Nichibutsu", "Gomoku Narabe Renju", MACHINE_SUPPORTS_SAVE )
