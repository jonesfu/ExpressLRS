#ifdef HAS_THERMAL
#include "thermal.h"
#include "logging.h"

#ifdef HAS_THERMAL_LM75A
#include "lm75a.h"
LM75A lm75a;
#endif

uint8_t thermal_threshold_data[] = {
    THERMAL_FAN_DEFAULT_HIGH_THRESHOLD,
    THERMAL_FAN_DEFAULT_LOW_THRESHOLD,
    THERMAL_FAN_ALWAYS_ON_HIGH_THRESHOLD,
    THERMAL_FAN_ALWAYS_ON_LOW_THRESHOLD,
    THERMAL_FAN_OFF_HIGH_THRESHOLD,
    THERMAL_FAN_OFF_LOW_THRESHOLD
};

int thermal_status = THERMAL_STATUS_FAIL;

void Thermal::init()
{
    Wire.begin(GPIO_PIN_SDA, GPIO_PIN_SCL);

    int status = -1;
#ifdef HAS_THERMAL_LM75A
    status = lm75a.init();
#endif    
    if(status == -1)
    {
        ERRLN("Thermal failed!");
    }
    else
    {
        INFOLN("Thermal OK!");
        temp_value = 0;
        thermal_status = THERMAL_STATUS_NORMAL;
        update_threshold(0);
    }
}

void Thermal::handle()
{
    read_temp(&temp_value);
}

void Thermal::read_temp(float *temp)
{
    if(thermal_status != THERMAL_STATUS_NORMAL)
    {
        ERRLN("thermal not ready!");
        return;
    }
#ifdef HAS_THERMAL_LM75A    
    lm75a.read_lm75a(temp);
#endif    
}

void Thermal::update_threshold(int index)
{
    if(thermal_status != THERMAL_STATUS_NORMAL)
    {
        ERRLN("thermal not ready!");
        return;
    }
    int size = sizeof(thermal_threshold_data)/sizeof(thermal_threshold_data[0]);
    if(index > size/2)
    {
        ERRLN("thermal index out of range!");
        return;
    }
    uint8_t high = thermal_threshold_data[2*index];
    uint8_t low = thermal_threshold_data[2*index+1];
#ifdef HAS_THERMAL_LM75A    
    lm75a.update_lm75a_threshold(high, low);
#endif    
}

float Thermal::getTempValue()
{
    return temp_value;
}

#endif