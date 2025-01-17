#include "targets.h"
#include "common.h"
#include "device.h"

#ifdef HAS_TFT_SCREEN
#include "logging.h"
#include "Wire.h"
#include "config.h"
#include "POWERMGNT.h"
#include "hwTimer.h"


#include "input.h"
#include "screen.h"
Input input;
Screen screen;

#ifdef HAS_GSENSOR
#include "gsensor.h"
extern Gsensor gsensor;
#endif

#ifdef HAS_THERMAL
#include "thermal.h"
extern Thermal thermal;
#endif

#define SCREEN_DURATION 20

#define LOGO_DISPLAY_TIMEOUT  5000
boolean isLogoDisplayed = false;

#define SCREEN_IDLE_TIMEOUT  20000
uint32_t none_input_start_time = 0;
boolean isUserInputCheck = false;

#define UPDATE_TEMP_TIMEOUT  5000
uint32_t update_temp_start_time = 0;

boolean is_screen_flipped = false;
boolean is_pre_screen_flipped = false;

extern bool ICACHE_RAM_ATTR IsArmed();
extern void EnterBindingMode();
extern void ExitBindingMode();

#if defined(TARGET_TX)
extern TxConfig config;
#else
extern RxConfig config;
#endif

void ScreenUpdateCallback(int updateType)
{
  switch(updateType)
  {
    case USER_UPDATE_TYPE_RATE:
      DBGLN("User set AirRate %d", screen.getUserRateIndex());
      config.SetRate(screen.getUserRateIndex());
      break;
    case USER_UPDATE_TYPE_POWER:
      DBGLN("User set Power %d", screen.getUserPowerIndex());
      config.SetPower(screen.getUserPowerIndex());
      break;
    case USER_UPDATE_TYPE_RATIO:
      DBGLN("User set TLM RATIO %d", screen.getUserRatioIndex());
      config.SetTlm(screen.getUserRatioIndex());
      break;
    case USER_UPDATE_TYPE_BINDING:
      DBGLN("User request binding!");
      EnterBindingMode();
      break;
    case USER_UPDATE_TYPE_EXIT_BINDING:
      DBGLN("User request exit binding!");
      ExitBindingMode();
      break;      
    case USER_UPDATE_TYPE_WIFI:
      DBGLN("User request Wifi Update Mode!");
      connectionState = wifiUpdate;
      break;
    case USER_UPDATE_TYPE_SMARTFAN:
      DBGLN("User request SMART FAN Mode!");
      config.SetFanMode(screen.getUserSmartFanIndex());
      break;
    case USER_UPDATE_TYPE_POWERSAVING:
      DBGLN("User request Power Saving Mode!");
      config.SetMotionMode(screen.getUserPowerSavingIndex());
    default:
      DBGLN("Error handle user request %d", updateType);
      break;
  }
}

void handle(void)
{
  input.handle();

  if(!IsArmed() && !is_screen_flipped && connectionState != wifiUpdate)
  {
    int key;
    boolean isLongPressed;
    input.getKeyState(&key, &isLongPressed);
    if(screen.getScreenStatus() == SCREEN_STATUS_IDLE)
    {
#ifdef HAS_THERMAL      
      if(millis() - update_temp_start_time > UPDATE_TEMP_TIMEOUT)
      {
        screen.doTemperatureUpdate(thermal.getTempValue());
        update_temp_start_time = millis();
      }
#endif      
      if(isLongPressed)
      {
        screen.activeScreen();
      }
    }
    else if(screen.getScreenStatus() == SCREEN_STATUS_WORK)
    {
      if(!isUserInputCheck)
      {
        none_input_start_time = millis();
        isUserInputCheck = true;
      }

      if(key != INPUT_KEY_NO_PRESS)
      {
        INFOLN("user key = %d", key);
        isUserInputCheck = false;
        if(key == INPUT_KEY_DOWN_PRESS)
        {
          screen.doUserAction(USER_ACTION_DOWN);
        }
        else if(key == INPUT_KEY_UP_PRESS)
        {
          screen.doUserAction(USER_ACTION_UP);
        }
        else if(key == INPUT_KEY_LEFT_PRESS)
        {
          screen.doUserAction(USER_ACTION_LEFT);
        }
        else if(key == INPUT_KEY_RIGHT_PRESS)
        {
          screen.doUserAction(USER_ACTION_RIGHT);
        }
        else if(key == INPUT_KEY_OK_PRESS)
        {
          screen.doUserAction(USER_ACTION_CONFIRM);
        }
      }
      else
      {
        if((millis() - none_input_start_time) > SCREEN_IDLE_TIMEOUT)
        {
          isUserInputCheck = false;
          screen.idleScreen();
        }
      }
    }
  }

#ifdef HAS_GSENSOR
  is_screen_flipped = gsensor.isFlipped();

  if((is_screen_flipped == true) && (is_pre_screen_flipped == false))
  {
    screen.doScreenBackLight(SCREEN_BACKLIGHT_OFF);
  }
  else if((is_screen_flipped == false) && (is_pre_screen_flipped == true))
  {
    screen.doScreenBackLight(SCREEN_BACKLIGHT_ON);
  }
  is_pre_screen_flipped = is_screen_flipped;
#endif

}

static void initialize()
{
  Wire.begin(GPIO_PIN_SDA, GPIO_PIN_SCL);

  input.init();
  screen.updatecallback = &ScreenUpdateCallback;
  screen.init();
}

static int start()
{
    screen.doParamUpdate(config.GetRate(), (uint8_t)(POWERMGNT::currPower()), config.GetTlm(), config.GetMotionMode(), config.GetFanMode());
    return LOGO_DISPLAY_TIMEOUT;
}

static int event()
{
  if(!isLogoDisplayed)
  {
    return DURATION_IGNORE; // we are still displaying the startup message, so don't change the timeout
  }
  if(connectionState != wifiUpdate)
  {
      screen.doParamUpdate(config.GetRate(), (uint8_t)(POWERMGNT::currPower()), config.GetTlm(), config.GetMotionMode(), config.GetFanMode());
  }

  return SCREEN_DURATION;
}

static int timeout()
{
  if(screen.getScreenStatus() == SCREEN_STATUS_INIT)
  {
    isLogoDisplayed = true;
    screen.idleScreen();
  }

  handle();
  return SCREEN_DURATION;

}

device_t Screen_device = {
    .initialize = initialize,
    .start = start,
    .event = event,
    .timeout = timeout
};
#endif
