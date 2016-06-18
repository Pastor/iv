package ru.iv.support.dll;

import ru.iv.support.Device;
import ru.iv.support.Firmware;
import ru.iv.support.Packet;

public interface Callback {

    void connected(Device device, Firmware firmware);

    void disconnected(Device device, Firmware firmware);

    void handle(Device device, Firmware firmware, Packet[] packets, int count);
}
