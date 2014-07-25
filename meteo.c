/**
 * @file px4_simple_app.c
 * Minimal application example for PX4 autopilot.
 */

#include <nuttx/config.h>
#include <systemlib/err.h>
#include <stdio.h>
#include <errno.h>

#include <uORB/uORB.h>
#include <uORB/topics/debug_key_value.h>

#include <board_config.h>
#include <nuttx/i2c.h>

__EXPORT int meteo_main(int argc, char *argv[]);
void meteo_measure(void);
void meteo_transmit(void);

static const uint8_t hyt271_buf[1] = {0x0};

uint8_t hyt271_data[4];
float   humidity = 0.0;
float   temperature = 0.0;
bool    temp_or_hum = false;

struct i2c_dev_s *px4bus;
//orb_advert_t pub_temperature;
orb_advert_t pub_humidity;

struct i2c_msg_s msgv[2];

struct debug_key_value_s tmp_topic = { 
    .key = "temp", .value = 0.0f 
};

struct debug_key_value_s hum_topic = { 
    .key = "humidity", .value = 0.0f 
};

void meteo_measure(void)
{
    msgv[0].addr = 0x28;
	msgv[0].flags = 0;
	msgv[0].buffer = hyt271_data;
	msgv[0].length = 1;

    I2C_TRANSFER(px4bus, &msgv[0], 1);

    usleep(1000*60);

	msgv[0].addr = 0x28;
	msgv[0].flags = I2C_M_READ;
	msgv[0].buffer = hyt271_data;
	msgv[0].length = 4;

    //I2C_SETFREQUENCY(px4bus, 400000);
    I2C_TRANSFER(px4bus, &msgv[0], 1);

    humidity = ((hyt271_data[0] & 0x3f) << 8 | hyt271_data[1]) * (100.0 / 0x3fff);
    temperature = (hyt271_data[2] << 8 | (hyt271_data[3] & 0xfc)) * (165.0 / 0xfffc) - 40;
}

void meteo_transmit(void)
{
    tmp_topic.value = temperature;
    hum_topic.value = humidity;

    //warnx("Temp: %8.4f", (double)temperature);
    //warnx("Hum: %8.4f", (double)humidity);

    if(true == temp_or_hum) 
    {
        orb_publish(ORB_ID(debug_key_value), pub_humidity, &tmp_topic);
        temp_or_hum = false;
    } else 
    {
        orb_publish(ORB_ID(debug_key_value), pub_humidity, &hum_topic);
        temp_or_hum = true;
    }
}

int meteo_main(int argc, char *argv[])
{
    px4bus = up_i2cinitialize(PX4_I2C_BUS_EXPANSION);

	if (px4bus == NULL)
		errx(1, "failed to locate I2C bus");

    I2C_SETADDRESS(px4bus, 0x28, 7);

    //pub_temperature = orb_advertise(ORB_ID(debug_key_value), &tmp_topic);
    pub_humidity = orb_advertise(ORB_ID(debug_key_value), &hum_topic);

    while(true)
    {
        meteo_measure();
        meteo_transmit();
        usleep(500000);
    }

    return OK;
}

