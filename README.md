# SMC Winter School – Bela Sensors

Ready-to-use sensor classes for the [Bela](https://bela.io) platform. Every sensor lives in a single header file, so you can drop the `sensors/` folder into a Bela project and start reading values with a couple of lines.

## Getting started

1. Create a new C++ project in the Bela IDE.
2. Upload the `sensors/` folder and `render.cpp` to the project (drag and drop them into the IDE file browser).
3. Include everything with one line:

   ```cpp
   #include "sensors/sensors.h"
   ```

4. Create the sensor objects, call `Init()` in `setup()` and `Update()` / `Read()` in `render()`.

The included [render.cpp](render.cpp) is a working example: it prints a message every time a pad on an MPR121 capacitive touch sensor is touched or released.

## Example

```cpp
#include <Bela.h>
#include "sensors/sensors.h"

Potentiometer pot;
Button button;

bool setup(BelaContext *context, void *userData)
{
	pot.Init(0);               // analog input A0
	button.Init(context, 0);   // digital pin D0
	return true;
}

void render(BelaContext *context, void *userData)
{
	button.Update(context);
	float value = pot.Read(context);   // 0.0 to 1.0

	if(button.JustPressed())
		rt_printf("Pressed! Pot is at %f\n", value);
}

void cleanup(BelaContext *context, void *userData) {}
```

## Sensors

| Sensor | Class | Connection | Main functions |
|---|---|---|---|
| Push button | `Button` | Digital pin | `IsPressed()`, `JustPressed()`, `JustReleased()`, `IsHeld(ms)`, `Count()`, `Toggled()` |
| Potentiometer | `Potentiometer` | Analog input | `Read()` → 0.0–1.0 |
| Piezo disc | `Piezo` | Analog input | `Read()` → peak in block, 0.0–1.0 |
| Force sensing resistor (FSR) | `ForceSensor` | Analog input | `Read()` → 0.0–1.0, `IsPressed(threshold)` |
| Stretch sensor (Adafruit rubber cord) | `StretchSensor` | Analog input | `Read()` → 0.0–1.0 |
| Joystick (KY-023) | `Joystick` | 2 analog inputs + digital pin | `X()`, `Y()` → -1.0–1.0, `IsPressed()`, `sw` (a `Button`) |
| Ultrasonic distance (HC-SR04) | `Ultrasonic` | 2 digital pins (Trig, Echo) | `Read()` → cm, or -1 if nothing detected |
| Accelerometer (Adafruit MMA8451) | `Accelerometer` | I2C | `x`, `y`, `z` in m/s² |
| Barometer (Adafruit BMP585) | `Barometer` | I2C | `temperature` (°C), `pressure` (Pa) |
| Capacitive touch (Adafruit MPR121) | `CapacitiveTouch` | I2C | `IsTouched(pad)`, `JustTouched(pad)`, `JustReleased(pad)`, `Raw(pad)` |

Wiring notes for each sensor are in the comment at the top of its header file.

## Things to know

- **Use 3.3V, not 5V.** Bela's pins are not 5V tolerant. Power every sensor (including the HC-SR04) from 3.3V.
- **Analog inputs must be enabled** in the Bela project settings, otherwise analog sensors return 0.
- **I2C sensors run in the background.** The accelerometer, barometer and touch sensor read over I2C in a separate low-priority task, so `render()` never waits on the bus and audio doesn't drop out. Their `Init()` returns `false` if the sensor isn't found – check the wiring, or run `i2cdetect -y -r 1` on the Bela to see which addresses respond.
- **Call `Update()` once per `render()`** for sensors that have it (buttons, joystick, I2C sensors) before reading their values.
- **Joystick:** leave the stick centered when the program starts; the first reading is used as the middle position.
- **Ultrasonic:** up to 4 sensors can be used at once. They take turns so they don't hear each other's echoes.

## Project layout

```
render.cpp                  Example Bela program
sensors/
  sensors.h                 Includes every sensor
  button/Button.h
  potentiometer/Potentiometer.h
  piezo/Piezo.h
  force/ForceSensor.h
  stretch/StretchSensor.h
  joystick/Joystick.h
  distance/Ultrasonic.h
  accelerometer/Accelerometer.h
  barometer/Barometer.h
  touch/CapacitiveTouch.h
```
