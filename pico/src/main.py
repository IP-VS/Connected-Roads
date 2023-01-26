from machine import I2S
from machine import Pin

SCK_PIN = Pin(10)  # bclk
WS_PIN = Pin(11)  # lrck
SD_PIN = Pin(12)  # data
BITS_PER_SAMPLE = 32
BYTES_PER_SAMPLE = int(BITS_PER_SAMPLE / 8)
CHANNELS = 2
SAMPLE_RATE = 30_000


def record():
    audio_in = I2S(
        0,  # I2S bus, device specific
        sck=SCK_PIN,
        ws=WS_PIN,
        sd=SD_PIN,
        mode=I2S.RX,
        format=I2S.STEREO,
        bits=BITS_PER_SAMPLE,
        rate=SAMPLE_RATE,
        ibuf=20_000,
    )

    buf = bytearray(int(256 * CHANNELS * BYTES_PER_SAMPLE))

    acc_sums = [0] * CHANNELS
    acc_count = 0
    i = 0

    while True:
        num_read = audio_in.readinto(buf)

        for idx in range(0, num_read, int(CHANNELS * BYTES_PER_SAMPLE)):
            samples = buf[idx : idx + int(CHANNELS * BYTES_PER_SAMPLE)]
            for ch in range(0, CHANNELS):
                sample = samples[
                    int(ch * BYTES_PER_SAMPLE) : int((ch + 1) * BYTES_PER_SAMPLE)
                ]
                sample = int.from_bytes(sample, "little", True)  # endian, signed

                acc_sums[ch] += int(sample / 1e6)

            acc_count += 1

            if acc_count >= 256:
                print(
                    "micdata:"
                    + str(acc_sums[0])
                    + ","
                    + str(acc_sums[1])
                    + ","
                    + str(i)
                )
                acc_sums = [0] * CHANNELS
                acc_count = 0
                i += 1
