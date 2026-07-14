/**
* Create by Miguel Ángel López on 20/07/19
* and modify by xaxexa
* Refactoring & component making:
* Nightingale with a soldering iron 15.03.2024
**/
#include "esphome.h"
#include "esphome/core/defines.h"
#include "tclac.h"

namespace esphome{
namespace tclac{


ClimateTraits tclacClimate::traits() {
	auto traits = climate::ClimateTraits();
	traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
	
	// I responsibly declare that I took all this from christoph5180
	if (this->supported_modes_.empty()) {
		traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
		traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
	} else {
		for (auto mode : this->supported_modes_)
			traits.add_supported_mode(mode);
	}
	if (this->supported_presets_.empty()) {
		traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);
	} else {
		for (auto preset : this->supported_presets_)
			traits.add_supported_preset(preset);
	}
	// AUTO/LOW/MEDIUM/HIGH map directly onto TCL's own semantics, so they stay
	// built-in ClimateFanMode values (HA already renders them correctly).
	// Silent/power/in-between-levels are exposed as custom fan modes (set up
	// once in setup(), see set_supported_custom_fan_modes() there).
	traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
	traits.add_supported_fan_mode(climate::CLIMATE_FAN_LOW);
	traits.add_supported_fan_mode(climate::CLIMATE_FAN_MEDIUM);
	traits.add_supported_fan_mode(climate::CLIMATE_FAN_HIGH);
	if (this->supported_swing_modes_.empty()) {
		traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
	} else {
		for (auto swing_mode : this->supported_swing_modes_)
			traits.add_supported_swing_mode(swing_mode);
	}

