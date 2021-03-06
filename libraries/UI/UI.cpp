/*
 * UI.cpp
 *
 *  Created on: 25.08.2015
 */

#include "Arduino.h"
#include "Motor.h"
#include "RotaryEncoder.h"
#include "UI.h"

UI::UI(unsigned long interval, IR* ir, Radio* radio, Motor* motor, PID* pid,
       IMU* imu, KeypadWrapper* kpd, TimersClass* timer) {
  m_interval = interval;
  // m_uiState = NORMAL;
  m_ir = ir;
  m_motor = motor;
  m_pid = pid;
  m_imu = imu;
  m_kpd = kpd;
  m_timer = timer;
  m_radio = radio;

  m_cmdSerial = "";
  m_cmdKeypad = "";
  m_counter = 0;
  m_debugDevisor = 10;
}

UI::~UI() {
  // TODO Auto-generated destructor stub
}

void UI::update() {
  // Serial commands
  while (Serial.available() > 0) {
    char incoming = Serial.read();
    if (incoming != '\n' && incoming != ' ' && incoming != '\r') {
      m_cmdSerial += incoming;
    } else {
      exec(m_cmdSerial);
      m_cmdSerial = "";
    }
    // prevent misuse
    if (m_cmdSerial.length() > 60) {
      m_cmdSerial = "";
    }
  }

  if (m_ir != NULL) {
    String irCmd = m_ir->getCommand();
    if (irCmd != "") {
      exec(irCmd);
    }
  }

  if (m_radio != NULL) {
    switch (m_radio->getLastKey()) {
      case Radio::A:
        Serial.println("A");
        break;
      case Radio::B:
        Serial.println("B");
        break;
      case Radio::C:
        Serial.println("C");
        break;
      case Radio::D:
        Serial.println("D");
        break;
        /*evt:
         * a = I001
         * b = D001
         * ca = I010
         * cb = D010
         * ceba = GP
         * d = T
         */

      default:
        break;
    }
  }
  if (m_kpd != NULL) {
	  String kpdCmd = m_kpd->getCommand();
	  if (kpdCmd != "") {
		exec(kpdCmd);
		Serial.println(kpdCmd);
	  }
  }

  if (m_counter >= m_debugDevisor) {
    m_timer->debug(Serial);
    m_counter = 0;
  }
  m_counter++;
}
void UI::exec(String cmd) {
  if (cmd == "?")
    msg_help(Serial);
  else if (cmd == "DELCAL")
    m_imu->deleteCalibration();
  else if (cmd == "SETROT")
    m_imu->setCurrentRotationAsRef();
  else if (cmd == "DISCAL")
    m_imu->disableCalibration();
  else if (cmd == "ENCAL")
    m_imu->enableCalibration();
  else if (cmd == "RESETROT")
    m_imu->resetRotationRef();
  else if (cmd == "T")
    m_pid->tack();
  else if (cmd == "GP") {
    m_pid->setInactiv();
    m_motor->gotoParking();
  } else if (cmd == "I") {
    m_pid->setInactiv();
    m_motor->initialize();
    m_pid->resetErrorSum();
  } else if (cmd == "Z") {
	  m_pid->setInactiv();
	  m_pid->resetErrorSum();
	  m_motor->m_rotaryEncoder->m_currentPosition = 0;
  } else if (cmd == "RI") {
    m_pid->setInactiv();
    m_motor->reinitialize();
    m_pid->resetErrorSum();
  } else if (cmd == "S") {
    m_pid->setInactiv();
    m_motor->stop();
  } else if (cmd == "S2")
    m_motor->stop();
  else if (cmd == "M")
    m_pid->setMag();
  else if (cmd == "W")
    m_pid->setWind();
  else if (cmd.length() > 1) {
    if (cmd.charAt(0) == 'P')  // Parameter
    {
      String val = cmd.substring(2);
      float value = val.toFloat();

      switch (cmd.charAt(1)) {
        case 'P':
          m_pid->m_P = value;
          break;
        case 'I':
          m_pid->m_errorSum = 0.0;
          m_pid->m_I = value;
          break;
        case 'D':
          m_pid->m_D = value;
          break;
        case 'M':
          m_motor->gotoPos(value);
          break;
        case 'R':
          setDebugDevisor(value);
          break;
        case 'F':
          m_imu->setFilterFrequency(value);
          break;
        case 'C':
          m_pid->setFilterFrequency(value);
          break;
        case 'O':
          m_imu->setCalibrationOffset(value * PI / 180.0f);
          break;
        default:
          break;
      }
    } else if (cmd.charAt(0) == 'M')  // Magnet goal
    {
      String val = cmd.substring(1);
      float value = val.toFloat();
      m_pid->setMag(value);
    } else if (cmd.charAt(0) == 'W')  // Wind goal
    {
      String val = cmd.substring(1);
      float value = val.toFloat();
      m_pid->setWind(value);
    } else if (cmd.charAt(0) == 'I')  // Increase
    {
      String val = cmd.substring(1);
      float value = val.toFloat();
      m_pid->increase(value);
    } else if (cmd.charAt(0) == 'D')  // Decrease
    {
      String val = cmd.substring(1);
      float value = val.toFloat();
      m_pid->decrease(value);
    } else if (cmd.charAt(0) == 'C') // set calibration data for imu
    {
    	Serial.println("starting");
    	float values[6];
    	int index = 0;
    	int start = 1;
    	for(unsigned int end = 2; end < cmd.length(); end++)
    	{
    		if(cmd.charAt(end) == ',') {
    			Serial.println(cmd.substring(start, end));
    			values[index] = cmd.substring(start, end).toFloat();
    			Serial.println(values[index]);
    			index++;
    			start = end+1;
    		}
    	}
    	Serial.println(index);

    	if (index == 6)
    		Serial.println("setting");
    		m_imu->setMinMaxCalDat(values, values+3);
    }
  }
}

void UI::setDebugDevisor(int div) {
  if (div >= 1 && div < 10000) {
    m_debugDevisor = div;
  }
}

void UI::msg_help(HardwareSerial &serial) {
  serial.println(F("?: this help"));
  serial.println(F("DELCAL: delete IMU calibration"));
  serial.println(F("SETROT: set current rotation as reference"));
  serial.println(F("ENCAL: enable calibration"));
  serial.println(F("DISCAL: disable calibration"));
  serial.println(F("RESETROT: reset current rotation reference"));
  serial.println(F("T: tack"));
  serial.println(F("I: initialize"));
  serial.println(F("RI: reinitialize"));
  serial.println(F("S: stop"));
  serial.println(F("GP: go parking"));
  serial.println(F("M???: set magnet goal"));
  serial.println(F("W???: set Wind goal"));
  serial.println(F("I???: increase"));
  serial.println(F("D???: decrease"));
  serial.println(F("R[A-D]: Program Radio-key[A-D]"));
  serial.print(F("P[P,I,D,M,R,F,C,O][-DBL_MAX;DBL_MAX]: set Parameter controller"));
  serial.print(F("(PID-control-parameters, motor-position, debug Rate inverse,"));
  serial.println(F("lowpassFreqOrientation, lowpassFreqMotor, orientation offset"));
  serial.println(F("'C-69,-48,-220,122,144,50,': Set IMU calibration data"));
}

void UI::debug(HardwareSerial& serial) { serial.print("-"); }

void UI::debugHeader(HardwareSerial& serial) { serial.print("UI"); }
