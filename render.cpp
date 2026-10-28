#include <Bela.h>
#include "sensors/sensors.h"

// MPR121 12-key capacitive touch sensor on Bela's I2C pins (SDA, SCL)
CapacitiveTouch touch;

bool setup(BelaContext *context, void *userData)
{
	// Initialize the touch sensor (stop if it's not found)
	if(!touch.Init()){
		printf("MPR121 not found, check wiring\n");
		return false;
	}
	return true;
}

void render(BelaContext *context, void *userData)
{
	// Touch sensor: takes the latest reading, asks for a new one every ~5 ms
	touch.Update(context);

	// Print as soon as a pad is touched or let go
	for(int pad = 0; pad < CapacitiveTouch::kNumPads; pad++){
		if(touch.JustTouched(pad))
			rt_printf("Pad %d touched\n", pad);
		if(touch.JustReleased(pad))
			rt_printf("Pad %d released\n", pad);
	}
}

void cleanup(BelaContext *context, void *userData)
{
	// Nothing to do here
}