	return traits;
}


void tclacClimate::setup() {

	// Custom fan modes are set on the Climate entity itself (not on the local
	// ClimateTraits built in traits()) - get_traits() merges them in for us.
	std::vector<const char *> custom_fan_modes;
	custom_fan_modes.push_back(FAN_MODE_SILENT);
	if (this->fan_speed_levels_ == 5) custom_fan_modes.push_back(FAN_MODE_LOW_MEDIUM);
	if (this->fan_speed_levels_ == 5) custom_fan_modes.push_back(FAN_MODE_MEDIUM_HIGH);
	custom_fan_modes.push_back(FAN_MODE_POWER);
	this->set_supported_custom_fan_modes(custom_fan_modes);

#ifdef CONF_RX_LED
	this->rx_led_pin_->setup();
	this->rx_led_pin_->digital_write(false);
#endif
#ifdef CONF_TX_LED
	this->tx_led_pin_->setup();
	this->tx_led_pin_->digital_write(false);
#endif
}

void tclacClimate::loop()  {
	// If there is something in the UART buffer, then read that something
	if (esphome::uart::UARTDevice::available() > 0) {
		dataShow(0, true);
		dataRX[0] = esphome::uart::UARTDevice::read();
		// If the received byte is not the header (0xBB), then just leave the loop
		if (dataRX[0] != 0xBB) {
			ESP_LOGD("TCL", "Wrong byte");
			dataShow(0,0);
			return;
		}
		// But if the header matched (0xBB), then we start reading another 4 bytes in a chain
		// Sometimes, for some air conditioners, it is still necessary to add delay(5) between packets. Why- I don't know, but it is necessary. But not always. Although sometimes- yes. But not every time. Rarely. It happens.
		// delay(5);
		dataRX[1] = esphome::uart::UARTDevice::read();
		// delay(5);
		dataRX[2] = esphome::uart::UARTDevice::read();
		// delay(5);
		dataRX[3] = esphome::uart::UARTDevice::read();
		// delay(5);
		dataRX[4] = esphome::uart::UARTDevice::read();

		//auto raw = getHex(dataRX, 5);
		//ESP_LOGD("TCL", "first 5 byte : %s ", raw.c_str());

		// From the first 5 bytes we need the fifth- it contains the message length
		esphome::uart::UARTDevice::read_array(dataRX+5, dataRX[4]+1);

		// Get checksum:
		uint8_t packet_len = 0;
		if (dataRX[4] == 0x3e){
			// For a 68-byte data packet
			packet_len = 68;
		} else if (dataRX[4] == 0x37){
			// For a 61-byte data packet
			packet_len = 61;
		} else {
			// For a 65-byte data packet
			packet_len = 65;
		}
		check = getChecksum(dataRX, packet_len);

		auto raw = getHex(dataRX, packet_len);
		ESP_LOGD("TCL", "RX: %s", raw.c_str());
		
		// Verify checksum:
		if (dataRX[4] == 0x3e){
			// For a 68-byte data packet
			if (check != dataRX[67]) {
				ESP_LOGD("TCL", "Invalid checksum %x", check);
				this->dataShow(0,0);
				return;
			} else {
				//ESP_LOGD("TCL", "checksum OK %x", check);
			}
		} else if (dataRX[4] == 0x37){
			if (check != dataRX[60]) {
				// For a 61-byte data packet
				ESP_LOGD("TCL", "Invalid checksum %x", check);
				this->dataShow(0,0);
				return;
			} else {
				//ESP_LOGD("TCL", "checksum OK %x", check);
			}
		} else {
			if (check != dataRX[64]) {
				// For a 65-byte data packet
				ESP_LOGD("TCL", "Invalid checksum %x", check);
				this->dataShow(0,0);
				return;
			} else {
				//ESP_LOGD("TCL", "checksum OK %x", check);
			}
		}
		this->dataShow(0,0);
		// Having read everything from the buffer, we proceed to parse the data
		this->readData();
	}
}

void tclacClimate::update() {
	tclacClimate::dataShow(1,1);
	this->esphome::uart::UARTDevice::write_array(poll, sizeof(poll));
	auto raw = tclacClimate::getHex(poll, sizeof(poll));
	ESP_LOGD("TCL", "TX (poll): %s", raw.c_str());
	tclacClimate::dataShow(1,0);
}

void tclacClimate::readData() {
	
	// This construct was suggested by the Claude neural network, I don't understand such refinements at all, so I insert it as it is.
	current_temperature = ((float)((dataRX[17] << 8) | dataRX[18]) / 374.0f - 32.0f) / 1.8f;
	
	target_temperature = (dataRX[FAN_SPEED_POS] & SET_TEMP_MASK) + 16;

	//ESP_LOGD("TCL", "TEMP: %f ", current_temperature);

	if (dataRX[MODE_POS] & ( 1 << 4)) {
		// If the air conditioner is on, then parse the data for display
		ESP_LOGD("TCL", "AC is on");
		uint8_t modeswitch = MODE_MASK & dataRX[MODE_POS];
		uint8_t fan_speed_code = FAN_SPEED_CODE(dataRX[FAN_SPEED_POS]);
		uint8_t swingmodeswitch = SWING_MODE_MASK & dataRX[SWING_POS];

		switch (modeswitch) {
			case MODE_AUTO:
				this->mode = climate::CLIMATE_MODE_HEAT_COOL;
				break;
			case MODE_COOL:
				this->mode = climate::CLIMATE_MODE_COOL;
				break;
			case MODE_DRY:
				this->mode = climate::CLIMATE_MODE_DRY;
				break;
			case MODE_FAN_ONLY:
				this->mode = climate::CLIMATE_MODE_FAN_ONLY;
				break;
			case MODE_HEAT:
				this->mode = climate::CLIMATE_MODE_HEAT;
				break;
			default:
				ESP_LOGE("TCL", "Received unknown mode: %02x", modeswitch);
		}

		if (dataRX[FAN_QUIET_POS] & FAN_QUIET) {
			this->set_custom_fan_mode_(FAN_MODE_SILENT);
		} else if (dataRX[MODE_POS] & FAN_POWER) {
			this->set_custom_fan_mode_(FAN_MODE_POWER);
		} else {
			switch (fan_speed_code) {
				case 0:
					this->set_fan_mode_(climate::CLIMATE_FAN_AUTO);
					break;
				case 1:
					this->set_fan_mode_(climate::CLIMATE_FAN_LOW);
					break;
				case 2:
					this->set_fan_mode_(climate::CLIMATE_FAN_MEDIUM);
					break;
				case 3:
					this->set_fan_mode_(climate::CLIMATE_FAN_HIGH);
					break;
				case 4:
					this->set_custom_fan_mode_(FAN_MODE_LOW_MEDIUM);
					break;
				case 5:
					this->set_custom_fan_mode_(FAN_MODE_MEDIUM_HIGH);
					break;
				default:
					ESP_LOGE("TCL", "Received unknown fan speed code: %u", fan_speed_code);
			}
		}

		switch (swingmodeswitch) {
			case SWING_OFF: 
				swing_mode = climate::CLIMATE_SWING_OFF;
				break;
			case SWING_HORIZONTAL:
				swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
				break;
			case SWING_VERTICAL:
				swing_mode = climate::CLIMATE_SWING_VERTICAL;
				break;
			case SWING_BOTH:
				swing_mode = climate::CLIMATE_SWING_BOTH;
				break;
            default:
                ESP_LOGE("TCL", "Received unknown swing mode: %02x", swingmodeswitch);
		}
		
		// Preset data processing
		preset = ClimatePreset::CLIMATE_PRESET_NONE;
		if (dataRX[7] & (1 << 6)){
			preset = ClimatePreset::CLIMATE_PRESET_ECO;
		} else if (dataRX[9] & (1 << 2)){
			preset = ClimatePreset::CLIMATE_PRESET_COMFORT;
		} else if (dataRX[19] & (1 << 0)){
			preset = ClimatePreset::CLIMATE_PRESET_SLEEP;
		}
		
	} else {
		ESP_LOGD("TCL", "AC is OFF");
		// If the air conditioner is off, all modes are shown as off
		this->mode = climate::CLIMATE_MODE_OFF;
		//fan_mode = climate::CLIMATE_FAN_OFF;
		this->swing_mode = climate::CLIMATE_SWING_OFF;
		this->preset = ClimatePreset::CLIMATE_PRESET_NONE;
	}
	// Publish state
	this->publish_state();
	allow_take_control = true;
   }

// Climate control
void tclacClimate::control(const climate::ClimateCall &call) {
	
	ESP_LOGD("TCL", "Call from UI");
	
	// And this and below I trimmed from Vi3jo.
	
	if (call.get_mode().has_value()) this->mode = *call.get_mode();
    if (call.get_target_temperature().has_value()) this->target_temperature = *call.get_target_temperature();
    if (call.get_fan_mode().has_value()) this->set_fan_mode_(*call.get_fan_mode());
    if (call.has_custom_fan_mode()) this->set_custom_fan_mode_(call.get_custom_fan_mode());
	if (call.get_swing_mode().has_value()) this->swing_mode = *call.get_swing_mode();
	if (call.get_preset().has_value()) this->preset = *call.get_preset();
	
	this->publish_state();
	this->takeControl();
	this->allow_take_control = true;
}
	
	
void tclacClimate::takeControl() {
	
	dataTX[7]  = 0b00000000;
	dataTX[8]  = 0b00000000;
	dataTX[9]  = 0b00000000;
	dataTX[10] = 0b00000000;
	dataTX[11] = 0b00000000;
	dataTX[19] = 0b00000000;
	dataTX[32] = 0b00000000;
	dataTX[33] = 0b00000000;
	
	uint8_t target_temperature_set = 31-(int)target_temperature;
	
	// Enable or disable the beeper depending on the switch in the settings
	if (beeper_status_){
		ESP_LOGD("TCL", "Beep mode ON");
		dataTX[7] += 0b00100000;
	} else {
		ESP_LOGD("TCL", "Beep mode OFF");
		dataTX[7] += 0b00000000;
	}
	
	// Enable or disable the display on the air conditioner depending on the switch in the settings
	// Enable the display only if the air conditioner is in one of the operating modes
	
	// ATTENTION! When the display is turned off, the air conditioner itself forcibly goes into automatic mode!
	
	if ((display_status_) && (mode != climate::CLIMATE_MODE_OFF)){
		ESP_LOGD("TCL", "Dispaly turn ON");
		dataTX[7] += 0b01000000;
	} else {
		ESP_LOGD("TCL", "Dispaly turn OFF");
		dataTX[7] += 0b00000000;
	}
		
	// Configure air conditioner operation mode
	switch (this->mode) {
		case climate::CLIMATE_MODE_OFF:
			dataTX[7] += 0b00000000;
			dataTX[8] += 0b00000000;
			break;
		case climate::CLIMATE_MODE_HEAT_COOL:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00001000;
			break;
		case climate::CLIMATE_MODE_COOL:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000011;	
			break;
		case climate::CLIMATE_MODE_DRY:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000010;	
			break;
		case climate::CLIMATE_MODE_FAN_ONLY:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000111;	
			break;
		case climate::CLIMATE_MODE_HEAT:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000001;	
			break;
	}

	// Configure fan mode
	if (this->has_custom_fan_mode()) {
		esphome::StringRef custom_fan_mode = this->get_custom_fan_mode();
		if (custom_fan_mode == FAN_MODE_SILENT) {
			dataTX[8]	+= 0b10000000;
		} else if (custom_fan_mode == FAN_MODE_POWER) {
			dataTX[8]	+= 0b01000000;
		} else if (custom_fan_mode == FAN_MODE_LOW_MEDIUM) {
			dataTX[10]	+= this->encode_fan_speed_tx_(4);
		} else if (custom_fan_mode == FAN_MODE_MEDIUM_HIGH) {
			dataTX[10]	+= this->encode_fan_speed_tx_(5);
		}
	} else if (this->fan_mode.has_value()) {
		switch (*this->fan_mode) {
			case climate::CLIMATE_FAN_LOW:
				dataTX[10]	+= this->encode_fan_speed_tx_(1);
				break;
			case climate::CLIMATE_FAN_MEDIUM:
				dataTX[10]	+= this->encode_fan_speed_tx_(2);
				break;
			case climate::CLIMATE_FAN_HIGH:
				dataTX[10]	+= this->encode_fan_speed_tx_(3);
				break;
			default:
				// CLIMATE_FAN_AUTO needs no bits set; dataTX[8]/dataTX[10] already default to 0.
				break;
		}
	}
	
	// Set swing mode
	switch(this->swing_mode) {
		case climate::CLIMATE_SWING_OFF:
			dataTX[10]	+= 0b00000000;
			dataTX[11]	+= 0b00000000;
			break;
		case climate::CLIMATE_SWING_VERTICAL:
			dataTX[10]	+= 0b00111000;
			dataTX[11]	+= 0b00000000;
			break;
		case climate::CLIMATE_SWING_HORIZONTAL:
			dataTX[10]	+= 0b00000000;
			dataTX[11]	+= 0b00001000;
			break;
		case climate::CLIMATE_SWING_BOTH:
			dataTX[10]	+= 0b00111000;
			dataTX[11]	+= 0b00001000;  
			break;
	}
	
	// Set air conditioner presets
	if (this->preset.has_value()) {
		switch(*this->preset) {
			case ClimatePreset::CLIMATE_PRESET_NONE:
				break;
			case ClimatePreset::CLIMATE_PRESET_ECO:
				dataTX[7]	+= 0b10000000;
				break;
			case ClimatePreset::CLIMATE_PRESET_SLEEP:
				dataTX[19]	+= 0b00000001;
				break;
			case ClimatePreset::CLIMATE_PRESET_COMFORT:
				dataTX[8]	+= 0b00010000;
				break;
		}
	}

        //Swing mode
		//	Vertical swing
		//		Vertical swing [10 byte, mask 00111000]:
		//			000 - Swing disabled, louver in last position or fixated
		//			111 - Swing enabled in selected mode
		//		Vertical swing mode (fixation mode doesn't matter if swing is enabled) [32 byte, mask 00011000]:
		//			01 - swing up and down, DEFAULT
		//			10 - swing in upper half
		//			11 - swing in lower half
		//		Louver fixation mode (swing mode doesn't matter if swing is disabled) [32 byte, mask 00000111]:
		//			000 - no fixation, DEFAULT
		//			001 - fixation at top
		//			010 - fixation between top and middle
		//			011 - fixation in middle
		//			100 - fixation between middle and bottom
		//			101 - fixation at bottom
		//	Horizontal swing
		//		Horizontal swing [11 byte, mask 00001000]:
		//			0 - Swing disabled, louvers in last position or fixated
		//			1 - Swing enabled in selected mode
		//		Horizontal swing mode (fixation mode doesn't matter if swing is enabled) [33 byte, mask 00111000]:
		//			001 - swing left to right, DEFAULT
		//			010 - swing left
		//			011 - swing in center
		//			100 - swing right
		//		Horizontal louver fixation mode (swing mode doesn't matter if swing is disabled) [33 byte, mask 00000111]:
		//			000 - no fixation, DEFAULT
		//			001 - fixation at left
		//			010 - fixation between left side and middle
		//			011 - fixation in middle
		//			100 - fixation between middle and right side
		//			101 - fixation at right
		
		
	// Set mode for vertical swing direction
	switch(vertical_swing_direction_) {
		case VerticalSwingDirection::UP_DOWN:
			dataTX[32]	+= 0b00001000;
			ESP_LOGD("TCL", "Vertical swing: up-down");
			break;
		case VerticalSwingDirection::UPSIDE:
			dataTX[32]	+= 0b00010000;
			ESP_LOGD("TCL", "Vertical swing: upper");
			break;
		case VerticalSwingDirection::DOWNSIDE:
			dataTX[32]	+= 0b00011000;
			ESP_LOGD("TCL", "Vertical swing: downer");
			break;
	}
	// Set mode for horizontal swing direction
	switch(horizontal_swing_direction_) {
		case HorizontalSwingDirection::LEFT_RIGHT:
			dataTX[33]	+= 0b00001000;
			ESP_LOGD("TCL", "Horizontal swing: left-right");
			break;
		case HorizontalSwingDirection::LEFTSIDE:
			dataTX[33]	+= 0b00010000;
			ESP_LOGD("TCL", "Horizontal swing: lefter");
			break;
		case HorizontalSwingDirection::CENTER:
			dataTX[33]	+= 0b00011000;
			ESP_LOGD("TCL", "Horizontal swing: center");
			break;
		case HorizontalSwingDirection::RIGHTSIDE:
			dataTX[33]	+= 0b00100000;
			ESP_LOGD("TCL", "Horizontal swing: righter");
			break;
	}
	// Set fixation position for vertical louver
	switch(vertical_direction_) {
		case AirflowVerticalDirection::LAST:
			dataTX[32]	+= 0b00000000;
			ESP_LOGD("TCL", "Vertical fix: last position");
			break;
		case AirflowVerticalDirection::MAX_UP:
			dataTX[32]	+= 0b00000001;
			ESP_LOGD("TCL", "Vertical fix: up");
			break;
		case AirflowVerticalDirection::UP:
			dataTX[32]	+= 0b00000010;
			ESP_LOGD("TCL", "Vertical fix: upper");
			break;
		case AirflowVerticalDirection::CENTER:
			dataTX[32]	+= 0b00000011;
			ESP_LOGD("TCL", "Vertical fix: center");
			break;
		case AirflowVerticalDirection::DOWN:
			dataTX[32]	+= 0b00000100;
			ESP_LOGD("TCL", "Vertical fix: downer");
			break;
		case AirflowVerticalDirection::MAX_DOWN:
			dataTX[32]	+= 0b00000101;
			ESP_LOGD("TCL", "Vertical fix: down");
			break;
	}
	// Set fixation position for horizontal louvers
	switch(horizontal_direction_) {
		case AirflowHorizontalDirection::LAST:
			dataTX[33]	+= 0b00000000;
			ESP_LOGD("TCL", "Horizontal fix: last position");
			break;
		case AirflowHorizontalDirection::MAX_LEFT:
			dataTX[33]	+= 0b00000001;
			ESP_LOGD("TCL", "Horizontal fix: left");
			break;
		case AirflowHorizontalDirection::LEFT:
			dataTX[33]	+= 0b00000010;
			ESP_LOGD("TCL", "Horizontal fix: lefter");
			break;
		case AirflowHorizontalDirection::CENTER:
			dataTX[33]	+= 0b00000011;
			ESP_LOGD("TCL", "Horizontal fix: center");
			break;
		case AirflowHorizontalDirection::RIGHT:
			dataTX[33]	+= 0b00000100;
			ESP_LOGD("TCL", "Horizontal fix: righter");
			break;
		case AirflowHorizontalDirection::MAX_RIGHT:
			dataTX[33]	+= 0b00000101;
			ESP_LOGD("TCL", "Horizontal fix: right");
			break;
	}

	// Temperature setting
	dataTX[9] = target_temperature_set;
		
	// Collect byte array to send to the air conditioner
	dataTX[0] = 0xBB;	//start header byte
	dataTX[1] = 0x00;	//start header byte
	dataTX[2] = 0x01;	//start header byte
	dataTX[3] = 0x03;	//0x03 - control, 0x04 - poll
	dataTX[4] = 0x20;	//0x20 - control, 0x19 - poll
	dataTX[5] = 0x03;	//??
	dataTX[6] = 0x01;	//??
	//dataTX[7] = 0x64;	//eco,display,beep,ontimerenable, offtimerenable,power,0,0
	//dataTX[8] = 0x08;	//mute,0,turbo,health, mode(4) mode 01 heat, 02 dry, 03 cool, 07 fan, 08 auto, health(+16), 41=turbo-heat 43=turbo-cool (turbo = 0x40+ 0x01..0x08)
	//dataTX[9] = 0x0f;	//0 -31 ;    15 - 16 0,0,0,0, temp(4) settemp 31 - x
	//dataTX[10] = 0x00;	//0,timerindicator,swingv(3),fan(3) fan+swing modes //0=auto 1=low 2=med 3=high
	//dataTX[11] = 0x00;	//0,offtimer(6),0
	dataTX[12] = 0x00;	//fahrenheit,ontimer(6),0 cf 80=f 0=c
	dataTX[13] = 0x01;	//??
	dataTX[14] = 0x00;	//0,0,halfdegree,0,0,0,0,0
	dataTX[15] = 0x00;	//??
	dataTX[16] = 0x00;	//??
	dataTX[17] = 0x00;	//??
	dataTX[18] = 0x00;	//??
	//dataTX[19] = 0x00;	//sleep on = 1 off=0
	dataTX[20] = 0x00;	//??
	dataTX[21] = 0x00;	//??
	dataTX[22] = 0x00;	//??
	dataTX[23] = 0x00;	//??
	dataTX[24] = 0x00;	//??
	dataTX[25] = 0x00;	//??
	dataTX[26] = 0x00;	//??
	dataTX[27] = 0x00;	//??
	dataTX[28] = 0x00;	//??
	dataTX[29] = 0x00;	//??
	dataTX[30] = 0x00;	//??
	dataTX[31] = 0x00;	//??
	//dataTX[32] = 0x00;	//0,0,0,vertical swing mode(2),vertical fixation mode(3)
	//dataTX[33] = 0x00;	//0,0,horizontal swing mode(3),horizontal fixation mode(3)
	dataTX[34] = 0x00;	//??
	dataTX[35] = 0x00;	//??
	dataTX[36] = 0x00;	//??
	dataTX[37] = 0xFF;	//Checksum
	dataTX[37] = tclacClimate::getChecksum(dataTX, sizeof(dataTX));
	
	tclacClimate::sendData(dataTX, sizeof(dataTX));
	allow_take_control = false;
	is_call_control = false;
}

// Sending data to the air conditioner
void tclacClimate::sendData(uint8_t * message, uint8_t size) {
	tclacClimate::dataShow(1,1);
	//Serial.write(message, size);
	this->esphome::uart::UARTDevice::write_array(message, size);
	auto raw = getHex(message, size);
	ESP_LOGD("TCL", "TX: %s", raw.c_str());
	tclacClimate::dataShow(1,0);
}

// Converting byte array to hex string
String tclacClimate::getHex(uint8_t *message, uint8_t size) {
	char buffer[4];
	String raw = "";
	for (int i = 0; i < size; i++) {
		sprintf(buffer, "%02X ", message[i]);
		raw += buffer;
	}
	return raw;
}

// Checksum calculation
uint8_t tclacClimate::getChecksum(const uint8_t * message, size_t size) {
	uint8_t position = size - 1;
	uint8_t crc = 0;
	for (int i = 0; i < position; i++)
		crc ^= message[i];
	return crc;
}

// Blink LEDs
void tclacClimate::dataShow(bool flow, bool shine) {
	if (module_display_status_){
		if (flow == 0){
			if (shine == 1){
#ifdef CONF_RX_LED
				this->rx_led_pin_->digital_write(true);
#endif
			} else {
#ifdef CONF_RX_LED
				this->rx_led_pin_->digital_write(false);
#endif
			}
		}
		if (flow == 1) {
			if (shine == 1){
#ifdef CONF_TX_LED
				this->tx_led_pin_->digital_write(true);
#endif
			} else {
#ifdef CONF_TX_LED
				this->tx_led_pin_->digital_write(false);
#endif
			}
		}
	}
}

// Actions with data from config
	
// Getting beeper state
void tclacClimate::set_beeper_state(bool state) {
	this->beeper_status_ = state;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Getting air conditioner display state
void tclacClimate::set_display_state(bool disp_state) {
	this->display_status_ = disp_state;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Getting force settings application mode state
void tclacClimate::set_force_mode_state(bool f_state) {
	this->force_mode_status_ = f_state;
}
// Getting RX LED pin
#ifdef CONF_RX_LED
void tclacClimate::set_rx_led_pin(GPIOPin *rx_led_pin) {
	this->rx_led_pin_ = rx_led_pin;
}
#endif
// Getting TX LED pin
#ifdef CONF_TX_LED
void tclacClimate::set_tx_led_pin(GPIOPin *tx_led_pin) {
	this->tx_led_pin_ = tx_led_pin;
}
#endif
// Getting module communication LEDs state
void tclacClimate::set_module_display_state(bool d_state) {
	this->module_display_status_ = d_state;
}
// Getting vertical airflow fixation mode
void tclacClimate::set_vertical_airflow(AirflowVerticalDirection v_airflow) {
	this->vertical_direction_ = v_airflow;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Getting horizontal airflow fixation mode
void tclacClimate::set_horizontal_airflow(AirflowHorizontalDirection h_airflow) {
	this->horizontal_direction_ = h_airflow;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Getting vertical swing direction mode
void tclacClimate::set_vertical_swing_direction(VerticalSwingDirection vs_direction) {
	this->vertical_swing_direction_ = vs_direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Getting available air conditioner operation modes
void tclacClimate::set_supported_modes(climate::ClimateModeMask modes) {
	this->supported_modes_ = modes;
	ESP_LOGD("TCL", "Set up Modes");
}
// Getting horizontal swing direction mode
void tclacClimate::set_horizontal_swing_direction(HorizontalSwingDirection hs_direction) {
	horizontal_swing_direction_ = hs_direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Setting how many fan speed levels the unit supports (3 or 5)
void tclacClimate::set_fan_speed_levels(uint8_t levels){
	this->fan_speed_levels_ = levels;
}

// Encode a fan speed code into the bits sent in dataTX[10]. Matches the byte
// values used by the previously working MIDDLE/MEDIUM/HIGH/FOCUS enum mapping
// (verified on hardware for the 5-level unit).
// For the 3-level unit this is UNVERIFIED beyond codes 1-3: it assumes the
// TX fan field takes the same raw code as it reports on RX - please confirm
// on real hardware (especially silent/power) before relying on it.
uint8_t tclacClimate::encode_fan_speed_tx_(uint8_t code) const {
	switch (code) {
		case 1: return 0b00000001; // low
		case 2: return 0b00000011; // medium
		case 3: return 0b00000101; // high
		case 4: return 0b00000110; // low-medium
		case 5: return 0b00000111; // medium-high
		default: return 0;
	}
}
// Getting available swing modes
void tclacClimate::set_supported_swing_modes(climate::ClimateSwingModeMask swing_modes) {
	this->supported_swing_modes_ = swing_modes;
}
// Getting available presets
void tclacClimate::set_supported_presets(climate::ClimatePresetMask presets) {
  this->supported_presets_ = presets;
}


}
}
