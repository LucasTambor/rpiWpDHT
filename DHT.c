#include "DHT.h"
// #include <stdio.h>


static dht_t DHT_TYPE;
static int PIN_DHT;

void dht_configure(dht_t dhtType, int pinDHT)
{
    PIN_DHT = pinDHT;
    DHT_TYPE = dhtType; 

    pinMode(PIN_DHT, OUTPUT);

	// Wait 1 sec to stabilize sensor
	delay(1000);
}

int read_dht_data(float* temperature, float* humidity)
{
	volatile uint8_t laststate	= HIGH;
	uint8_t counter	= 0;
	uint8_t j = 0;
	int data[] = {0, 0, 0, 0, 0};
 
	
 
	/* pull pin down for 20 milliseconds */
	pinMode( PIN_DHT, OUTPUT );
	digitalWrite( PIN_DHT, LOW );
	delay(20);
 
	/* prepare to read the pin */
	pinMode( PIN_DHT, INPUT );
 
	/* detect change and read data */
	for (uint16_t i = 0; i < MAX_TIMINGS; i++)
	{
		counter = 0;
		while (digitalRead(PIN_DHT) == laststate)
		{
			counter++;
			delayMicroseconds(1);
			if ( counter == 255 )
			{
				break;
			}
		}

		laststate = digitalRead(PIN_DHT);

		if ( counter == 255 )
			break;
 
		/* ignore first 3 transitions */
		if ( (i >= 4) && (i % 2 == 0) )
		{
			/* shove each bit into the storage bytes */
			data[j / 8] <<= 1;
			if ( counter > 30 )
				data[j / 8] |= 1;
			j++;
		}
	}
 
	/*
	 * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	 * print it out if data is good
	 */
	if ( (j >= 40) && (data[4] == ( (data[0] + data[1] + data[2] + data[3]) & 0xFF) ) )
	{

		float h = (float)((data[0] << 8) + data[1]) / 10;
		if ( h > 100 )
		{
			h = data[0];	// for DHT11
        	*humidity = h;
		}

		float c = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
		if ( c > 125 )
		{
			c = data[2];	// for DHT11
			if ( data[2] & 0x80 )
			{
				c = -c;
			}
			*temperature = c;
		}
		return 0;
	}else{
		return 1;
	}
}