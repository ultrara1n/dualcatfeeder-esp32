# dualcatfeeder-esp32
This is the microcontroller implementation part of the DualCatFeeder project. The code is developed using VS Code with the PlatformIO extension so my urgent advice is to use this constellation if you are planning to build and use the software by yourself.

Here you will only find software and wiring related information, the more important stuff you can find in the [main repository](https://github.com/ultrara1n/DualCatFeeder) 
## Prepare your IDE
## Flash your ESP32
It is necessary to do two separate flash processes to get all the data to your microcontroller. On the one hand you will have to flash the general C++ code and on the other hand some webserver related stuff to the flash memory of your ESP32. But don't worry, at the end there are only two different buttons to press.
### Code
![Upload code](https://github.com/ultrara1n/dualcatfeeder-esp32/raw/master/assets/uploadcode.png "Upload Code")
### Webserver data
![Upload flash](https://github.com/ultrara1n/dualcatfeeder-esp32/raw/master/assets/uploadflash.png "Upload flash")
## Wiring

## WebSocket API
The ESP32 is providing a WebSocket API which is called with JSON input to control and receive data from the device.

### Set Timer
```json
{
    "data": {
        "type": "timer",
        "id": "1",
        "attributes": {
            "active": true,
            "timestamp": 18900,
            "seconds": 30
        }
    }
}
```

### On connect to websocket
```json
{
    "data": {
        "type": "initial",
        "attributes": {
            "boottime": 1611267046,
            "rebootreasoncpu0": "SW_CPU_RESET",
            "rebootreasoncpu1": "SW_CPU_RESET",
            "rssi": -55,
            "freeheap": 164124,
            "rightlastfeedtime": 1611325170,
            "rightlastfeedduration": 15000,
            "leftlastfeedtime": 1611292500,
            "leftlastfeedduration": 30000,
            "timer1": {
                "active": true,
                "timestamp": 18900,
                "seconds": 30
            }
        }
    }
}
```

### Start motor
motor can be *left*, *right* or *both*
```json
{
    "data": {
        "type": "startMotor",
        "attributes": {
            "motor": "right",
            "seconds": 20,
        }
    }
}
```

### Stop all motors
```json
{
    "data": {
        "type": "stopMotor"
    }
}
```

### System operations
```json
{
    "data": {
        "type": "reboot"
    }
}
```

### Feedback when performing operations
```json
{
    "data": {
        "type": "response",
        "attributes": {
            "message": "right"
        }
    }
}
```

## Updating


# Third-party resources
- App icon by [Freepik](https://www.freepik.com/) from [Flaticon](https://www.flaticon.com)
- [ESP Async WebServer from me-no-dev](https://github.com/me-no-dev/ESPAsyncWebServer)