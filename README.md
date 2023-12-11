# Hackerhotel 2024 CH32V003 firmware

Clone https://github.com/cnlohr/ch32v003fun somewhere and build/flash using make:

```
make CH32V003FUN=../ch32v003fun/ch32v003fun MINICHLINK=../ch32v003fun/minichlink
```

## Example

```
mkdir hh24ch32
cd hh24ch32
git clone https://github.com/cnlohr/ch32v003fun.git
git clone https://github.com/badgeteam/hackerhotel-2024-firmware-ch32v003.git
cd hackerhotel-2024-firmware-ch32v003
make CH32V003FUN=../ch32v003fun/ch32v003fun MINICHLINK=../ch32v003fun/minichlink
```

## Usage

Shows up on the I2C bus at address `0x42`.

### Registers

Registers 0-4 are the LEDs, every bit in these registers represents one of the LEDs

Registers 5-9 are the buttons, button values are a bitmask of 1 for right, 2 for center and 4 for left

