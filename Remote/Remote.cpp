#include "Remote.h"

Remote::Remote(void)
{
  Display.init( this );
}

void Remote::begin(void)
{
  // Prepare OLED for I2C communication
  Display.begin();

  // Load settings from the EEPROM to the Settings class
  Settings.load();
  
  pinMode(PIN_USBDETECT, INPUT);
  pinMode(PIN_UPPERTRIGGER, INPUT_PULLUP);
  pinMode(PIN_UPPERTRIGGER, INPUT_PULLUP);
}


void Remote::menuLoop( void ){
  
  if( upperTrigger() )
  {
    if( triggerFlag == false ){
  
      if( currentSetting == SETTINGS_COUNT){
        // Save settings and exit to main screen
        Settings.save();
        remoteSettings = false;
      }
  
      selectSetting = !selectSetting;
      triggerFlag = true;
    }
  }else{
    triggerFlag = false;
  }
  
  uint16_t value = Settings.getValue( currentSetting );
  
  if( getThrottle() > THROTTLE_CENTER + 200 ){
    // Add one to the selected setting or move a setting up  
    if( settingFlag == false && selectSetting){
      value++;
      if( Settings.inRange( value, currentSetting)){
        Settings.setValue(currentSetting, value);
        settingFlag = true;  
      }
    }
    else if( settingFlag == false && currentSetting > 0 ){
      currentSetting--;
      settingFlag = true;  
    }
  }else if( getThrottle() < THROTTLE_CENTER - 200){
    // Substract one to the selected setting or move a setting down
    if( settingFlag == false && selectSetting){
      value--;
      if( Settings.inRange( value, currentSetting)){
        Settings.setValue(currentSetting, value);
        settingFlag = true;  
      }
    }
    else if( settingFlag == false && currentSetting < SETTINGS_COUNT ){
      currentSetting++;
      settingFlag = true;  
    }
  }else if( getThrottle() >= THROTTLE_CENTER - 50 && getThrottle() <= THROTTLE_CENTER + 50){
    settingFlag = false;  
  }
  
}

uint8_t Remote::batteryPercentage(void)
{
	measureVoltage();

  Serial.println(voltage);

	if(voltage >= VOLTAGE_MAX)
		return 100;
	else
		return (uint8_t)( (voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN) * 100.0);
}


uint16_t Remote::getThrottle()
{
  return this->throttle;
}

void Remote::calculateThrottle( void ){
  // First sample the hall sensor value
  this->measureHallOutput();

  // Map the hall value to the corresponding settings
  if ( this->hallOutput >= Settings.throttleCenter )
  {
    this->throttle = constrain( map( this->hallOutput, Settings.throttleCenter, Settings.throttleMax, THROTTLE_CENTER, 1023), THROTTLE_CENTER, 1023 );
  } 
  else 
  {
    this->throttle = constrain( map( this->hallOutput, Settings.throttleMin, Settings.throttleCenter, 0, THROTTLE_CENTER), 0, THROTTLE_CENTER );
  }

  // Remove hall center noise
  if ( abs(this->throttle - THROTTLE_CENTER) < Settings.throttleDeadzone )
  {
    this->throttle = THROTTLE_CENTER;
  }
  
}

void Remote::measureHallOutput(void){

  uint16_t sum = 0;
  uint8_t samples = 5;
  
  for ( uint8_t i = 0; i < samples; i++ )
  {
    sum += analogRead(PIN_HALL);
  }
  
  uint16_t mean = sum / samples;
  this->hallRaw = mean;
  
  // Smooths the hallValue with a simple digital filter
  this->hallOutput = filter(mean, hallOutput, 0.75); 
}

void Remote::measureVoltage(void){

	uint16_t sum;
  uint8_t samples = 5;

	for (int i = 0; i < samples; ++i){
		sum += analogRead(PIN_VOLTAGE);
	}

	float mean = sum/samples;

	// Multiplying by 2 because of the voltage divider
	this->voltage = mean / 1023.0 * VOLTAGE_REF * 2;

}


bool Remote::upperTrigger()
{
  if( !digitalRead(PIN_UPPERTRIGGER) )
    return true;
  else
    return false;
}



bool Remote::lowerTrigger()
{
  if( !digitalRead(PIN_LOWERTRIGGER) )
    return true;
  else
    return false;
}

bool Remote::usbConnected()
{
  if( digitalRead(PIN_USBDETECT) )
    return true;
  else
    return false;
}

/* Exponential moving weighted average */
uint16_t Remote::filter( uint16_t newSample, uint16_t oldSample, float alpha ){
  return (uint16_t)((alpha * (float)newSample) + (1.0-alpha) * (float)oldSample);  
}
