# Connected roads

<div align="center">
    English | <a href='./README_de.md'>Deutsch</a>
</div>

This project is intended to monitor road traffic using several Nordic nRF52840 dongles and connected microphones to extract traffic information based on this.

See also the [documentation](https://gitlab.plagge.it/fh-aachen/ip/vernetzte-strassen/-/wikis/home) for more info.

![Connected Roads](screenshot.png)

## Features

- [x] Connection of several nodes to a mesh network
- [x] Sending commands from a web interface to the nodes
- [x] Recording of audio raw data
- [x] Transmission of audio raw data
- [x] node as a gateway to the Internet
- [x] Extract traffic information from audio raw data
- [ ] Transmission of traffic information to a web interface

## Hardware

- [Nordic nRF52840 Dongle] (https://www.nordicsemi.com/Software-and-tools/Development-Kits/nRF52840-Dongle)
- [microphone]()

## Software

- [Zephyr RTOS](https://www.zephyrproject.org/)

## Installation

### Zephyr

For detailed installation instructions of the development environment see [Getting Started](https://gitlab.plagge.it/fh-aachen/ip/vernetzte-strassen/-/wikis/Ressources/Development%20Setup).

Once the development environment has been set up successfully, the project must be created:

      git clone https://gitlab.plagge.it/fh-aachen/ip/vernetzte-strassen.git
      Git checkout server
      west build -b nrf52840dongle_nrf52840 .

**Or** a release from the [release page](https://gitlab.plagge.it/fh-aachen/ip/vernetzte-strassen/-/releases) can be used directly. Then simply select and flash this .hex file in the nRF programmer.

Then one of the dongles can be connected and flashed using the nRF Programmer Tool from the nRF Connect Toolbox. All nodes use the same build.

Then connect via UART (under Windows e.g.: puTTY) with the BAUD rate 115200. If the button on the dongles is now pressed within the first 5 seconds, the device configures itself as a provisioner so that it manages the mesh network. Currently it is configured as a node and joined the mesh network. If the button is pressed after 5 seconds and the dongle has not yet been provisioned, it will provision itself with test IDs.

If the button is pressed after successful provisioning, the node sends a test message to the mesh network.

### Evaluation

The evaluation script starts a TCP server and waits for measurement data in the following format (big-endian):

```
int32: left channel
int32: right channel
int64: timestamp
```

As soon as an event was detected, an output was made to stdout.

The corresponding container can be built as follows:

```
git checkout evaluation
CD detector
podman build -t detector .
```

Execution e.g. with:

```
podman run --rm -it -p 12345:5678 -e LISTEN_PORT=1234 detector
```

### Server

The web interface is written in TypeScript and can be created as follows:

      CD server
      npm i
      npm run dev # or npm run prod

This can then be reached at [http://localhost:3000](http://localhost:3000). The `server/.env` file should be adjusted beforehand:

      NODE_ENV=Development
      WINDOW_SIZE_MS=900000
      MAX_CONNECTIONS_PER_WINDOW=10000
      PORT=3000
      SERIAL_PORT=COM11
      # RPI: SERIAL_PORT=/dev/ttyACM0
      BAUD_RATE=115200
      SERVER_URL=http://localhost

SERIAL_PORT is particularly relevant here, since it varies depending on the port and device. With Windows, this is usually `COM11`, with Linux `ttyACM0` or similar.

`WINDOW_SIZE_MS` is the size of a window in MS in which `MAX_CONNECTIONS_PER_WINDOW` can be established. If this number is exceeded, the user will be temporarily blocked.

After starting and selecting the correct port, the connected nodes are displayed in the overview, as well as their status. Text messages can be sent to the nodes using the `Send command` box.

### Microphones

Reading the I2S microphones is currently not working.

The following ports are assigned to the 2-channel I2S interface:

- SCK: P0.29
- LRCK: P1.15
- SDIN: P0.02

The audio input can be read out with the following command:

      git checkout microphone-i2s
      west build -b nrf52840dongle_nrf52840 .

Flash again with the nRF Programmer Tool and then connect to the BAUD rate 115200 via UART (under Windows e.g.: puTTY).
