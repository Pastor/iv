package ru.iv.support.notify;

import ru.iv.support.Device;
import ru.iv.support.Event;
import ru.iv.support.Firmware;

@SuppressWarnings({"unused", "FieldCanBeLocal"})
abstract class DeviceNotify extends Notify {
    private final Device device;
    private final Firmware firmware;

    DeviceNotify(Type type, Event event) {
        super(type);
        this.device = event.device;
        this.firmware = event.firmware;
    }
}
