package ru.iv.support.notify;

import ru.iv.support.Event;
import ru.iv.support.Packet;

@SuppressWarnings({"unused", "FieldCanBeLocal"})
final class PacketsNotify extends DeviceNotify {
    private final Packet[] packets;

    PacketsNotify(Event event) {
        super(Type.DEVICE_RECEIVE, event);
        this.packets = event.packets;
    }
}
