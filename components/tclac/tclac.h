/**
* Create by Miguel Ángel López on 20/07/19
* and modify by xaxexa
* Refactoring & component making:
* Nightingale with a soldering iron 15.03.2024
**/

#ifndef TCL_ESP_TCL_H
#define TCL_ESP_TCL_H

#include "esphome.h"
#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace tclac {

#define SET_TEMP_MASK	0b00001111

#define MODE_POS		7
#define MODE_MASK		0b00111111

#define MODE_AUTO		0b00110101
#define MODE_COOL		0b00110001
#define MODE_DRY		0b00110011
#define MODE_FAN_ONLY	0b00110010
#define MODE_HEAT		0b00110100

#define FAN_SPEED_POS	8
#define FAN_QUIET_POS	33

#define FAN_QUIET		0x80		//silent
#define FAN_POWER		0b10000000	//	POWER [7]

// Fan speed is encoded in bits 6-4 of dataRX[FAN_SPEED_POS] as a sequential level
// code, bit 7 being irrelevant to the speed itself: 0=auto, 1..5=speed level.
// Units with fewer than 5 physical speeds (auto+silent+low+medium+high+power)
// simply use a subset of codes 1-3 instead of 1-5.
#define FAN_SPEED_CODE(byte)	(((byte) >> 4) & 0x07)

// low/medium/high map directly onto the built-in ClimateFanMode enum (codes
// 1/2/3), so HA's translated "Low"/"Medium"/"High" labels are used as-is.
// Only the concepts TCL's protocol has that ESPHome's enum doesn't (silent,
// power, and the two in-between levels only present on 5-level units) need
// custom fan modes.
// NOTE: ESPHome's API matches an incoming fan mode string case-insensitively
// against the built-in ClimateFanMode names (ON, OFF, AUTO, LOW, MEDIUM,
// HIGH, MIDDLE, FOCUS, DIFFUSE, QUIET) BEFORE ever checking custom fan modes -
// regardless of what this device's traits actually support. None of the
// custom names below may equal one of those 10 words verbatim (this is why
// we don't just add "Low Medium"/"Medium High" as literal fragments of them).
static const char *const FAN_MODE_SILENT = "Silent";
static const char *const FAN_MODE_LOW_MEDIUM = "Low Medium";
static const char *const FAN_MODE_MEDIUM_HIGH = "Medium High";
static const char *const FAN_MODE_POWER = "Turbo";

// "Gentle Breeze" has no equivalent in ESPHome's built-in ClimatePreset enum
// (NONE/HOME/AWAY/BOOST/COMFORT/ECO/SLEEP/ACTIVITY), so - like the fan modes
// above - it is exposed as a custom preset instead.
// TX: dataTX[10] bit 0b01000000. RX: dataRX[50] bit 0b00100000.
// Both bits verified on hardware (UART capture, 2026-07-16) on a 5-level unit.
static const char *const PRESET_GENTLE_BREEZE = "Gentle Breeze";
#define GENTLE_BREEZE_TX_BIT	0b01000000
#define GENTLE_BREEZE_RX_POS	50
#define GENTLE_BREEZE_RX_BIT	0b00100000

#define SWING_POS			10
#define SWING_OFF			0b00000000
#define SWING_HORIZONTAL	0b00100000
#define SWING_VERTICAL		0b01000000
#define SWING_BOTH			0b01100000
#define SWING_MODE_MASK		0b01100000

using climate::ClimateCall;
using climate::ClimateMode;
using climate::ClimatePreset;
using climate::ClimateTraits;
using climate::ClimateFanMode;
using climate::ClimateSwingMode;

class tclacClimate : public climate::Climate, public esphome::uart::UARTDevice, public PollingComponent {

	private:
		uint8_t checksum;
		uint8_t check = 0;
		// dataTX with control consists of 38 bytes
		uint8_t dataTX[38];
		// And dataRX in some models swells to 68 bytes
		uint8_t dataRX[68];
		// State request command
		uint8_t poll[8] = {0xBB,0x00,0x01,0x04,0x02,0x01,0x00,0xBD};
		// Initialization and initial filling of switch state variables
		bool beeper_status_;
		bool display_status_;
		bool force_mode_status_;
		uint8_t switch_preset = 0;
		uint8_t switch_fan_mode = 0;
		uint8_t fan_speed_levels_ = 5;
		bool is_call_control = false;
		uint8_t switch_swing_mode = 0;
		int target_temperature_set = 0;
		uint8_t switch_climate_mode = 0;
		bool allow_take_control = false;
		
		esphome::climate::ClimateTraits traits_;
		
	public:

		tclacClimate() : PollingComponent(5 * 1000) {
			checksum = 0;
		}

		void readData();
		void takeControl();
		void loop() override;
		void setup() override;
		void update() override;
		void set_beeper_state(bool state);
		void set_display_state(bool disp_state);
		void set_force_mode_state(bool f_state);
		void sendData(uint8_t * message, uint8_t size);
		static String getHex(uint8_t *message, uint8_t size);
		static uint8_t getChecksum(const uint8_t * message, size_t size);
		void set_supported_presets(climate::ClimatePresetMask presets);
		void set_supported_custom_presets_option(std::vector<std::string> presets);
		void set_supported_modes(climate::ClimateModeMask modes);
		void set_supported_swing_modes(climate::ClimateSwingModeMask swing_modes);
		void set_fan_speed_levels(uint8_t levels);

	protected:
		ClimateTraits traits() override;
		climate::ClimateModeMask supported_modes_{};
		climate::ClimatePresetMask supported_presets_{};
		std::vector<std::string> supported_custom_presets_{PRESET_GENTLE_BREEZE};
		climate::ClimateSwingModeMask supported_swing_modes_{};
		void control(const climate::ClimateCall &call) override;
		uint8_t encode_fan_speed_tx_(uint8_t code) const;
};
}
}

#endif //TCL_ESP_TCL_H
